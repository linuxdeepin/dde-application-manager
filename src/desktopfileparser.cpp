// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QRegularExpression>
#include <QStringBuilder>
#include <QLoggingCategory>

#include "desktopfileparser.h"
#include "constant.h"
#include "global.h"

Q_LOGGING_CATEGORY(logDesktopFileParser, "dde.am.desktopfile.parser")
using namespace Qt::StringLiterals;

namespace {
QString toUtf8String(QByteArrayView view) noexcept
{
    return QString::fromUtf8(view);
}

QString toLatin1String(QByteArrayView view) noexcept
{
    return QString::fromLatin1(view.data(), view.size());
}

bool isLocaleString(QStringView key) noexcept
{
    return key == u"Name"_s || key == u"GenericName"_s || key == u"Comment"_s || key == u"Keywords"_s;
}

bool isValidLocaleString(QStringView str) noexcept
{
    // linear search
    if (str.isEmpty()) {
        return false;
    }

    const auto *it = str.cbegin();
    const auto *const end = str.end();

    // 1. Language: [a-z]+ (MUST)
    if (it == end || !it->isLower()) {
        return false;
    }

    while (it != end && it->isLower()) {
        ++it;
    }

    // 2. Country: _[A-Z]+ (OPTIONAL)
    if (it != end && *it == u'_') {
        ++it;

        if (it == end || !it->isUpper()) {
            return false;
        }

        while (it != end && it->isUpper()) {
            ++it;
        }
    }

    // 3. Encoding: .[0-9A-Z-]+ (OPTIONAL)
    if (it != end && *it == u'.') {
        ++it;

        auto isEncodingChar = [](auto c) {
            const auto u = c.unicode();
            return (u >= u'A' && u <= u'Z') || (u >= u'a' && u <= u'z') || (u >= u'0' && u <= u'9') || u == u'-';
        };

        if (it == end || !isEncodingChar(*it)) {
            return false;
        }

        while (it != end && isEncodingChar(*it)) {
            ++it;
        }
    }

    // 4. Modifier: @[A-Za-z0-9_-]+ (OPTIONAL)
    if (it != end && *it == u'@') {
        ++it;

        auto isModifierChar = [](auto c) {
            const auto u = c.unicode();
            return (u >= u'a' && u <= u'z') || (u >= u'A' && u <= u'Z') || (u >= u'0' && u <= u'9') || u == u'_' || u == u'-';
        };

        if (it == end || !isModifierChar(*it)) {
            return false;
        }

        while (it != end && isModifierChar(*it)) {
            ++it;
        }
    }

    // 5. must be end
    return it == end;
}

}  // namespace

ParserError DesktopFileParser::parse(Groups &ret) noexcept
{
    bool isFirstGroup{true};

    skip();

    while (!atEnd()) {
        if (m_line.isEmpty()) {
            break;
        }

        QString currentGroupName;
        auto err = addGroup(ret, currentGroupName);
        if (err != ParserError::NoError) {
            return err;
        }

        if (isFirstGroup && !currentGroupName.isEmpty()) {
            if (currentGroupName != QStringView{DesktopFileEntryKey}) {
                qCWarning(logDesktopFileParser) << "There should be nothing preceding "
                                                   "'Desktop Entry' group in the desktop entry file "
                                                   "but possibly one or more comments.";
                return ParserError::InvalidFormat;
            }

            isFirstGroup = false;
        }
    }

    if (!m_line.isEmpty()) {
        qCCritical(logDesktopFileParser) << "Something is wrong in Desktop file parser, check logic.";
        return ParserError::InternalError;
    }

    return ParserError::NoError;
}

