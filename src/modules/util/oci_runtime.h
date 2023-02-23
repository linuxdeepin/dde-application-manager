// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LINGLONG_BOX_SRC_UTIL_OCI_RUNTIME_H_
#define LINGLONG_BOX_SRC_UTIL_OCI_RUNTIME_H_

#include <sys/mount.h>

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

#include <QVariant>
#include <QVector>
#include <QMap>

#include "util.h"

namespace linglong {

#undef linux

struct Root {
    QString path;
    // 删除 std::optional 和 宏定义
    bool readonly;
};

inline void fromJson(const QByteArray &array, Root &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson root failed";
        return;
    }

    QJsonObject obj = doc.object();
    // readonly 可以不存在
    if (!obj.contains("path")) {
        return;
    }

    o.path = obj.value("path").toString();
    if (obj.contains("readonly")) {
        o.readonly = obj.value("readonly").toBool();
    }
}

inline void toJson(QByteArray &array, const Root &o)
{
    QJsonObject obj {
        {"path", o.path},
        {"readonly", o.readonly}
    };

    array = QJsonDocument(obj).toJson();
}

struct Process {
    QList<QString> args;
    QList<QString> env;
    QString cwd;
};

inline void fromJson(const QByteArray &array, Process &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson process failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("args") || !obj.contains("env") || !obj.contains("cwd")) {
        std::cout << "process json invalid format" << std::endl;
        return;
    }

    for (auto arg : obj.value("args").toArray()) {
        o.args.append(arg.toString());
    }
    for (auto env : obj.value("env").toArray()) {
        o.env.append(env.toString());
    }
    o.cwd = obj.value("cwd").toString();
}


inline void toJson(QByteArray &array, const Process &o)
{
    QJsonArray argsArray;
    for (auto arg : o.args) {
        argsArray.append(arg);
    }
    QJsonArray envsArray;
    for (auto env : o.env) {
        envsArray.append(env);
    }

    QJsonObject obj = {
        {"args", argsArray},
        {"env", envsArray},
        {"cwd", o.cwd}
    };

    array = QJsonDocument(obj).toJson();
}

struct Mount {
    enum Type {
        Unknown,
        Bind,
        Proc,
        Sysfs,
        Devpts,
        Mqueue,
        Tmpfs,
        Cgroup,
        Cgroup2,
    };
    QString destination;
    QString type;
    QString source;
    QList<QString> data;

    Type fsType;
    uint32_t flags = 0u;
};

