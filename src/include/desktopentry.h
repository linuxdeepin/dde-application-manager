// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QString>
#include <QMap>
#include <QDebug>
#include <QLocale>
#include <QTextStream>
#include <optional>

constexpr static auto defaultKeyStr = "default";

enum class ParseError {
    NoError,
    NotFound,
    MismatchedFile,
    InvalidLocation,
    OpenFailed,
    GroupHeaderInvalid,
    EntryKeyInvalid
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
        [[nodiscard]] QString unescape(const QString &str) const noexcept;
    };

    DesktopEntry() = default;
    ~DesktopEntry() = default;
    [[nodiscard]] ParseError parse(QTextStream& stream) noexcept;
    [[nodiscard]] QMap<QString, Value> group(const QString &key) const noexcept;

private:
    QMap<QString, QMap<QString, Value>> m_entryMap;

    auto parserGroupHeader(const QString &str) noexcept;
    ParseError parseEntry(const QString &str, decltype(m_entryMap)::iterator &currentGroup) noexcept;
};

struct DesktopFile
{
    DesktopFile(const DesktopFile &) = default;
    DesktopFile(DesktopFile &&) = default;
    DesktopFile &operator=(const DesktopFile &) = default;
    DesktopFile &operator=(DesktopFile &&) = default;
    ~DesktopFile() = default;

    const QString &filePath() const { return m_filePath; }
    const QString &desktopId() const { return m_desktopId; }
    
    static std::optional<DesktopFile> searchDesktopFile(const QString &desktopFilePath, ParseError& err) noexcept;

private:
    DesktopFile(QString &&path,QString &&fileId):m_filePath(std::move(path)),m_desktopId(std::move(fileId)){}
    QString m_filePath;
    QString m_desktopId;
};

QDebug operator<<(QDebug debug, const DesktopEntry::Value& v);

QDebug operator<<(QDebug debug, const ParseError &v);
