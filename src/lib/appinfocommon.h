// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPINFOCOMMON_H
#define APPINFOCOMMON_H
#include <string>

class AppinfoCommon
{
public:
    const static std::string    MainSection;
    const static std::string    KeyType;
    const static std::string    KeyVersion;
    const static std::string    KeyName;
    const static std::string    KeyGenericName;
    const static std::string    KeyNoDisplay;
    const static std::string    KeyComment;
    const static std::string    KeyIcon;
    const static std::string    KeyHidden;
    const static std::string    KeyOnlyShowIn;
    const static std::string    KeyNotShowIn;
    const static std::string    KeyTryExec;
    const static std::string    KeyExec;
    const static std::string    KeyPath;
    const static std::string    KeyTerminal;
    const static std::string    KeyMimeType;
    const static std::string    KeyCategories;
    const static std::string    KeyKeywords;
    const static std::string    KeyStartupNotify;
    const static std::string    KeyStartupWMClass;
    const static std::string    KeyURL;
    const static std::string    KeyActions;
    const static std::string    KeyDBusActivatable;

    const static std::string    TypeApplication;
    const static std::string    TypeLink;
    const static std::string    TypeDirectory;

    const static std::string    EnvDesktopEnv;
    const static std::string    DesktopExt;
    const static std::string    EnableInvoker;
    const static std::string    TurboInvokerFailedMsg;
    const static std::string    TurboInvokerErrMsg;
    const static std::string    SectionDefaultApps;
    const static std::string    SectionAddedAssociations;
    const static std::string    DeepinVendor ;
    const static std::string    AppMimeTerminal;
};

#endif // APPINFOCOMMON_H
