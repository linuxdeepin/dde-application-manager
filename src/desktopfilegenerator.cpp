// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"
#include "desktopfilegenerator.h"
#include "desktopfileparser.h"
#include "constant.h"

Q_LOGGING_CATEGORY(DDEAMGen, "dde.am.desktopfile.generator")

using namespace Qt::StringLiterals;

bool DesktopFileGenerator::checkValidation(const QVariantMap &desktopFile, QString &err) noexcept
{
    if (!desktopFile.contains(fromStaticRaw(DesktopEntryType)) || !desktopFile.contains(fromStaticRaw(DesktopEntryName))) {
        err = "required key doesn't exists.";
        return false;
    }

    auto it = desktopFile.constFind(fromStaticRaw(DesktopEntryType));
    if (it == desktopFile.cend()) {
        err = "Type key doesn't exists.";
        return false;
    }

    auto type = it->toString();
    if (type.isEmpty()) {
        err = "Type's type is invalid";
        return false;
    }

    if (type == fromStaticRaw(DesktopEntryLink) && !desktopFile.contains(fromStaticRaw(DesktopEntryURL))) {
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
    if (key == fromStaticRaw(DesktopEntryActionName)) {
        return 1;
    }

    if (key == fromStaticRaw(DesktopEntryName)) {
        const auto &nameMap = qdbus_cast<QStringMap>(value);
        if (nameMap.isEmpty()) {
            qDebug() << "Name's type mismatch:" << nameMap;
            return -1;
        }

        mainEntry->insert(fromStaticRaw(DesktopEntryName), QVariant::fromValue(nameMap));
        return 1;
    }

    if (key == fromStaticRaw(DesktopEntryGenericName)) {
        const auto &genericNameMap = qdbus_cast<QStringMap>(value);
        if (genericNameMap.isEmpty()) {
            qDebug() << "GenericName's type mismatch:" << genericNameMap;
            return 1;
        }

        mainEntry->insert(fromStaticRaw(DesktopEntryGenericName), QVariant::fromValue(genericNameMap));
        return 1;
    }

    if (key == fromStaticRaw(DesktopEntryIcon)) {
        const auto &iconMap = qdbus_cast<QStringMap>(value);
        if (auto icon = iconMap.constFind(fromStaticRaw(DesktopFileDefaultKeyLocale));
            icon != iconMap.cend() && !icon->isEmpty()) {
            mainEntry->insert(fromStaticRaw(DesktopEntryIcon), *icon);
        }
        return 1;
    }

    if (key == fromStaticRaw(DesktopEntryExec)) {
        const auto &execMap = qdbus_cast<QStringMap>(value);
        if (auto exec = execMap.constFind(fromStaticRaw(DesktopFileDefaultKeyLocale));
            exec != execMap.cend() && !exec->isEmpty()) {
            mainEntry->insert(fromStaticRaw(DesktopEntryExec), *exec);
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

        if (key == fromStaticRaw(DesktopEntryActions)) {
            if (value.userType() == QMetaType::QString) {
                mainEntry->insert(key, value);
                continue;
            }

            if (value.userType() == QMetaType::QStringList) {
                const auto actionList = value.toStringList();
                if (actionList.isEmpty()) {
                    qDebug() << "Actions's type mismatch or empty:" << value.typeName() << "expected: QString";
                    return false;
                }

                mainEntry->insert(key, actionList.join(u';'));
                continue;
            }

            qDebug() << "Actions's type mismatch:" << value.typeName() << "expected: QString";
            return false;
        }

        mainEntry->insert(key, value);
    }

    mainEntry->insert(fromStaticRaw(DesktopEntryXDeepinCreateBy), u"dde-application-manager"_s);
    return true;
}

bool DesktopFileGenerator::processActionGroup(const QVariantMap &actionName,
                                              const QStringList &actions,
                                              DesktopEntry::container_type &content,
                                              const QVariantMap &rawValue) noexcept
{
    QStringMap iconMap;
    if (auto actionIcon = rawValue.constFind(fromStaticRaw(DesktopEntryIcon)); actionIcon != rawValue.cend()) {
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
        actionGroup->insert(fromStaticRaw(DesktopEntryName), QVariant::fromValue(*it));

        if (auto actionIcon = iconMap.constFind(action); actionIcon != iconMap.cend() && !actionIcon->isEmpty()) {
            actionGroup->insert(fromStaticRaw(DesktopEntryIcon), *actionIcon);
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
    if (auto actions = desktopFile.find(fromStaticRaw(DesktopEntryActions)); actions != desktopFile.end()) {
        auto it = desktopFile.constFind(fromStaticRaw(DesktopEntryActionName));
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