ParserError DesktopFileParser::addGroup(Groups &groups, QString &groupName) noexcept
{
    if (m_line.isEmpty() || m_line.front() != '[' || m_line.back() != ']') {
        qCDebug(logDesktopFileParser) << "Invalid desktop file format: unexpected line:" << toUtf8String(m_line);
        return ParserError::InvalidFormat;
    }

    // Parsing group header.
    // https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#group-header

    const auto groupNameBytes = m_line.sliced(1, m_line.size() - 2).trimmed();
    if (std::any_of(groupNameBytes.cbegin(), groupNameBytes.cend(), [](auto ch) { return ch == '[' || ch == ']'; })) {
        qCDebug(logDesktopFileParser) << "group header invalid:" << toUtf8String(m_line);
        return ParserError::InvalidFormat;
    }

    auto groupNameView = QStringView{groupName = toUtf8String(groupNameBytes)};
    if (hasNonAsciiAndControlCharacters(groupNameView)) {
        qCDebug(logDesktopFileParser) << "group header invalid:" << groupName;
        return ParserError::InvalidFormat;
    }

    auto group = groups.find(groupName);
    if (group != groups.end()) {
        qCDebug(logDesktopFileParser) << "duplicated group header detected:" << groupNameView;
        return ParserError::InvalidFormat;
    }

    group = groups.insert(groupName, {});
    clearLine();

    while (!atEnd()) {
        skip();

        if (m_line.isEmpty()) {
            break;
        }

        if (m_line.startsWith('[')) {
            // End of this group and start of next group, just break
            break;
        }

        auto err = addEntry(group);
        if (err != ParserError::NoError) {
            return err;
        }
    }

    return ParserError::NoError;
}