inline void fromJson(const QByteArray &array, Mount &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson mount failed";
        return;
    }

    QJsonObject obj = doc.object();
    static QMap<QString, Mount::Type> fsTypes = {
        {"bind", Mount::Bind},   {"proc", Mount::Proc},   {"devpts", Mount::Devpts}, {"mqueue", Mount::Mqueue},
        {"tmpfs", Mount::Tmpfs}, {"sysfs", Mount::Sysfs}, {"cgroup", Mount::Cgroup}, {"cgroup2", Mount::Cgroup2},
    };

    struct mountFlag {
        bool clear;
        uint32_t flag;
    };

    static QMap<QVariant, mountFlag> optionFlags = {
        {"acl", {false, MS_POSIXACL}},
        {"async", {true, MS_SYNCHRONOUS}},
        {"atime", {true, MS_NOATIME}},
        {"bind", {false, MS_BIND}},
        {"defaults", {false, 0}},
        {"dev", {true, MS_NODEV}},
        {"diratime", {true, MS_NODIRATIME}},
        {"dirsync", {false, MS_DIRSYNC}},
        {"exec", {true, MS_NOEXEC}},
        {"iversion", {false, MS_I_VERSION}},
        {"lazytime", {false, MS_LAZYTIME}},
        {"loud", {true, MS_SILENT}},
        {"mand", {false, MS_MANDLOCK}},
        {"noacl", {true, MS_POSIXACL}},
        {"noatime", {false, MS_NOATIME}},
        {"nodev", {false, MS_NODEV}},
        {"nodiratime", {false, MS_NODIRATIME}},
        {"noexec", {false, MS_NOEXEC}},
        {"noiversion", {true, MS_I_VERSION}},
        {"nolazytime", {true, MS_LAZYTIME}},
        {"nomand", {true, MS_MANDLOCK}},
        {"norelatime", {true, MS_RELATIME}},
        {"nostrictatime", {true, MS_STRICTATIME}},
        {"nosuid", {false, MS_NOSUID}},
        // {"nosymfollow",{false, MS_NOSYMFOLLOW}}, // since kernel 5.10
        {"rbind", {false, MS_BIND | MS_REC}},
        {"relatime", {false, MS_RELATIME}},
        {"remount", {false, MS_REMOUNT}},
        {"ro", {false, MS_RDONLY}},
        {"rw", {true, MS_RDONLY}},
        {"silent", {false, MS_SILENT}},
        {"strictatime", {false, MS_STRICTATIME}},
        {"suid", {true, MS_NOSUID}},
        {"sync", {false, MS_SYNCHRONOUS}},
        // {"symfollow",{true, MS_NOSYMFOLLOW}}, // since kernel 5.10
    };

    if (!obj.contains("destination") || !obj.contains("type") || !obj.contains("source") \
        || !obj.contains("options")) {
        std::cout << "mount json invalid format" << std::endl;
        return;
    }

    o.destination = obj.value("destination").toString();
    o.type = obj.value("type").toString();
    o.fsType = fsTypes.find(o.type).value();
    if (o.fsType == Mount::Bind) {
        o.flags = MS_BIND;
    }
    o.source = obj.value("source").toString();
    o.data = {};

    // Parse options to data and flags.
    // FIXME: support "propagation flags" and "recursive mount attrs"
    // https://github.com/opencontainers/runc/blob/c83abc503de7e8b3017276e92e7510064eee02a8/libcontainer/specconv/spec_linux.go#L958
    auto options = obj.value("options").toArray().toVariantList();
    for (auto const &opt : options) {
        auto it = optionFlags.find(opt);
        if (it != optionFlags.end()) {
            if (it.value().clear) {
                o.flags &= ~it.value().flag;
            } else
                o.flags |= it.value().flag;
        } else {
            o.data.push_back(opt.toString());
        }
    }
}

inline void toJson(QByteArray &array, const Mount &o)
{
    QJsonArray dataArray;
    for (auto const &data : o.data) {
        dataArray.append(data);
    }

    QJsonObject obj = {
        {"destination", o.destination},
        {"source", o.source},
        {"type", o.type},
        {"options", dataArray},// FIXME: this data is not original options, some of them have been prased to flags.
    };

    array = QJsonDocument(obj).toJson();
}

struct Namespace {
    int type;
};

static std::map<std::string, int> namespaceType = {
    {"pid", CLONE_NEWPID},     {"uts", CLONE_NEWUTS}, {"mount", CLONE_NEWNS},  {"cgroup", CLONE_NEWCGROUP},
    {"network", CLONE_NEWNET}, {"ipc", CLONE_NEWIPC}, {"user", CLONE_NEWUSER},
};

inline void fromJson(const QByteArray &array, Namespace &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson namespace failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("type")) {
        std::cout << "namespace json invalid format" << std::endl;
        return;
    }

    o.type = namespaceType.find(obj.value("type").toString().toStdString())->second;
}

inline void toJson(QByteArray &array, const Namespace &o)
{
    auto matchPair = std::find_if(std::begin(namespaceType), std::end(namespaceType),
                                  [&](const std::pair<std::string, int> &pair) { return pair.second == o.type; });

    QJsonObject obj = {
        {"type", matchPair->second},
    };

    array = QJsonDocument(obj).toJson();
}

struct IDMap {
    uint64_t containerID = 0u;
    uint64_t hostID = 0u;
    uint64_t size = 0u;
};

inline void fromJson(const QByteArray &array, IDMap &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson idmap failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("hostID") || !obj.contains("containerID") || !obj.contains("size")) {
        std::cout << "idmap json invalid format" << std::endl;
        return;
    }

    o.hostID = obj.value("hostID").toVariant().ULongLong;
    o.containerID = obj.value("containerID").toVariant().ULongLong;
    o.size = obj.value("size").toVariant().ULongLong;
}

