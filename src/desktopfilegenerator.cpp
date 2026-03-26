// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "desktopfilegenerator.h"
#include "desktopfileparser.h"

Q_LOGGING_CATEGORY(DDEAMGen, "dde.am.desktopfile.generator")

using namespace Qt::StringLiterals;

bool DesktopFileGenerator::checkValidation(const QVariantMap &desktopFile, QString &err) noexcept
{
    if (!desktopFile.contains(u"Type"_s) || !desktopFile.contains(u"Name"_s)) {
        err = "required key doesn't exists.";
        return false;
    }

    auto it = desktopFile.constFind(u"Type"_s);
    if (it == desktopFile.cend()) {
        err = "Type key doesn't exists.";
        return false;
    }

    auto type = it->toString();
    if (type.isEmpty()) {
        err = "Type's type is invalid";
        return false;
    }

    if (type == u"Link"_s && !desktopFile.contains(u"URL"_s)) {
        err = "URL must be set when Type is 'Link'";
        return false;
    }
    return true;
}

int DesktopFileGenerator::processMainGroupLocaleEntry(DesktopEntry::container_type::iterator mainEntry,
                                                      const QString &key,
                                                      const QVariant &value) noexcept
{
    using namespace Qt::StringLiterals;
    if (key == u"ActionName"_s) {
        return 1;
    }

    if (key == u"Name"_s) {
        const auto &nameMap = qdbus_cast<QStringMap>(value);
        if (nameMap.isEmpty()) {
            qDebug() << "Name's type mismatch:" << nameMap;
            return -1;
        }

        mainEntry->insert(u"Name"_s, QVariant::fromValue(nameMap));
        return 1;
    }

    if (key == u"GenericName"_s) {
        const auto &genericNameMap = qdbus_cast<QStringMap>(value);
        if (genericNameMap.isEmpty()) {
            qDebug() << "GenericName's type mismatch:" << genericNameMap;
            return 1;
        }

        mainEntry->insert(u"GenericName"_s, QVariant::fromValue(genericNameMap));
        return 1;
    }

    if (key == u"Icon"_s) {
        const auto &iconMap = qdbus_cast<QStringMap>(value);
        if (auto icon = iconMap.constFind(fromStaticRaw(DesktopFileDefaultKeyLocale));
            icon != iconMap.cend() && !icon->isEmpty()) {
            mainEntry->insert(u"Icon"_s, *icon);
        }
        return 1;
    }

    if (key == u"Exec"_s) {
        const auto &execMap = qdbus_cast<QStringMap>(value);
        if (auto exec = execMap.constFind(fromStaticRaw(DesktopFileDefaultKeyLocale));
            exec != execMap.cend() && !exec->isEmpty()) {
            mainEntry->insert(u"Exec"_s, *exec);
        }
        return 1;
    }

    return 0;
}

bool DesktopFileGenerator::processMainGroup(DesktopEntry::container_type &content, const QVariantMap &rawValue) noexcept
{
    auto mainEntry = content.insert(fromStaticRaw(DesktopFileEntryKey), {});
    for (const auto &[key, value] : rawValue.asKeyValueRange()) {
        if (mainEntry->contains(key)) {
            qDebug() << "duplicate key:" << key << ",skip";
            return false;
        }

        auto ret = processMainGroupLocaleEntry(mainEntry, key, value);
        if (ret == 1) {
            continue;
        }

        if (ret == -1) {
            return false;
        }

        mainEntry->insert(key, value);
    }

    mainEntry->insert(u"X-Deepin-CreateBy"_s, u"dde-application-manager"_s);
    return true;
}

bool DesktopFileGenerator::processActionGroup(const QVariantMap &actionName,
                                              const QStringList &actions,
                                              DesktopEntry::container_type &content,
                                              const QVariantMap &rawValue) noexcept
{
    QStringMap iconMap;
    if (auto actionIcon = rawValue.constFind(u"Icon"_s); actionIcon != rawValue.cend()) {
        iconMap = qvariant_cast<QStringMap>(*actionIcon);
        if (iconMap.isEmpty()) {
            qCDebug(DDEAMGen) << "Icon's type mismatch or empty:" << actionIcon->typeName() << "expected: QMap<QString, QString>";
            return false;
        }
    }

    QStringMap execMap;
    if (auto actionExec = rawValue.constFind(u"Exec"_s); actionExec != rawValue.cend()) {
        execMap = qvariant_cast<QStringMap>(*actionExec);
        if (execMap.isEmpty()) {
            qCDebug(DDEAMGen) << "Exec's type mismatch or empty:" << actionExec->typeName() << "expected: QMap<QString, QString>";
            return false;
        }
    }

    for (const auto &action : actions) {
        if (action.isEmpty()) {
            qCDebug(DDEAMGen) << "action's content is empty. skip";
            continue;
        }

        if (!actionName.contains(action)) {
            qCDebug(DDEAMGen) << "couldn't find actionName, current action:" << action;
            return false;
        }

        auto actionGroup = content.insert(fromStaticRaw(DesktopFileActionKey) % action, {});

        auto it = actionName.constFind(action);
        if (it == actionName.cend()) {
            qCDebug(DDEAMGen) << "couldn't find actionName, current action:" << action;
            return false;
        }
        actionGroup->insert(u"Name"_s, QVariant::fromValue(*it));

        if (auto actionIcon = iconMap.constFind(action); actionIcon != iconMap.cend() && !actionIcon->isEmpty()) {
            actionGroup->insert(u"Icon"_s, *actionIcon);
        }

        if (auto actionExec = execMap.constFind(action); actionExec != execMap.cend() && !actionExec->isEmpty()) {
            actionGroup->insert(u"Exec"_s, *actionExec);
        }
    };

    return true;
}

QString DesktopFileGenerator::generate(const QVariantMap &desktopFile, QString &err) noexcept
{
    DesktopEntry::container_type content{};
    if (auto actions = desktopFile.find(u"Actions"_s); actions != desktopFile.end()) {
        auto it = desktopFile.constFind(u"ActionName"_s);
        if (it == desktopFile.cend()) {
            err = u"'ActionName' doesn't exists"_s;
            return {};
        }

        auto nameMap = qvariant_cast<QVariantMap>(*it);
        if (nameMap.isEmpty()) {
            err = u"ActionName's type mismatch or empty:"_s % it->typeName() % u"expected: QMap<QString, QString>";
            return {};
        }

        auto actionList = actions->toStringList();
        if (actionList.isEmpty()) {
            err = u"Actions's type mismatch or empty:"_s % actions->typeName() % u"expected: QStringList";
            return {};
        }

        if (!processActionGroup(nameMap, actionList, content, desktopFile)) {
            err = u"please check action group"_s;
            return {};
        }
    }

    if (!processMainGroup(content, desktopFile)) {
        err = u"please check main group."_s;
        return {};
    }

    auto fileContent = toString(content);
    if (fileContent.isEmpty()) {
        err = u"couldn't convert to desktop file."_s;
        return {};
    }

    return fileContent;
}
