// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef INIPARSER_H
#define INIPARSER_H

#include <QMap>
#include <QByteArray>
#include <QByteArrayView>
#include <QDebug>
#include <QString>
#include <QStringView>
#include <QFile>

enum class ParserError : uint8_t {
    NoError,
    NotFound,
    MismatchedFile,
    InvalidLocation,
    InvalidFormat,
    OpenFailed,
    MissingInfo,
    Parsed,
    InternalError
};

template <typename Value>
class Parser
{
public:
    explicit Parser(QFile &file)
        : m_file(file) {};
    virtual ~Parser() = default;
    using Groups = QMap<QString, QMap<QString, Value>>;

    Parser(const Parser &) = delete;
    Parser(Parser &&) = delete;
    Parser &operator=(const Parser &) = delete;
    Parser &operator=(Parser &&) = delete;

    virtual ParserError parse(Groups &groups) noexcept = 0;

protected:
    virtual ParserError addGroup(Groups &groups, QString &groupName) noexcept = 0;
    virtual ParserError addEntry(typename Groups::iterator group) noexcept = 0;
    [[nodiscard]] bool atEnd() noexcept
    {
        ensureLoaded();
        return m_line.isEmpty() && m_offset >= m_content.size();
    }

    void skip() noexcept
    {
        ensureLoaded();
        while (m_offset < m_content.size()) {
            const auto lineBegin = m_offset;
            while (m_offset < m_content.size() && m_content.at(m_offset) != '\n') {
                ++m_offset;
            }

            auto lineEnd = m_offset;
            if (m_offset < m_content.size() && m_content.at(m_offset) == '\n') {
                ++m_offset;
            }

            if (lineEnd > lineBegin && m_content.at(lineEnd - 1) == '\r') {
                --lineEnd;
            }

            const auto trimmedView = QByteArrayView{m_content}.sliced(lineBegin, lineEnd - lineBegin).trimmed();

            if (trimmedView.isEmpty() || trimmedView.startsWith('#')) {
                continue;
            }

            m_line = trimmedView;
            break;
        }

        if (m_offset >= m_content.size() && m_line.isEmpty()) {
            m_line = {};
        }
    }

    void clearLine() noexcept { m_line = {}; }

private:
    void ensureLoaded() noexcept
    {
        if (m_loaded) {
            return;
        }

        m_loaded = true;
        const auto content = m_file.readAll();
        if (content.isEmpty()) {
            return;
        }

        m_content = content;
    }

protected:
    QFile &m_file;
    QByteArray m_content;
    QByteArrayView m_line;
    qsizetype m_offset{0};
    bool m_loaded{false};
};

inline bool hasNonAsciiAndControlCharacters(QStringView str) noexcept
{
    if (str.isEmpty()) {
        return false;
    }

    bool hasControl{false};
    bool hasNonAscii{false};

    for (const auto &ch : str) {
        const auto u = ch.unicode();

        if (u > 127) {
            hasNonAscii = true;
        }

        if (u <= 31 || (u >= 127 && u <= 159)) {
            hasControl = true;
        }

        if (hasNonAscii && hasControl) {
            return true;
        }
    }

    return false;
}

inline QDebug operator<<(QDebug debug, const ParserError &v)
{
    const QDebugStateSaver saver{debug};
    QString errMsg;
    switch (v) {
    case ParserError::NoError: {
        errMsg = "no error.";
    } break;
    case ParserError::NotFound: {
        errMsg = "file not found.";
    } break;
    case ParserError::MismatchedFile: {
        errMsg = "file type is mismatched.";
    } break;
    case ParserError::InvalidLocation: {
        errMsg = "file location is invalid, please check $XDG_DATA_DIRS.";
    } break;
    case ParserError::OpenFailed: {
        errMsg = "couldn't open the file.";
    } break;
    case ParserError::InvalidFormat: {
        errMsg = "the format of desktopEntry file is invalid.";
    } break;
    case ParserError::MissingInfo: {
        errMsg = "missing required infomation.";
    } break;
    case ParserError::Parsed: {
        errMsg = "this desktop entry is parsed.";
    } break;
    case ParserError::InternalError: {
        errMsg = "internal error of parser.";
    } break;
    }
    debug << errMsg;
    return debug;
}

#endif