inline void toJson(QByteArray &array, const IDMap &o)
{
    QJsonObject obj = {
        {"hostID", QString::number(o.hostID, 10)},
        {"containerID", QString::number(o.containerID, 10)},
        {"size", QString::number(o.size, 10)},
    };

    array = QJsonDocument(obj).toJson();
}

typedef QString SeccompAction;
typedef QString SeccompArch;

struct SyscallArg {
    u_int index; // require
    u_int64_t value; // require
    u_int64_t valueTwo; // optional
    QString op; // require
};

inline void fromJson(const QByteArray &array, SyscallArg &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson SyscallArg failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("index") || !obj.contains("value") \
        || !obj.contains("valueTwo") || !obj.contains("op")) {
        qWarning() << "syscallarg json invalid format";
        return;
    }

    o.index = obj.value("index").toVariant().UInt;
    o.value = obj.value("value").toVariant().ULongLong;
    o.valueTwo = obj.value("valueTwo").toVariant().ULongLong;
    o.op = obj.value("op").toString();
}

inline void toJson(QByteArray &array, const SyscallArg &o)
{
    QJsonObject obj = {
        {"index", QString::number(o.index, 10)},
        {"value", QString::number(o.value, 10)},
        {"valueTwo", QString::number(o.valueTwo, 10)},
        {"op", o.op},
    };

    array = QJsonDocument(obj).toJson();
}

struct Syscall {
    QList<QString> names;
    SeccompAction action;
    QList<SyscallArg> args;
};

inline void fromJson(const QByteArray &array, Syscall &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson Syscall failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("names") || !obj.contains("action") || !obj.contains("args")) {
        std::cout << "syscall json invalid format" << std::endl;
        return;
    }

    o.action = obj.value("action").toString();
    for (auto const &name : obj.value("names").toArray()) {
        o.names.append(name.toString());
    }

    QJsonValue argsValue = obj.take("args");
    SyscallArg tmpArgs;
    for (const auto &arg : argsValue.toArray()) {
        fromJson(QJsonDocument(arg.toObject()).toJson(), tmpArgs);
        o.args.append(tmpArgs);
    }
}

inline void toJson(QByteArray &array, const Syscall &o)
{
    QByteArray argsArray;
    QJsonArray argsJsonArray;
    for (const auto &arg : o.args) {
        toJson(argsArray, arg);
        argsJsonArray.append(QJsonDocument::fromJson(argsArray).array());
    }

    QByteArray namesArray;
    QJsonArray namesJsonArray;
    for (const auto &name : o.names) {
        namesJsonArray.append(name);
    }

    QJsonObject obj = {
      { "names", namesJsonArray },
      { "action", o.action },
      { "args", argsJsonArray },
    };

    array = QJsonDocument(obj).toJson();
}

struct Seccomp {
    SeccompAction defaultAction = "INVALID_ACTION";
    QList<QString> architectures;
    QList<Syscall> syscalls;
};

inline void fromJson(const QByteArray &array, Seccomp &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson Seccomp failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("defaultAction") || !obj.contains("architectures") \
        || !obj.contains("syscalls")) {
        qWarning() << "seccomp json invalid format";
        return;
    }

    o.defaultAction = obj.value("defaultAction").toString();
    for (auto const &arch : obj.value("architectures").toArray()) {
        o.architectures.append(arch.toString());
    }

    Syscall syscallsArray;
    for (auto const &syscall : obj.value("syscalls").toArray()) {
        fromJson(QJsonDocument(syscall.toObject()).toJson(), syscallsArray);
        o.syscalls.append(syscallsArray);
    }
}

inline void toJson(QByteArray &array, const Seccomp &o)
{
    QJsonArray syscallsJsonArray;
    QByteArray syscallsByteArray;
    for (auto const &syscall : o.syscalls) {
        toJson(syscallsByteArray, syscall);
        syscallsJsonArray.append(QJsonDocument::fromJson(syscallsByteArray).array());
    }

    QJsonArray archArray;
    for (auto const &arch : o.architectures) {
        archArray.append(arch);
    }

    QJsonObject obj = {
      { "defaultAction", o.defaultAction },
      { "architectures", archArray },
      { "syscalls", syscallsJsonArray },
    };

    array = QJsonDocument(obj).toJson();
}

