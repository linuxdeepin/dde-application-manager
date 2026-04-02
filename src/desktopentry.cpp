// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"
#include "desktopentry.h"
#include "desktopfileparser.h"
#include <QDir>
#include <QDirIterator>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QStringView>
#include <QVariant>
#include <sys/stat.h>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(logDesktopEntry, "dde.am.desktopfile.entry")

namespace {
[[nodiscard]] bool isValidDBusWellKnownNameElement(QStringView element) noexcept
{
    if (element.isEmpty() || element.front().isDigit()) {
        return false;
    }

    for (const auto ch : element) {
        if (ch.isLetterOrNumber() || ch == u'-' || ch == u'_') {
            continue;
        }

        return false;
    }

    return true;
}

[[nodiscard]] bool isValidApplicationDesktopFileStem(QStringView stem) noexcept
{
    if (stem.isEmpty()) {
        return false;
    }

    auto current = stem;
    while (!current.isEmpty()) {
        const auto dotIndex = current.indexOf(u'.');
        const auto element = dotIndex == -1 ? current : current.first(dotIndex);
        if (!isValidDBusWellKnownNameElement(element)) {
            return false;
        }

        if (dotIndex == -1) {
            return true;
        }

        current = current.sliced(dotIndex + 1);
    }

    return true;
}

[[nodiscard]] std::optional<std::pair<qint64, qint64>> fileTimesFromStat(const QString &path) noexcept
{
    const auto utf8Path = path.toUtf8();
    struct stat st{};
    if (::stat(utf8Path.constData(), &st) != 0) {
        return std::nullopt;
    }

    auto toMSecs = [](const struct timespec &ts) -> qint64 { return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000; };
    return std::pair{toMSecs(st.st_mtim), toMSecs(st.st_ctim)};
}
}  // namespace

bool DesktopEntry::checkMainEntryValidation() const noexcept
{
    auto it = m_entryMap.constFind(fromStaticRaw(DesktopFileEntryKey));
    if (it == m_entryMap.cend()) {
        return false;
    }

    if (auto name = it->constFind(fromStaticRaw(DesktopEntryName)); name == it->cend()) {
        qCWarning(logDesktopEntry) << "No Name entry";
        return false;
    }

    auto type = it->constFind(fromStaticRaw(DesktopEntryType));
    if (type == it->cend()) {
        qCWarning(logDesktopEntry) << "No Type entry";
        return false;
    }

    const auto &typeStr = type->toString();
    if (typeStr.isEmpty()) {
        return false;
    }

    if (typeStr == fromStaticRaw(DesktopEntryLink)) {
        if (!it->contains(fromStaticRaw(DesktopEntryURL))) {
            return false;
        }
    }

    return true;
}

std::optional<DesktopFile> DesktopFile::createTemporaryDesktopFile(std::unique_ptr<QFile> temporaryFile) noexcept
{
    const auto utf8Path = temporaryFile->fileName().toUtf8();
    struct stat st{};
    if (::stat(utf8Path.constData(), &st) != 0) {
        qCWarning(logDesktopEntry) << "create temporary file failed.";
        return std::nullopt;
    }
    auto toMSecs = [](const struct timespec &ts) -> qint64 { return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000; };
    return DesktopFile{std::move(temporaryFile), "", toMSecs(st.st_mtim), toMSecs(st.st_ctim)};
}

std::optional<DesktopFile> DesktopFile::createTemporaryDesktopFile(QUtf8StringView temporaryFile) noexcept
{
    const static QString userTmp = u"/run/user/" % QString::number(getCurrentUID());
    auto tempFile = std::make_unique<QFile>(userTmp % QUuid::createUuid().toString(QUuid::Id128) % desktopSuffix);

    if (!tempFile->open(QFile::NewOnly | QFile::WriteOnly | QFile::Text)) {
        qCWarning(logDesktopEntry) << "failed to create temporary desktop file:" << tempFile->fileName()
                                   << tempFile->errorString();
        return std::nullopt;
    }

    auto writeByte = tempFile->write(temporaryFile.data(), temporaryFile.size());
    if (writeByte == -1 || writeByte != temporaryFile.size()) {
        qCWarning(logDesktopEntry) << "write to temporary file failed:" << tempFile->errorString();
        return std::nullopt;
    }

    tempFile->close();

    return createTemporaryDesktopFile(std::move(tempFile));
}

