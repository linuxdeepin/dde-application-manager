/*
 * Copyright (C) 2022 ~ 2023 Deepin Technology Co., Ltd.
 *
 * Author:     weizhixiang <weizhixiang@uniontech.com>
 *
 * Maintainer: weizhixiang <weizhixiang@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "windowidentify.h"
#include "common.h"
#include "appinfo.h"
#include "dock.h"
#include "processinfo.h"
#include "dstring.h"
#include "basedir.h"
#include "dfile.h"
#include "xcbutils.h"

#include <QDebug>
#include <QThread>

#define XCB XCBUtils::instance()

WindowPatterns WindowIdentify::patterns;

static QMap<QString, QString> crxAppIdMap = {
    {"crx_onfalgmmmaighfmjgegnamdjmhpjpgpi", "apps.com.aiqiyi"},
    {"crx_gfhkopakpiiaeocgofdpcpjpdiglpkjl", "apps.cn.kugou.hd"},
    {"crx_gaoopbnflngfkoobibfgbhobdeiipcgh", "apps.cn.kuwo.kwmusic"},
    {"crx_jajaphleehpmpblokgighfjneejapnok", "apps.com.evernote"},
    {"crx_ebhffdbfjilfhahiinoijchmlceailfn", "apps.com.letv"},
    {"crx_almpoflgiciaanepplakjdkiaijmklld", "apps.com.tongyong.xxbox"},
    {"crx_heaphplipeblmpflpdcedfllmbehonfo", "apps.com.peashooter"},
    {"crx_dbngidmdhcooejaggjiochbafiaefndn", "apps.com.rovio.angrybirdsseasons"},
    {"crx_chfeacahlaknlmjhiagghobhkollfhip", "apps.com.sina.weibo"},
    {"crx_cpbmecbkmjjfemjiekledmejoakfkpec", "apps.com.openapp"},
    {"crx_lalomppgkdieklbppclocckjpibnlpjc", "apps.com.baidutieba"},
    {"crx_gejbkhjjmicgnhcdpgpggboldigfhgli", "apps.com.zhuishushenqi"},
    {"crx_gglenfcpioacendmikabbkecnfpanegk", "apps.com.duokan"},
    {"crx_nkmmgdfgabhefacpfdabadjfnpffhpio", "apps.com.zhihu.daily"},
    {"crx_ajkogonhhcighbinfgcgnjiadodpdicb", "apps.com.netease.newsreader"},
    {"crx_hgggjnaaklhemplabjhgpodlcnndhppo", "apps.com.baidu.music.pad"},
    {"crx_ebmgfebnlgilhandilnbmgadajhkkmob", "apps.cn.ibuka"},
    {"crx_nolebplcbgieabkblgiaacdpgehlopag", "apps.com.tianqitong"},
    {"crx_maghncnmccfbmkekccpmkjjfcmdnnlip", "apps.com.youjoy.strugglelandlord"},
    {"crx_heliimhfjgfabpgfecgdhackhelmocic", "apps.cn.emoney"},
    {"crx_jkgmneeafmgjillhgmjbaipnakfiidpm", "apps.com.instagram"},
    {"crx_cdbkhmfmikobpndfhiphdbkjklbmnakg", "apps.com.easymindmap"},
    {"crx_djflcciklfljleibeinjmjdnmenkciab", "apps.com.lgj.thunderbattle"},
    {"crx_ffdgbolnndgeflkapnmoefhjhkeilfff", "apps.com.qianlong"},
    {"crx_fmpniepgiofckbfgahajldgoelogdoap", "apps.com.windhd"},
    {"crx_dokjmallmkihbgefmladclcdcinjlnpj", "apps.com.youdao.hanyu"},
    {"crx_dicimeimfmbfcklbjdpnlmjgegcfilhm", "apps.com.ibookstar"},
    {"crx_cokkcjnpjfffianjbpjbcgjefningkjm", "apps.com.yidianzixun"},
    {"crx_ehflkacdpmeehailmcknlnkmjalehdah", "apps.com.xplane"},
    {"crx_iedokjbbjejfinokgifgecmboncmkbhb", "apps.com.wedevote"},
    {"crx_eaefcagiihjpndconigdpdmcbpcamaok", "apps.com.tongwei.blockbreaker"},
    {"crx_mkjjfibpccammnliaalefmlekiiikikj", "apps.com.dayima"},
    {"crx_gflkpppiigdigkemnjlonilmglokliol", "apps.com.cookpad"},
    {"crx_jfhpkchgedddadekfeganigbenbfaohe", "apps.com.issuu"},
    {"crx_ggkmfnbkldhmkehabgcbnmlccfbnoldo", "apps.bible.cbol"},
    {"crx_phlhkholfcljapmcidanddmhpcphlfng", "apps.com.kanjian.radio"},
    {"crx_bjgfcighhaahojkibojkdmpdihhcehfm", "apps.de.danoeh.antennapod"},
    {"crx_kldipknjommdfkifomkmcpbcnpmcnbfi", "apps.com.asoftmurmur"},
    {"crx_jfhlegimcipljdcionjbipealofoncmd", "apps.com.tencentnews"},
    {"crx_aikgmfkpmmclmpooohngmcdimgcocoaj", "apps.com.tonghuashun"},
    {"crx_ifimglalpdeoaffjmmihoofapmpflkad", "apps.com.letv.lecloud.disk"},
    {"crx_pllcekmbablpiogkinogefpdjkmgicbp", "apps.com.hwadzanebook"},
    {"crx_ohcknkkbjmgdfcejpbmhjbohnepcagkc", "apps.com.douban.radio"},
};

WindowIdentify::WindowIdentify(Dock *_dock, QObject *parent)
 : QObject(parent)
 , dock(_dock)
{
    identifyWindowFuns["Android"] = identifyWindowAndroid;
    identifyWindowFuns["PidEnv"] = identifyWindowByPidEnv;
    identifyWindowFuns["CmdlineTurboBooster"] = identifyWindowByCmdlineTurboBooster;
    identifyWindowFuns["Cmdline-XWalk"] = identifyWindowByCmdlineXWalk;
    identifyWindowFuns["FlatpakAppID"] = identifyWindowByFlatpakAppID;
    identifyWindowFuns["CrxId"] = identifyWindowByCrxId;
    identifyWindowFuns["Rule"] = identifyWindowByRule;
    identifyWindowFuns["Bamf"] = identifyWindowByBamf;
    identifyWindowFuns["Pid"] = identifyWindowByPid;
    identifyWindowFuns["Scratch"] = identifyWindowByScratch;
    identifyWindowFuns["GtkAppId"] = identifyWindowByGtkAppId;
    identifyWindowFuns["WmClass"] = identifyWindowByWmClass;

}

AppInfo *WindowIdentify::identifyWindow(WindowInfoBase *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    //qInfo() << "identifyWindow: window id " << winInfo->getXid() << " innerId " << winInfo->getInnerId();
    if (winInfo->getWindowType() == "X11")
        ret = identifyWindowX11(static_cast<WindowInfoX *>(winInfo), innerId);
    else if (winInfo->getWindowType() == "Wayland")
        ret = identifyWindowWayland(static_cast<WindowInfoK *>(winInfo), innerId);

    return ret;
}

AppInfo *WindowIdentify::identifyWindowX11(WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *appInfo = nullptr;
    if (winInfo->getInnerId() == "") {
        qInfo() << "identify WindowX11: innerId is empty";
        return appInfo;
    }

    for (auto iter = identifyWindowFuns.begin(); iter != identifyWindowFuns.end(); iter++) {
        QString name = iter.key();
        IdentifyFunc func = iter.value();
        qInfo() << "identifyWindowX11: try " << name;
        appInfo = func(dock, winInfo, innerId);
        if (appInfo) {  // TODO: if name == "Pid", appInfo may by nullptr
            // 识别成功
            qInfo() << "identify Window by " << name << " innerId " << appInfo->getInnerId() << " success!";
            AppInfo *fixedAppInfo = fixAutostartAppInfo(appInfo->getFileName());
            if (fixedAppInfo) {
                delete appInfo;
                appInfo = fixedAppInfo;
                appInfo->setIdentifyMethod(name + "+FixAutostart");
                innerId = appInfo->getInnerId();
            } else {
                appInfo->setIdentifyMethod(name);
            }
            return appInfo;
        }
    }

    qInfo() << "identifyWindowX11: failed";
    return appInfo;
}

AppInfo *WindowIdentify::identifyWindowWayland(WindowInfoK *winInfo, QString &innerId)
{
    // TODO: 对桌面调起的文管应用做规避处理，需要在此处添加，因为初始化时appId和title为空
    if (winInfo->getAppId() == "dde-desktop" && dock->shouldShowOnDock(winInfo)) {
        winInfo->setAppId("dde-file-manager");
    }

    QString appId = winInfo->getAppId();
    if (appId.isEmpty()) {
        QString title = winInfo->getTitle();
        // TODO: 对于appId为空的情况，使用title过滤，此项修改针对浏览器下载窗口

    }

    // 先使用appId获取appInfo,如果不能成功获取再使用GIO_LAUNCHED_DESKTOP_FILE环境变量获取
    AppInfo *appInfo = new AppInfo(appId);
    if (!appInfo->isValidApp() && winInfo->getProcess()) {
        ProcessInfo *process = winInfo->getProcess();
        std::string desktopFilePath = process->getEnv("GIO_LAUNCHED_DESKTOP_FILE");
        if (DString::endWith(desktopFilePath, ".desktop")) {
            appInfo = new AppInfo(desktopFilePath.c_str());
        }
    }

    // autoStart
    if (appInfo->isValidApp()) {
        AppInfo *fixedAppInfo = fixAutostartAppInfo(appInfo->getFileName());
        if (fixedAppInfo) {
            delete appInfo;
            appInfo = fixedAppInfo;
            appInfo->setIdentifyMethod("FixAutostart");
        }
    } else {
        // bamf
        XWindow winId = winInfo->getXid();
        QString desktop = dock->getDesktopFromWindowByBamf(winId);
        if (!desktop.isEmpty()) {
            appInfo = new AppInfo(desktop);
        }

        WMClass wmClass = XCB->getWMClass(winId);
        if (wmClass.instanceName.size() > 0) {
            QString instance(wmClass.instanceName.c_str());
            appInfo = new AppInfo("org.deepin.flatdeb." + instance);
            if (appInfo)
                return appInfo;

            appInfo = new AppInfo(instance);
            if (appInfo)
                return appInfo;

            if (wmClass.className.size() > 0) {
                appInfo = new AppInfo(wmClass.className.c_str());
                if (appInfo)
                    return appInfo;
            }
        }
    }

    if (appInfo)
        innerId = appInfo->getInnerId();

    return appInfo;
}

AppInfo *WindowIdentify::identifyWindowAndroid(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    int32_t androidId = getAndroidUengineId(winInfo->getXid());
    QString androidName = getAndroidUengineName(winInfo->getXid());
    if (androidId != -1 && androidName != "") {
        QString desktopPath = "/usr/share/applications/uengine." + androidName + ".desktop";
        DesktopInfo desktopInfo(desktopPath.toStdString());
        if (!desktopInfo.isValidDesktop()) {
            qInfo() << "identifyWindowAndroid: not exist DesktopFile " << desktopPath;
            return ret;
        }

        ret = new AppInfo(desktopInfo);
        ret->setIdentifyMethod("Android");
        innerId = ret->getInnerId();
    }

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByPidEnv(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    int pid = winInfo->getPid();
    auto process = winInfo->getProcess();
    qInfo() << "identifyWindowByPidEnv: pid=" << pid << " WindowId=" << winInfo->getXid();
    if (pid != 0 && process) {
        QString launchedDesktopFile = process->getEnv("GIO_LAUNCHED_DESKTOP_FILE").c_str();
        QString launchedDesktopFilePidStr = process->getEnv("GIO_LAUNCHED_DESKTOP_FILE_PID").c_str();
        int launchedDesktopFilePid = launchedDesktopFilePidStr.toInt();
        qInfo() << "launchedDesktopFilePid=" << launchedDesktopFilePid << " launchedDesktopFile=" << launchedDesktopFile;

        // 以下 2 种情况下，才能信任环境变量 GIO_LAUNCHED_DESKTOP_FILE。
        // 1. 当窗口 pid 和 launchedDesktopFilePid 相同时；
        // 2. 当窗口的进程的父进程 id（即 ppid）和 launchedDesktopFilePid 相同，
        // 并且该父进程是 sh 或 bash 时。
        bool needTry = false;
        if (pid == launchedDesktopFilePid) {
            needTry = true;
        } else if (process->getPpid() && process->getPpid() == launchedDesktopFilePid) {
            Process parentProcess(launchedDesktopFilePid);
            auto parentCmdLine = parentProcess.getCmdLine();
            if (parentCmdLine.size() > 0) {
                qInfo() << "ppid equal " << "parentCmdLine[0]:" << parentCmdLine[0].c_str();
                QString cmd0 = parentCmdLine[0].c_str();
                int pos = cmd0.lastIndexOf('/');
                if (pos > 0)
                    cmd0 = cmd0.remove(0, pos + 1);

                if (cmd0 == "sh" || cmd0 == "bash")
                    needTry = true;
            }
        }

        if (needTry) {
            ret = new AppInfo(launchedDesktopFile);
            innerId = ret->getInnerId();
        }
    }

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByCmdlineTurboBooster(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    int pid = winInfo->getPid();
    ProcessInfo *process = winInfo->getProcess();
    if (pid != 0 && process) {
        auto cmdline = process->getCmdLine();
        if (cmdline.size() > 0) {
            QString desktopFile;
            if (DString::endWith(cmdline[0], ".desktop")) {
                desktopFile = cmdline[0].c_str();
            } else if (QString(cmdline[0].c_str()).contains("/applications/")) {
                QFileInfo fileInfo(cmdline[0].c_str());
                QString path = fileInfo.path();
                QString base = fileInfo.baseName();
                QDir dir(path);
                QStringList files = dir.entryList(QDir::Files);
                for (auto f : files) {
                    if (f.contains(path + "/" + base + ".desktop")) {
                        desktopFile = f;
                        break;
                    }
                }

                qInfo() << "identifyWindowByCmdlineTurboBooster: desktopFile is " << desktopFile;
                if (!desktopFile.isEmpty()) {
                    ret = new AppInfo(desktopFile);
                    innerId = ret->getInnerId();
                }
            }
        }
    }

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByCmdlineXWalk(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    qInfo() << "identifyWindowByCmdlineXWalk: windowId=" << winInfo->getXid();
    AppInfo *ret = nullptr;
    do {
        auto process = winInfo->getProcess();
        if (!process || !winInfo->getPid())
            break;

        QString exe = process->getExe().c_str();
        QFileInfo file(exe);
        QString exeBase = file.baseName();
        auto args = process->getArgs();
        if (exe != "xwalk" || args.size() == 0)
            break;

        QString lastArg = args[args.size() - 1].c_str();
        file.setFile(lastArg);
        if (file.baseName() == "manifest.json") {
            auto strs = lastArg.split("/");
            if (strs.size() > 3 && strs[strs.size() - 2].size() > 0) {    // appId为 strs倒数第二个字符串
                ret = new AppInfo(strs[strs.size() - 2]);
                innerId = ret->getInnerId();
                break;
            }
        }

        qInfo() << "identifyWindowByCmdlineXWalk: failed";
    } while (0);

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByFlatpakAppID(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    QString flatpak = winInfo->getFlatpakAppId();
    qInfo() << "identifyWindowByFlatpakAppID: flatpak:" << flatpak;
    if (flatpak.startsWith("app/")) {
        auto parts = flatpak.split("/");
        if (parts.size() > 0) {
            QString appId = parts[1];
            ret = new AppInfo(appId);
            innerId = ret->getInnerId();
        }
    }

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByCrxId(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    WMClass wmClass = XCB->getWMClass(winInfo->getXid());
    QString className, instanceName;
    className.append(wmClass.className.c_str());
    instanceName.append(wmClass.instanceName.c_str());

    if (className.toLower() == "chromium-browser" && instanceName.toLower().startsWith("crx_")) {
        if (crxAppIdMap.find(instanceName.toLower()) != crxAppIdMap.end()) {
            QString appId = crxAppIdMap[instanceName.toLower()];
            qInfo() << "identifyWindowByCrxId: appId " << appId;
            ret = new AppInfo(appId);
            innerId = ret->getInnerId();
        }
    }

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByRule(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    qInfo() << "identifyWindowByRule: windowId=" << winInfo->getXid();
    AppInfo *ret = nullptr;
    QString matchStr = patterns.match(winInfo);
    if (matchStr.isEmpty())
        return ret;

    if (matchStr.size() > 4 && matchStr.startsWith("id=")) {
        matchStr.remove(0, 3);
        ret = new AppInfo(matchStr);
    } else if (matchStr == "env") {
        auto process = winInfo->getProcess();
        if (process) {
            QString launchedDesktopFile = process->getEnv("GIO_LAUNCHED_DESKTOP_FILE").c_str();
            if (!launchedDesktopFile.isEmpty())
                ret = new AppInfo(launchedDesktopFile);
        }
    } else {
        qInfo() << "patterns match bad result";
    }

    if (ret)
        innerId = ret->getInnerId();

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByBamf(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    XWindow xid = winInfo->getXid();
    qInfo() << "identifyWindowByBamf:  windowId=" << xid;
    QString desktopFile;
    // 重试 bamf 识别，yozo office 的窗口经常要第二次时才能识别到。
    for (int i=0; i<3; i++) {
        desktopFile = _dock->getDesktopFromWindowByBamf(xid);
        if (!desktopFile.isEmpty())
            break;

       QThread::msleep(100);
    }

    if (!desktopFile.isEmpty()) {
        ret = new AppInfo(desktopFile);
        innerId = ret->getInnerId();
    }

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByPid(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    if (winInfo->getPid() > 10) {
        auto entry = _dock->getEntryByWindowId(winInfo->getPid());
        if (entry) {
            ret = entry->getApp();
            innerId = ret->getInnerId();
        }
    }

    return ret;
}

AppInfo *WindowIdentify::identifyWindowByScratch(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    QString desktopFile = scratchDir + winInfo->getInnerId() + ".desktop";
    qInfo() << "identifyWindowByScratch: xid " << winInfo->getXid() << " desktopFile" << desktopFile;
    QFile file(desktopFile);

    if (file.exists()) {
        ret = new AppInfo(desktopFile);
        innerId = ret->getInnerId();
    }
    return ret;
}

AppInfo *WindowIdentify::identifyWindowByGtkAppId(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    QString gtkAppId = winInfo->getGtkAppId();
    if (!gtkAppId.isEmpty()) {
        ret = new AppInfo(gtkAppId);
        innerId = ret->getInnerId();
    }

    qInfo() << "identifyWindowByGtkAppId: gtkAppId:" << gtkAppId;
    return ret;
}

AppInfo *WindowIdentify::identifyWindowByWmClass(Dock *_dock, WindowInfoX *winInfo, QString &innerId)
{
    AppInfo *ret = nullptr;
    WMClass wmClass = winInfo->getWMClass();
    do {
        if (wmClass.instanceName.size() > 0) {
            // example:
            // WM_CLASS(STRING) = "Brackets", "Brackets"
            // wm class instance is Brackets
            // try app id org.deepin.flatdeb.brackets
            ret = new AppInfo("org.deepin.flatdeb." + QString(wmClass.instanceName.c_str()).toLower());
            if (ret)
                break;

            ret = new AppInfo(wmClass.instanceName.c_str());
            if (ret)
                break;
        }

        if (wmClass.className.size() > 0) {
            ret = new AppInfo(wmClass.className.c_str());
            if (ret)
                break;
        }
    } while (0);

    if (ret)
        innerId = ret->getInnerId();

    return ret;
}

AppInfo *WindowIdentify::fixAutostartAppInfo(QString fileName)
{
    QFileInfo file(fileName);
    QString filePath = file.absolutePath();
    bool isAutoStart = false;
    for (auto &dir : BaseDir::autoStartDirs()) {
        if (QString(dir.c_str()).contains(filePath)) {
            isAutoStart = true;
            break;
        }
    }

    return isAutoStart ? new AppInfo(file.baseName()) : nullptr;
}

int32_t WindowIdentify::getAndroidUengineId(XWindow winId)
{
    // TODO 获取AndroidUengineId
    return 0;
}

QString WindowIdentify::getAndroidUengineName(XWindow winId)
{
    // TODO 获取AndroidUengineName
    return "";
}