// https://github.com/containers/crun/blob/main/crun.1.md#memory-controller
struct ResourceMemory {
    int64_t limit = -1;
    int64_t reservation = -1;
    int64_t swap = -1;
};

inline void fromJson(const QByteArray &array, ResourceMemory &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson ResourceMemory failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("limit") || !obj.contains("reservation") || !obj.contains("swap")) {
        qWarning() << "resourceMemory json invalid format";
        return;
    }

    o.limit = obj.value("limit").toVariant().toLongLong();
    o.reservation = obj.value("reservation").toVariant().toLongLong();
    o.swap = obj.value("swap").toVariant().toLongLong();
}

inline void toJson(QByteArray &array, const ResourceMemory &o)
{
    QJsonObject obj = {
        {"limit", static_cast<qint64>(o.limit)},
        {"reservation", static_cast<qint64>(o.reservation)},
        {"swap", static_cast<qint64>(o.swap)},
    };

    array = QJsonDocument(obj).toJson();
}

// https://github.com/containers/crun/blob/main/crun.1.md#cpu-controller
// support v1 and v2 with conversion
struct ResourceCPU {
    u_int64_t shares = 1024;
    int64_t quota = 100000;
    u_int64_t period = 100000;
    //    int64_t realtimeRuntime;
    //    int64_t realtimePeriod;
    //    std::string cpus;
    //    std::string mems;
};

inline void fromJson(const QByteArray &array, ResourceCPU &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson ResourceCPU failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("shares") || !obj.contains("quota") || !obj.contains("period")) {
        qWarning() << "resourcecpu json invalid format";
        return;
    }

    o.shares = obj.value("shares").toVariant().toULongLong();
    o.quota = obj.value("quota").toVariant().toLongLong();
    o.period = obj.value("period").toVariant().toULongLong();
}

inline void toJson(QByteArray &array, const ResourceCPU &o)
{
    QJsonObject obj = {
        {"shares", QString::number(o.shares, 10)},
        {"quota", static_cast<qint64>(o.quota)},
        {"period", QString::number(o.period, 10)},
    };

    array = QJsonDocument(obj).toJson();
}

struct Resources {
    ResourceMemory memory;
    ResourceCPU cpu;
};

inline void fromJson(const QByteArray &array, Resources &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson Resources failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("cpu") || !obj.contains("memory")) {
        qWarning() << "resources json invalid format";
        return;
    }

    fromJson(QJsonDocument(obj.value("cpu").toObject()).toJson(), o.cpu);
    fromJson(QJsonDocument(obj.value("memory").toObject()).toJson(), o.cpu);
}

inline void toJson(QByteArray &array, const Resources &o)
{
    QByteArray cpuArray;
    toJson(cpuArray, o.cpu);
    QByteArray memoryArray;
    toJson(memoryArray, o.memory);
    QJsonObject obj = {
        {"cpu", QJsonDocument::fromJson(cpuArray).array()},
        {"memory", QJsonDocument::fromJson(memoryArray).array()},
    };

    array = QJsonDocument(obj).toJson();
}

struct Linux {
    QList<Namespace> namespaces;
    QList<IDMap> uidMappings;
    QList<IDMap> gidMappings;
    Seccomp seccomp;
    QString cgroupsPath;
    Resources resources;
};