std::optional<DesktopFile> DesktopFile::createDesktopFile(const QFileInfo &desktopFileInfo, QString desktopId) noexcept
{
    const auto absolutePath = desktopFileInfo.absoluteFilePath();
    const QStringView pathView{absolutePath};

    if (!desktopFileInfo.isFile() || !desktopFileInfo.isAbsolute()) {
        qCWarning(logDesktopEntry) << "desktop file not found.";
        return std::nullopt;
    }

    if (!pathView.endsWith(desktopSuffix)) {
        qCWarning(logDesktopEntry) << "file isn't a desktop file:" << pathView;
        return std::nullopt;
    }

    const auto times = fileTimesFromStat(absolutePath);
    if (!times) {
        qCWarning(logDesktopEntry) << "desktop file not found.";
        return std::nullopt;
    }

    auto filePtr = std::make_unique<QFile>(absolutePath);
    return DesktopFile{std::move(filePtr), std::move(desktopId), times->first, times->second};
}

bool DesktopFile::hasStandardizedApplicationFileName() const noexcept
{
    const QFileInfo info{m_sourcePath};
    const auto fileName = info.fileName();
    const QStringView fileNameView{fileName};
    if (!fileNameView.endsWith(desktopSuffix)) {
        return false;
    }

    return isValidApplicationDesktopFileStem(fileNameView.chopped(desktopSuffix.size()));
}

std::optional<DesktopFile> DesktopFile::searchDesktopFileByPath(const QString &desktopFile, ParserError &err) noexcept
{
    const QStringView pathView{desktopFile};

    if (!pathView.endsWith(desktopSuffix)) {
        qCWarning(logDesktopEntry) << "file isn't a desktop file:" << pathView;
        err = ParserError::MismatchedFile;
        return std::nullopt;
    }

    if (!QDir::isAbsolutePath(desktopFile)) {
        qCWarning(logDesktopEntry) << "desktop file not found.";
        err = ParserError::NotFound;
        return std::nullopt;
    }

    const auto utf8Path = desktopFile.toUtf8();
    struct stat st{};
    if (::stat(utf8Path.constData(), &st) != 0) {
        qCWarning(logDesktopEntry) << "desktop file not found.";
        err = ParserError::NotFound;
        return std::nullopt;
    }

    // Extract desktop ID from path: .../applications/subdir/app.desktop -> subdir-app
    const auto base = pathView.chopped(desktopSuffix.size());
    const auto tokens = qTokenize(base, QDir::separator(), Qt::SkipEmptyParts);

    QString id;
    bool foundRoot{false};
    for (auto component : tokens) {
        if (component.isEmpty()) {
            continue;
        }

        if (!foundRoot) {
            if (component == u"applications" || component == u"autostart") {
                foundRoot = true;
            }

            continue;
        }

        if (!id.isEmpty()) {
            id.append(u'-');
        }

        id.append(component);
    }

    err = ParserError::NoError;
    const QFileInfo info{desktopFile};
    if (auto ret = createDesktopFile(info, std::move(id)); ret) {
        return ret;
    }

    err = ParserError::NotFound;
    return std::nullopt;
}

