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
    FilePathEmpty,
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
        QString unescape(const QString& str) const noexcept;
    };

    DesktopEntry() = default;
    ~DesktopEntry() = default;
    ParseError parse(QTextStream& stream) noexcept;
    QMap<QString, Value> group(const QString &key) const noexcept;

private:
    QMap<QString, QMap<QString, Value>> m_entryMap;

    auto parserGroupHeader(const QString &str) noexcept;
    ParseError parseEntry(const QString &str, decltype(m_entryMap)::iterator &currentGroup) noexcept;
};

struct DesktopFile
{
    const QString filePath;
    const QString desktopId;

    static std::optional<DesktopFile> searchDesktopFile(const QString &desktopFilePath, ParseError& err) noexcept;

private:
    DesktopFile() = default;
};

QDebug operator<<(QDebug debug, const DesktopEntry::Value& v);