inline void fromJson(const QByteArray &array, Linux &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson Linux failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("namespaces") || !obj.contains("uidMappings") || !obj.contains("gidMappings") \
        || !obj.contains("cgroupsPath") || !obj.contains("resources")) {
        qWarning() << "linux json invalid format";
        return;
    }

    Namespace nameArray;
    for (auto name : obj.take("namespaces").toArray()) {
        fromJson(QJsonDocument(name.toObject()).toJson(), nameArray);
        o.namespaces.append(nameArray);
    }

    {
        IDMap uidMap;
        for (auto uid : obj.take("uidMappings").toArray()) {
            fromJson(QJsonDocument(uid.toObject()).toJson(), uidMap);
            o.uidMappings.append(uidMap);
        }
    }

    {
        IDMap gidMap;
        for (auto gid : obj.take("gidMappings").toArray()) {
            fromJson(QJsonDocument(gid.toObject()).toJson(), gidMap);
            o.gidMappings.append(gidMap);
        }
    }

    o.cgroupsPath = obj.value("cgroupsPath").toString();
    fromJson(QJsonDocument(obj.value("resources").toObject()).toJson(), o.resources);

    if (obj.contains("seccomp")) {
        fromJson(QJsonDocument(obj.value("seccomp").toObject()).toJson(), o.seccomp);
    }
}

inline void toJson(QByteArray &array, const Linux &o)
{
    QJsonArray nameSpacesJsonArray;
    QByteArray nameSpacesByteArray;
    for (auto const &nameSpace : o.namespaces) {
        toJson(nameSpacesByteArray, nameSpace);
        nameSpacesJsonArray.append(QJsonDocument::fromJson(nameSpacesByteArray).array());
    }

    QJsonArray uidMapsJsonArray;
    QByteArray uidMapsByteArray;
    for (auto const &uid : o.uidMappings) {
        toJson(uidMapsByteArray, uid);
        uidMapsJsonArray.append(QJsonDocument::fromJson(uidMapsByteArray).array());
    }

    QJsonArray gidMapsJsonArray;
    QByteArray gidMapsByteArray;
    for (auto const &gid : o.gidMappings) {
        toJson(gidMapsByteArray, gid);
        gidMapsJsonArray.append(QJsonDocument::fromJson(gidMapsByteArray).array());
    }

    QByteArray seccompArray;
    toJson(seccompArray, o.seccomp);

    QByteArray resourcesArray;
    toJson(resourcesArray, o.resources);

    QJsonObject obj = {
        {"namespaces", nameSpacesJsonArray},
        {"uidMappings", uidMapsJsonArray},
        {"gidMappings", gidMapsJsonArray},
        {"seccomp", QJsonDocument::fromJson(seccompArray).array()},
        {"cgroupsPath", o.cgroupsPath},
        {"resources", QJsonDocument::fromJson(resourcesArray).array()},
    };

    array = QJsonDocument(obj).toJson();
}

/*
    "hooks": {
        "prestart": [
            {
                "path": "/usr/bin/fix-mounts",
                "args": ["fix-mounts", "arg1", "arg2"],
                "env":  [ "key1=value1"]
            },
            {
                "path": "/usr/bin/setup-network"
            }
        ],
        "poststart": [
            {
                "path": "/usr/bin/notify-start",
                "timeout": 5
            }
        ],
        "poststop": [
            {
                "path": "/usr/sbin/cleanup.sh",
                "args": ["cleanup.sh", "-f"]
            }
        ]
    }
 */

struct Hook {
    QString path;
    QList<QString> args;
    QList<QString> env;
};

inline void fromJson(const QByteArray &array, Hook &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson Hook failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("path")) {
        qWarning() << "hook json invalid format";
        return;
    }

    o.path = obj.value("path").toString();

    if (obj.contains("args")) {
        for (auto const &arg : obj.value("args").toArray()) {
            o.args.append(arg.toString());
        }
    }

    if (obj.contains("env")) {
        for (auto const &env : obj.value("env").toArray()) {
            o.env.append(env.toString());
        }
    }
}

inline void toJson(QByteArray &array, const Hook &o)
{
    QJsonArray argsArray;
    for (auto const &arg : o.args) {
        argsArray.append(arg);
    }

    QJsonArray envArray;
    for (auto const &env : o.env) {
        envArray.append(env);
    }

    QJsonObject obj = {
        {"path", o.path},
        {"args", argsArray},
        {"env", envArray},
    };

    array = QJsonDocument(obj).toJson();
}

struct Hooks {
    QList<Hook> prestart;
    QList<Hook> poststart;
    QList<Hook> poststop;
};