std::optional<DesktopFile> DesktopFile::searchDesktopFileById(QStringView appId, ParserError &err) noexcept
{
    auto appDirs = getApplicationsDirs();

    for (const auto &dir : std::as_const(appDirs)) {
        QString relativePath = appId % desktopSuffix;

        while (true) {
            auto fullPath = QDir{dir}.filePath(relativePath);

            if (QFile::exists(fullPath)) {
                return searchDesktopFileByPath(fullPath, err);
            }

            auto hyphenIndex = relativePath.indexOf(u'-');
            if (hyphenIndex == -1) {
                break;
            }

            relativePath[hyphenIndex] = QDir::separator();  // NOLINT
        }
    }

    err = ParserError::NotFound;
    return std::nullopt;
}

bool DesktopFile::modified(qint64 time) const noexcept
{
    return time != m_mtime;
}

ParserError DesktopEntry::parse(const DesktopFile &file) noexcept
{
    DesktopFileGuard guard{file};

    if (!guard.try_open()) {
        qCWarning(logDesktopEntry) << file.sourcePath() << "can't open.";
        return ParserError::OpenFailed;
    }

    return parse(file.sourceFileRef());
}

ParserError DesktopEntry::parse(QFile &file) noexcept
{
    if (m_parsed) {
        return ParserError::Parsed;
    }

    if (file.atEnd()) {
        return ParserError::OpenFailed;
    }

    ParserError err{ParserError::NoError};
    DesktopFileParser p(file);
    err = p.parse(m_entryMap);
    m_parsed = true;
    if (err != ParserError::NoError) {
        return err;
    }

    if (!checkMainEntryValidation()) {
        qCWarning(logDesktopEntry) << "invalid MainEntry, abort.";
        err = ParserError::MissingInfo;
    }

    return err;
}

std::optional<std::reference_wrapper<const QMap<QString, DesktopEntry::Value>>>
DesktopEntry::group(const QString &key) const noexcept
{
    if (auto group = m_entryMap.constFind(key); group != m_entryMap.cend()) {
        return *group;
    }

    return std::nullopt;
}

std::optional<std::reference_wrapper<const DesktopEntry::Value>> DesktopEntry::value(const QString &groupKey,
                                                                                     const QString &valueKey) const noexcept
{
    const auto &destGroup = group(groupKey);
    if (!destGroup) {
        qCDebug(logDesktopEntry) << "group " << groupKey << " can't be found.";
        return std::nullopt;
    }

    const auto &groupRef = destGroup->get();
    auto it = groupRef.constFind(valueKey);
    if (it == groupRef.cend()) {
        qCDebug(logDesktopEntry) << "value " << valueKey << " can't be found.";
        return std::nullopt;
    }
    return *it;
}

void DesktopEntry::insert(const QString &key, const QString &valueKey, Value &&val) noexcept
{
    auto &outer = m_entryMap[key];  // NOLINT
    outer.insert(valueKey, val);
}

QString unescapeValue(QStringView str) noexcept
{
    QString out;
    out.reserve(str.size());

    for (const auto *it = str.begin(); it != str.end(); ++it) {
        if (*it == u'\\' && (it + 1) != str.end()) {
            const auto next = (*(++it)).unicode();
            switch (next) {
            case 's':
                out.append(u' ');
                break;
            case 'n':
                out.append(u'\n');
                break;
            case 't':
                out.append(u'\t');
                break;
            case 'r':
                out.append(u'\r');
                break;
            case '\\':
                out.append(u'\\');
                break;
            case ';':
                out.append(u';');
                break;
            default:
                out.append(u'\\');
                out.append(next);
                break;
            }
        } else {
            out.append(*it);
        }
    }

    return out;
}

QString toString(const DesktopEntry::Value &value, bool skipUnescape) noexcept
{
    const auto id = value.userType();

    QString str;
    if (id == QMetaTypeId<QStringMap>::qt_metatype_id()) {  // get default locale
        const auto &val = *static_cast<const QStringMap *>(value.constData());
        auto it = val.constFind(fromStaticRaw(DesktopFileDefaultKeyLocale));
        if (it != val.cend()) {
            str = it.value();
        }
    } else if (id == QMetaType::QString) {
        str = *static_cast<const QString *>(value.constData());
    } else {
        qCritical() << "unknown value type:" << id;
        return {};
    }

    if (str.isEmpty()) {
        qCWarning(logDesktopEntry) << "failed to convert value to string.";
        return str;
    }

    if (!skipUnescape) {
        str = unescapeValue(str);
    }

    if (hasNonAsciiAndControlCharacters(str)) {
        return {};
    }

    return str;
}