ParserError DesktopFileParser::addEntry(Groups::iterator group) noexcept
{
    const auto splitCharIndex = m_line.indexOf('=');
    if (splitCharIndex == -1) {
        qCDebug(logDesktopFileParser) << "invalid line in desktop file, skip it:" << toUtf8String(m_line);
        clearLine();
        return ParserError::NoError;
    }

    const auto keyBytes = m_line.first(splitCharIndex).trimmed();
    const auto valueBytes = m_line.sliced(splitCharIndex + 1).trimmed();

    // NOTE:
    // We are process "localized keys" here, for usage check:
    // https://specifications.freedesktop.org/desktop-entry/latest/localized-keys.html
    const qsizetype localeBegin = keyBytes.indexOf('[');
    const qsizetype localeEnd = keyBytes.lastIndexOf(']');
    if ((localeBegin == -1) != (localeEnd == -1)) {
        qCDebug(logDesktopFileParser) << "unmatched [] detected in desktop file, skip this line: " << toUtf8String(m_line);
        clearLine();

        return ParserError::NoError;
    }

    const bool hasLocaleKey = localeBegin != -1;
    QByteArrayView mainKeyBytes = keyBytes;
    QString localeKey;
    QStringView localeKeyView;
    if (hasLocaleKey) {
        mainKeyBytes = keyBytes.sliced(0, localeBegin).trimmed();
        localeKey = toLatin1String(keyBytes.sliced(localeBegin + 1, localeEnd - localeBegin - 1).trimmed());
    } else {
        localeKey = fromStaticRaw(DesktopFileDefaultKeyLocale);
    }
    localeKeyView = localeKey;

    for (auto ch : mainKeyBytes) {
        if ((ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') && ch != '-') {
            clearLine();
            qCDebug(logDesktopFileParser).noquote()
                << QString(u"invalid KEY (%2) for key \"%1\"").arg(toUtf8String(keyBytes), toUtf8String(mainKeyBytes));
            return ParserError::NoError;
        }
    }

    if (hasLocaleKey) {
        if (localeKeyView.isEmpty() || !isValidLocaleString(localeKeyView)) {
            clearLine();
            qCDebug(logDesktopFileParser).noquote()
                << QString(u"invalid LOCALE (%2) for key \"%1\"").arg(toUtf8String(keyBytes), localeKeyView);
            return ParserError::NoError;
        }
    }

    const auto mainKey = toLatin1String(mainKeyBytes);
    const auto mainKeyView = QStringView{mainKey};
    const auto supportsLocale{isLocaleString(mainKeyView)};
    auto valVariant = group->lowerBound(mainKey);
    const bool keyExists = valVariant != group->end() && valVariant.key() == mainKey;

    if (keyExists) {
        if (!supportsLocale) {
            qCDebug(logDesktopFileParser) << "duplicate key:" << mainKeyView << "skip.";
            clearLine();
            return ParserError::NoError;
        }

        auto &val = valVariant.value();
        // maybe custom key has locale string, try to promote it to QStringMap
        if (val.userType() != QMetaType::fromType<QStringMap>().id()) {
            QStringMap newMap{{fromStaticRaw(DesktopFileDefaultKeyLocale), val.toString()}};
            val = QVariant::fromValue(std::move(newMap));
        }

        auto *map = static_cast<QStringMap *>(val.data());
        auto localeIt = map->lowerBound(localeKey);
        if (localeIt != map->end() && localeIt.key() == localeKey) {
            qCDebug(logDesktopFileParser) << "duplicate locale key:" << mainKeyView << "[" << localeKeyView << "]";
        } else {
            map->insert(localeIt, localeKey, toUtf8String(valueBytes));
        }
    } else {
        if (supportsLocale) {
            group->insert(valVariant, mainKey, QVariant::fromValue(QStringMap{{localeKey, toUtf8String(valueBytes)}}));
        } else {
            group->insert(valVariant, mainKey, toUtf8String(valueBytes));
        }
    }

    clearLine();
    return ParserError::NoError;
}

QString toString(const DesktopFileParser::Groups &groups)
{
    if (groups.isEmpty()) {
        return {};
    }

    QString ret;
    ret.reserve(groups.size() * 2048);

    auto groupToString = [&ret, &groups, defaultLoc = QStringView{DesktopFileDefaultKeyLocale}](auto it) {
        if (it == groups.cend()) {
            return;
        }

        const auto &groupEntry = it.value();
        ret.append(u'[' % it.key() % u"]\n");

        for (const auto &[key, value] : groupEntry.asKeyValueRange()) {
            const int typeId = value.userType();
            if (typeId == QMetaType::fromType<QStringMap>().id()) {
                const auto &rawMap = *static_cast<const QStringMap *>(value.constData());
                for (const auto &[locKey, locValue] : rawMap.asKeyValueRange()) {
                    ret.append(key);
                    if (locKey != defaultLoc) {
                        ret.append(u'[' % locKey % u']');
                    }

                    ret.append(u'=' % locValue % u'\n');
                }
            } else if (typeId == QMetaType::QStringList) {
                const auto &list = *static_cast<const QStringList *>(value.constData());
                ret.append(key % u'=' % list.join(u';') % u'\n');
            } else {
                ret.append(key % u'=' % value.toString() % u'\n');
            }
        }

        ret.append(u'\n');
    };

    const auto &mainEntry = fromStaticRaw(DesktopFileEntryKey);
    auto mainEntryIt = groups.constFind(mainEntry);
    if (mainEntryIt == groups.cend()) {
        qCWarning(logDesktopFileParser) << "Desktop Entry group not found, failing to serialize to string.";
        return {};
    }

    groupToString(mainEntryIt);

    auto actionsIt = mainEntryIt->constFind(fromStaticRaw(DesktopEntryActions));
    if (actionsIt != mainEntryIt->cend()) {
        const auto &actionValue = actionsIt.value();
        if (actionValue.userType() != QMetaType::QString) {
            qCWarning(logDesktopFileParser) << "Actions entry is not stored as QString, serializing via toString():"
                                            << actionValue.metaType().name();
        }
        const auto actions = actionValue.toString().split(u';', Qt::SkipEmptyParts);

        for (const auto &action : actions) {
            if (action.isEmpty()) {
                continue;
            }

            auto groupActionIt = groups.constFind(DesktopFileActionKey % action);
            groupToString(groupActionIt);
        }
    }

    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        const auto &key = it.key();
        if (key == mainEntry || key.startsWith(DesktopFileActionKey)) {
            continue;
        }

        groupToString(it);
    }

    return ret;
}