inline void fromJson(const QByteArray &array, Hooks &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson Hooks failed";
        return;
    }

    QJsonObject obj = doc.object();
    Hook hook;
    if (obj.contains("prestart")) {
        for (auto prestart : obj.take("prestart").toArray()) {
            fromJson(QJsonDocument(prestart.toObject()).toJson(), hook);
            o.prestart.append(hook);
        }
    }

    if (obj.contains("poststart")) {
        for (auto poststart : obj.take("poststart").toArray()) {
            fromJson(QJsonDocument(poststart.toObject()).toJson(), hook);
            o.poststart.append(hook);
        }
    }

    if (obj.contains("poststop")) {
        for (auto poststop : obj.take("poststop").toArray()) {
            fromJson(QJsonDocument(poststop.toObject()).toJson(), hook);
            o.poststop.append(hook);
        }
    }
}

inline void toJson(QByteArray &array, const Hooks &o)
{
    QJsonArray poststartJsonArray;
    QByteArray poststartByteArray;
    for (auto const &poststart : o.poststart) {
        toJson(poststartByteArray, poststart);
        poststartJsonArray.append(QJsonDocument::fromJson(poststartByteArray).array());
    }

    QJsonArray poststopJsonArray;
    QByteArray poststopByteArray;
    for (auto const &poststop : o.poststop) {
        toJson(poststopByteArray, poststop);
        poststopJsonArray.append(QJsonDocument::fromJson(poststopByteArray).array());
    }

    QJsonArray prestartJsonArray;
    QByteArray prestartByteArray;
    for (auto const &prestart : o.prestart) {
        toJson(prestartByteArray, prestart);
        prestartJsonArray.append(QJsonDocument::fromJson(prestartByteArray).array());
    }

    QJsonObject obj = {
        {"prestart", prestartJsonArray},
        {"poststart", poststartJsonArray},
        {"poststop", poststopJsonArray},
    };

    array = QJsonDocument(obj).toJson();
}

struct AnnotationsOverlayfs {
    QString lower_parent;
    QString upper;
    QString workdir;
    QList<Mount> mounts;
};

inline void fromJson(const QByteArray &array, AnnotationsOverlayfs &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson AnnotationsOverlayfs failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("lower_parent") || !obj.contains("upper") \
        || !obj.contains("workdir") || !obj.contains("mounts")) {
        qWarning() << "annotationsOverlayfs json invalid format";
        return;
    }

    o.lower_parent = obj.value("lower_parent").toString();
    o.upper = obj.value("upper").toString();
    o.workdir = obj.value("workdir").toString();

    Mount mount;
    for (auto mountArray : obj.take("mounts").toArray()) {
        fromJson(QJsonDocument(mountArray.toObject()).toJson(), mount);
        o.mounts.append(mount);
    }
}


inline void toJson(QByteArray &array, const AnnotationsOverlayfs &o)
{
    QJsonArray MountsJsonArray;
    QByteArray MountsByteArray;
    for (auto const &mount : o.mounts) {
        toJson(MountsByteArray, mount);
        MountsJsonArray.append(QJsonDocument::fromJson(MountsByteArray).array());
    }

    QJsonObject obj = {
        {"lower_parent", o.lower_parent},
        {"upper", o.upper},
        {"workdir", o.workdir},
        {"mounts", MountsJsonArray},
    };

    array = QJsonDocument(obj).toJson();
}

struct AnnotationsNativeRootfs {
    QList<Mount> mounts;
};

inline void fromJson(const QByteArray &array, AnnotationsNativeRootfs &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson AnnotationsNativeRootfs failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("mounts")) {
        qWarning() << "annotationsNativeRootfs json invalid format";
        return;
    }

    Mount mount;
    for (auto uid : obj.take("mounts").toArray()) {
        fromJson(QJsonDocument(uid.toObject()).toJson(), mount);
        o.mounts.append(mount);
    }
}

inline void toJson(QByteArray &array, const AnnotationsNativeRootfs &o)
{
    QJsonArray mountsJsonArray;
    QByteArray mountsByteArray;
    for (auto const &mount : o.mounts) {
        toJson(mountsByteArray, mount);
        mountsJsonArray.append(QJsonDocument::fromJson(mountsByteArray).array());
    }

    QJsonObject obj = {
        {"mounts", mountsJsonArray},
    };

    array = QJsonDocument(obj).toJson();
}