QString toLocaleString(const DesktopEntry::Value &localeEntry, const QLocale &locale) noexcept
{
    // see: https://specifications.freedesktop.org/desktop-entry/latest/localized-keys.html
    if (localeEntry.userType() != QMetaTypeId<QStringMap>::qt_metatype_id()) {
        return {};
    }

    const auto &localeMap = *static_cast<const QStringMap *>(localeEntry.constData());
    if (Q_UNLIKELY(localeMap.isEmpty())) {
        return {};
    }

    const auto posixName = locale.name(QLocale::TagSeparator::Underscore);
    const QStringView name{posixName};

    QStringView modifier;
    const auto atIdx = name.indexOf(u'@');
    const auto mainPart = (atIdx != -1) ? name.sliced(0, atIdx) : name;
    if (atIdx != -1) {
        modifier = name.sliced(atIdx + 1);
    }

    QStringView lang;
    QStringView country;
    const auto dotIdx = mainPart.indexOf(u'.');
    const auto cleanMain = (dotIdx != -1) ? mainPart.sliced(0, dotIdx) : mainPart;

    const auto underscoreIdx = cleanMain.indexOf(u'_');
    if (underscoreIdx != -1) {
        lang = cleanMain.sliced(0, underscoreIdx);
        country = cleanMain.sliced(underscoreIdx + 1);
    } else {
        lang = cleanMain;
    }

    QVarLengthArray<QString, 4> candidates;
    if (!country.isEmpty() && !modifier.isEmpty()) {
        // lang_COUNTRY@modifier
        candidates.append(lang % u'_' % country % u'@' % modifier);
    }

    if (!country.isEmpty()) {
        // lang_COUNTRY
        candidates.append(lang % u'_' % country);
    }

    if (!modifier.isEmpty()) {
        // lang@modifier
        candidates.append(lang % u'@' % modifier);
    }

    // lang
    candidates.append(lang.toString());

    for (const auto &key : candidates) {
        auto it = localeMap.constFind(key);
        if (it != localeMap.cend()) {
            return unescapeValue(it.value());
        }
    }

    return {};
}

QString toIconString(const DesktopEntry::Value &value) noexcept
{
    return toString(value);
}

bool toBoolean(const DesktopEntry::Value &value, bool &ok) noexcept
{
    ok = false;
    const auto &str = toString(value);
    if (str == u"true"_s) {
        ok = true;
        return true;
    }

    if (str == u"false"_s) {
        ok = true;
        return false;
    }

    return false;
}

float toNumeric(const DesktopEntry::Value &value, bool &ok) noexcept
{
    return value.toFloat(&ok);
}

bool operator==(const DesktopEntry &lhs, const DesktopEntry &rhs)
{
    if (lhs.m_parsed != rhs.m_parsed) {
        return false;
    }

    if (lhs.m_entryMap != rhs.m_entryMap) {
        return false;
    }

    return true;
}

bool operator!=(const DesktopEntry &lhs, const DesktopEntry &rhs)
{
    return !(lhs == rhs);
}

bool operator==(const DesktopFile &lhs, const DesktopFile &rhs)
{
    if (lhs.m_desktopId != rhs.m_desktopId) {
        return false;
    }

    if (lhs.sourcePath() != rhs.sourcePath()) {
        return false;
    }

    if (lhs.m_ctime != rhs.m_ctime or lhs.m_mtime != rhs.m_mtime) {
        return false;
    }

    return true;
}

bool operator!=(const DesktopFile &lhs, const DesktopFile &rhs)
{
    return !(lhs == rhs);
}
