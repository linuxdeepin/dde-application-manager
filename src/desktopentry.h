// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QString>
#include <QMap>
#include <QDebug>
#include <QLocale>
#include <QTextStream>
#include <optional>
#include <sys/stat.h>

constexpr static auto defaultKeyStr = "default";

enum class DesktopErrorCode {
    NoError,
    NotFound,
    MismatchedFile,
    InvalidLocation,
    OpenFailed,
    GroupHeaderInvalid,
    EntryKeyInvalid
};

struct DesktopFile
{
    DesktopFile(const DesktopFile &) = default;
    DesktopFile(DesktopFile &&) = default;
    DesktopFile &operator=(const DesktopFile &) = default;
    DesktopFile &operator=(DesktopFile &&) = default;
    ~DesktopFile() = default;

    [[nodiscard]] const QString &filePath() const { return m_filePath; }
    [[nodiscard]] const QString &desktopId() const { return m_desktopId; }

    static std::optional<DesktopFile> searchDesktopFileById(const QString &appId, DesktopErrorCode &err) noexcept;
    static std::optional<DesktopFile> searchDesktopFileByPath(const QString &desktopFilePath, DesktopErrorCode &err) noexcept;
    [[nodiscard]] bool modified(std::size_t time) const noexcept;

private:
    DesktopFile(QString &&path, QString &&fileId, std::size_t mtime)
        : m_mtime(mtime)
        , m_filePath(std::move(path))
        , m_desktopId(std::move(fileId))
    {
    }

    std::size_t m_mtime;
    QString m_filePath{""};
    QString m_desktopId{""};
};

class DesktopEntry
{
public:
    class Value : public QMap<QString, QString>
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

private:
    QMap<QString, QMap<QString, Value>> m_entryMap;
    auto parserGroupHeader(const QString &str) noexcept;
    static DesktopErrorCode parseEntry(const QString &str, decltype(m_entryMap)::iterator &currentGroup) noexcept;
};

QDebug operator<<(QDebug debug, const DesktopEntry::Value &v);

QDebug operator<<(QDebug debug, const DesktopErrorCode &v);