struct Annotations {
    QString container_root_path;
    AnnotationsOverlayfs overlayfs;
    AnnotationsNativeRootfs native;
};

inline void fromJson(const QByteArray &array, Annotations &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson Linux failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("container_root_path")) {
        qWarning() << "annotations json invalid format";
        return;
    }

    o.container_root_path = obj.value("container_root_path").toString();
    if (obj.contains("overlayfs")) {
        fromJson(QJsonDocument(obj.value("overlayfs").toObject()).toJson(), o.overlayfs);
    }
    if (obj.contains("native")) {
        fromJson(QJsonDocument(obj.value("native").toObject()).toJson(), o.native);
    }
}

inline void toJson(QByteArray &array, const Annotations &o)
{
    QByteArray overlayfsArray;
    toJson(overlayfsArray, o.overlayfs);
    QByteArray nativeArray;
    toJson(nativeArray, o.native);

    QJsonObject obj = {
        {"overlayfs", QJsonDocument::fromJson(overlayfsArray).array()},
        {"native", QJsonDocument::fromJson(nativeArray).array()},
        {"container_root_path", o.container_root_path}
    };

    array = QJsonDocument(obj).toJson();
}

struct Runtime {
    QString version;
    Root root;
    Process process;
    QString hostname;
    Linux linux;
    QList<Mount> mounts;
    Hooks hooks;
    Annotations annotations;
};

inline void fromJson(const QByteArray &array, Runtime &o)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson Runtime failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("version") || !obj.contains("root") || !obj.contains("process") \
        || !obj.contains("hostname") || !obj.contains("linux")) {
        qWarning() << "runtime json invalid format";
        return;
    }

    o.version = obj.value("version").toString();
    o.hostname = obj.value("hostname").toString();
    fromJson(QJsonDocument(obj.value("root").toObject()).toJson(), o.root);
    fromJson(QJsonDocument(obj.value("process").toObject()).toJson(), o.process);
    fromJson(QJsonDocument(obj.value("linux").toObject()).toJson(), o.linux);

    // mounts hooks annotations 可不存在
    if (!obj.contains("mounts") || !obj.contains("hooks") || !obj.contains("annotations")) {
        return;
    }
    fromJson(QJsonDocument(obj.value("hooks").toObject()).toJson(), o.hooks);
    fromJson(QJsonDocument(obj.value("annotations").toObject()).toJson(), o.annotations);
    Mount mount;
    for (auto uid : obj.take("mounts").toArray()) {
        fromJson(QJsonDocument(uid.toObject()).toJson(), mount);
        o.mounts.append(mount);
    }
}

inline void toJson(QByteArray &array, const Runtime &o)
{
    QByteArray rootArray;
    QByteArray processArray;
    QByteArray linuxArray;
    QByteArray hooksArray;
    QByteArray annotationsArray;
    toJson(rootArray, o.root);
    toJson(processArray, o.process);
    toJson(linuxArray, o.linux);
    toJson(hooksArray, o.hooks);
    toJson(annotationsArray, o.annotations);

    QJsonArray MountsJsonArray;
    QByteArray MountsByteArray;
    for (auto const &mount : o.mounts) {
        toJson(MountsByteArray, mount);
        MountsJsonArray.append(QJsonDocument::fromJson(MountsByteArray).array());
    }

    QJsonObject obj = {
        {"version", o.version},
        {"root", QJsonDocument::fromJson(rootArray).array()},
        {"process", QJsonDocument::fromJson(processArray).array()},
        {"hostname", o.hostname},
        {"linux", QJsonDocument::fromJson(linuxArray).array()},
        {"hooks", QJsonDocument::fromJson(hooksArray).array()},
        {"annotations", QJsonDocument::fromJson(annotationsArray).array()},
        {"mounts", MountsJsonArray},
    };

    array = QJsonDocument(obj).toJson();
}

} // namespace linglong

#endif /* LINGLONG_BOX_SRC_UTIL_OCI_RUNTIME_H_ */
