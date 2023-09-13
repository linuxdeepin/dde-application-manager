// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DESKTOPENTRY_H
#define DESKTOPENTRY_H

#include <QString>
#include <QMap>
#include <QDebug>
#include <QLocale>
#include <QTextStream>
#include <optional>
#include <QFile>

constexpr static auto defaultKeyStr = "default";

enum class DesktopErrorCode {
    NoError,
    NotFound,
    MismatchedFile,
    InvalidLocation,
    InvalidFormat,
    OpenFailed,
    MissingInfo,
    Parsed,
    InternalError,
};

enum class EntryContext { Unknown, EntryOuter, Entry, Done };

enum class EntryValueType { String, LocaleString, Boolean, IconString };

struct DesktopFileGuard;

struct DesktopFile
{
    friend struct DesktopFileGuard;
    DesktopFile(const DesktopFile &) = delete;
    DesktopFile(DesktopFile &&) = default;
    DesktopFile &operator=(const DesktopFile &) = delete;
    DesktopFile &operator=(DesktopFile &&) = default;
    ~DesktopFile() = default;

    [[nodiscard]] QString sourcePath() const noexcept;
    // WARNING: This raw pointer's ownership belong to DesktopFile, DO NOT MODIFY!
    [[nodiscard]] QFile *sourceFile() const noexcept { return &sourceFileRef(); };
    [[nodiscard]] QFile &sourceFileRef() const noexcept { return *m_fileSource; };
    [[nodiscard]] const QString &desktopId() const noexcept { return m_desktopId; }
    [[nodiscard]] bool modified(qint64 time) const noexcept;
    [[nodiscard]] qint64 createTime() const noexcept { return m_ctime; }

    static std::optional<DesktopFile> searchDesktopFileById(const QString &appId, DesktopErrorCode &err) noexcept;
    static std::optional<DesktopFile> searchDesktopFileByPath(const QString &desktopFilePath, DesktopErrorCode &err) noexcept;
    static std::optional<DesktopFile> createTemporaryDesktopFile(const QString &temporaryFile) noexcept;
    static std::optional<DesktopFile> createTemporaryDesktopFile(std::unique_ptr<QFile> temporaryFile) noexcept;

private:
    DesktopFile(std::unique_ptr<QFile> source, QString fileId, qint64 mtime, qint64 ctime)
        : m_mtime(mtime)
        , m_ctime(ctime)
        , m_fileSource(std::move(source))
        , m_desktopId(std::move(fileId))
    {
    }

    qint64 m_mtime;
    qint64 m_ctime;
    std::unique_ptr<QFile> m_fileSource{nullptr};
    QString m_desktopId{""};
};

struct DesktopFileGuard
{
    DesktopFileGuard(const DesktopFileGuard &) = delete;
    DesktopFileGuard(DesktopFileGuard &&other) noexcept
        : fileRef(other.fileRef)
    {
    }
    DesktopFileGuard &operator=(const DesktopFileGuard &) = delete;
    DesktopFileGuard &operator=(DesktopFileGuard &&) = delete;

    explicit DesktopFileGuard(const DesktopFile &file)
        : fileRef(file)
    {
    }

    bool try_open()
    {
        if (fileRef.m_fileSource->isOpen()) {
            return true;
        }

        auto ret = fileRef.m_fileSource->open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text);
        if (!ret) {
            qWarning() << "open desktop file failed:" << fileRef.m_fileSource->errorString();
        }

        return ret;
    }

    ~DesktopFileGuard()
    {
        if (fileRef.m_fileSource->isOpen()) {
            fileRef.m_fileSource->close();
        }
    }

private:
    const DesktopFile &fileRef;
};

class DesktopEntry
{
public:
    class Value final : public QMap<QString, QString>
    {
    public:
        using QMap<QString, QString>::QMap;
        QString toString(bool &ok) const noexcept;
        bool toBoolean(bool &ok) const noexcept;
        QString toIconString(bool &ok) const noexcept;
        float toNumeric(bool &ok) const noexcept;
        QString toLocaleString(const QLocale &locale, bool &ok) const noexcept;
        friend QDebug operator<<(QDebug debug, const DesktopEntry::Value &v);

    private:
        [[nodiscard]] static QString unescape(const QString &str) noexcept;
    };

    DesktopEntry() = default;
    DesktopEntry(const DesktopEntry &) = default;
    DesktopEntry(DesktopEntry &&) = default;
    DesktopEntry &operator=(const DesktopEntry &) = default;
    DesktopEntry &operator=(DesktopEntry &&) = default;

    ~DesktopEntry() = default;
    [[nodiscard]] DesktopErrorCode parse(const DesktopFile &file) noexcept;
    [[nodiscard]] DesktopErrorCode parse(QTextStream &stream) noexcept;
    [[nodiscard]] std::optional<QMap<QString, Value>> group(const QString &key) const noexcept;
    [[nodiscard]] std::optional<Value> value(const QString &key, const QString &valueKey) const noexcept;

    friend bool operator==(const DesktopEntry &lhs, const DesktopEntry &rhs);
    friend bool operator!=(const DesktopEntry &lhs, const DesktopEntry &rhs);

private:
    [[nodiscard]] bool checkMainEntryValidation() const noexcept;
    QMap<QString, QMap<QString, Value>> m_entryMap;
    bool m_parsed{false};
};

QDebug operator<<(QDebug debug, const DesktopEntry::Value &v);

QDebug operator<<(QDebug debug, const DesktopErrorCode &v);

bool operator==(const DesktopEntry &lhs, const DesktopEntry &rhs);

bool operator!=(const DesktopEntry &lhs, const DesktopEntry &rhs);

#endif
