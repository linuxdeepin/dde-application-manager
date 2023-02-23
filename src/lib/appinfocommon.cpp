// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appinfocommon.h"

const std::string AppinfoCommon::MainSection        = "Desktop Entry";
const std::string AppinfoCommon::KeyType            = "Type";
const std::string AppinfoCommon::KeyVersion         = "Version";
const std::string AppinfoCommon::KeyName            = "Name";
const std::string AppinfoCommon::KeyGenericName     = "GenericName";
const std::string AppinfoCommon::KeyNoDisplay       = "NoDisplay";
const std::string AppinfoCommon::KeyComment         = "Comment";
const std::string AppinfoCommon::KeyIcon            = "Icon";
const std::string AppinfoCommon::KeyHidden          = "Hidden";
const std::string AppinfoCommon::KeyOnlyShowIn      = "OnlyShowIn";
const std::string AppinfoCommon::KeyNotShowIn       = "NotShowIn";
const std::string AppinfoCommon::KeyTryExec         = "TryExec";
const std::string AppinfoCommon::KeyExec            = "Exec";
const std::string AppinfoCommon::KeyPath            = "Path";
const std::string AppinfoCommon::KeyTerminal        = "Terminal";
const std::string AppinfoCommon::KeyMimeType        = "MimeType";
const std::string AppinfoCommon::KeyCategories      = "Categories";
const std::string AppinfoCommon::KeyKeywords        = "Keywords";
const std::string AppinfoCommon::KeyStartupNotify   = "StartupNotify";
const std::string AppinfoCommon::KeyStartupWMClass  = "StartupWMClass";
const std::string AppinfoCommon::KeyURL             = "URL";
const std::string AppinfoCommon::KeyActions         = "Actions";
const std::string AppinfoCommon::KeyDBusActivatable = "DBusActivatable";

const std::string AppinfoCommon::TypeApplication            = "Application";
const std::string AppinfoCommon::TypeLink                   = "Link";
const std::string AppinfoCommon::TypeDirectory              = "Directory";

const std::string AppinfoCommon::EnvDesktopEnv              = "XDG_CURRENT_DESKTOP";
const std::string AppinfoCommon::DesktopExt                 = ".desktop";
const std::string AppinfoCommon::EnableInvoker              = "ENABLE_TURBO_INVOKER";
const std::string AppinfoCommon::TurboInvokerFailedMsg      = "Failed to invoke: Booster:";
const std::string AppinfoCommon::TurboInvokerErrMsg         = "deepin-turbo-invoker: error";
const std::string AppinfoCommon::SectionDefaultApps         = "Default Applications";
const std::string AppinfoCommon::SectionAddedAssociations   = "Added Associations";
const std::string AppinfoCommon::DeepinVendor               = "X-Deepin-Vendor";
const std::string AppinfoCommon::AppMimeTerminal            = "application/x-terminal";
