// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include <QString>
#include <QProcess>
#include <QDir>

#include "../modules/methods/basic.h"
#include "../modules/methods/instance.hpp"
#include "../modules/methods/process_status.hpp"
#include "../modules/methods/registe.hpp"
#include "../modules/methods/task.hpp"
#include "../modules/socket/client.h"
#include "../modules/tools/desktop_deconstruction.hpp"
#include "../modules/util/oci_runtime.h"

extern char** environ;

// from linglong
#define LINGLONG 118
#define LL_VAL(str) #str
#define LL_TOSTRING(str) LL_VAL(str)

struct App {
    std::string type;
    std::string prefix;
    std::string id;
};

static App parseApp(const QString& app)
{
    QStringList values = app.split('/', QString::SkipEmptyParts);
    qInfo() << "app:" << app << ", values size:" << values.size();
    App result;
    if (values.size() == 3) {
        result.prefix = values.at(0).toStdString();
        result.type   = values.at(1).toStdString();
        result.id     = values.at(2).toStdString();
    }
    return result;
}

void quit() {}

void sig_handler(int num)
{
    int   status;
    pid_t pid;
    /* 由于该信号不能叠加，所以可能同时有多个子进程已经结束 所以循环wait */
    while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status))  // 判断子进程的退出状态 是否是正常退出
            printf("-----child %d exit with %d\n", pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))  // 判断子进程是否是 通过信号退出
            printf("child %d killed by the %dth signal\n", pid, WTERMSIG(status));
    }
}

// TODO: startManager合并流程？
int childFreedesktop(Methods::Task* task, std::string path)
{
    prctl(PR_SET_PDEATHSIG, SIGKILL);
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    DesktopDeconstruction dd(path);
    dd.beginGroup("Desktop Entry");
    std::cout << dd.value<std::string>("Exec") << std::endl;

    QStringList envs;
    for (auto it = task->environments.begin(); it != task->environments.end(); ++it) {
        envs.append(it.key() + "=" + it.value());
    }

    QStringList exeArgs;
    exeArgs << QString::fromStdString(dd.value<std::string>("Exec")).split(" ");

    QString exec = exeArgs[0];
    exeArgs.removeAt(0);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork()");
        return -1;
    }

    if (pid == 0) {
        // 子进程
        QProcess process;
        qInfo() << "exec:" << exec;
        qInfo() << "exeArgs:" << exeArgs;
        process.setWorkingDirectory(QDir::homePath());
        process.setEnvironment(envs);
        process.start(exec, exeArgs);
        process.waitForFinished(-1);
        process.close();
        qInfo() << "process finish";
        exit(0);
    }
    return pid;
}

int childLinglong(Methods::Task* task, std::string path)
{
    prctl(PR_SET_PDEATHSIG, SIGKILL);
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    DesktopDeconstruction dd(path);
    dd.beginGroup("Desktop Entry");
    std::cout << dd.value<std::string>("Exec") << std::endl;

    linglong::Runtime     runtime;
    linglong::Annotations annotations;
    linglong::Root        root;
    linglong::Mount       mount;
    annotations.container_root_path = "/run/user/1000/DAM/" + task->id;
    annotations.native              = { { mount } };
    root.path                       = annotations.container_root_path + "/root";
    mount.destination               = "/";
    mount.source                    = "/";
    mount.type                      = "bind";
    mount.data                      = { "ro" };
    runtime.hostname                = "hostname";
    runtime.process.cwd             = "/";
    std::filesystem::path container_root_path(annotations.container_root_path.toStdString());
    if (!std::filesystem::exists(container_root_path)) {
        if (!std::filesystem::create_directories(container_root_path)) {
            std::cout << "[Loader] [Warning] cannot create container root path." << std::endl;
            return -1;
        }
    }

    for (auto it = task->environments.begin(); it != task->environments.end(); ++it) {
        runtime.process.env.append(it.key() + "=" + it.value());
    }

    std::istringstream stream(dd.value<std::string>("Exec"));
    std::string        s;
    while (getline(stream, s, ' ')) {
        if (s.empty()) {
            continue;
        }

        // TODO: %U
        if (s.length() == 2 && s[0] == '%') {
            continue;
        }
        runtime.process.args.push_back(QString::fromStdString(s));
    }

    // 应用运行信息
    QByteArray runtimeArray;
    toJson(runtimeArray, runtime);
    qWarning() << "runtimeArray: " << runtimeArray;

    // 使用Pipe进行父子进程通信
    int pipeEnds[2];
    if (pipe(pipeEnds) != 0) {
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork()");
        return -1;
    }

    if (pid == 0) {
        // 子进程
        (void) close(pipeEnds[1]);

        // 重定向到LINGLONG
        if (dup2(pipeEnds[0], LINGLONG) == -1) {
            return EXIT_FAILURE;
        }
        (void) close(pipeEnds[0]);

        // 初始化运行命令和参数，并执行
        char const* const args[] = { "/usr/bin/ll-box", LL_TOSTRING(LINGLONG), nullptr };
        int               ret    = execvp(args[0], (char**) args);
        std::cout << "[Loader] [Fork] " << ret << std::endl;
        //std::filesystem::remove(container_root_path);
        exit(ret);
    } else {
        // 父进程
        QByteArray runtimeArray;
        linglong::toJson(runtimeArray, runtime);
        const std::string data = runtimeArray.data();
        close(pipeEnds[0]);

        // 将运行时信息通过pipe传递给子进程
        write(pipeEnds[1], data.c_str(), data.size());
        close(pipeEnds[1]);
    }

    return pid;
}

int childAndroid(Methods::Task* task, std::string path)
{
    // TODO
    return 0;
}

#define DAM_TASK_HASH "DAM_TASK_HASH"
#define DAM_TASK_TYPE "DAM_TASK_TYPE"

int main(int argc, char* argv[])
{
    const char* dam_task_hash = getenv(DAM_TASK_HASH);
    if (!dam_task_hash) {
        return -1;
    }
    const char* dam_task_type = getenv(DAM_TASK_TYPE);
    if (!dam_task_type) {
        return -2;
    }

    char socketPath[50];
    sprintf(socketPath, "/run/user/%d/dde-application-manager.socket", getuid());

    // register client and run quitConnect
    Socket::Client client;
    client.connect(socketPath);

    // 初始化应用注册信息
    QByteArray registerArray;
    Methods::Registe registe;
    registe.id = dam_task_type;
    registe.hash = dam_task_hash;
    Methods::toJson(registerArray, registe);

    // 向AM注册应用信息进行校验
    Methods::Registe registe_result;
    registe_result.state = false;
    QByteArray result = client.get(registerArray);
    if (!result.isEmpty()) {
        Methods::fromJson(result, registe_result);
    }
    if (!registe_result.state) {
        return -3;
    }

    // 初始化应用实例信息
    Methods::Instance instance;
    instance.hash = registe_result.hash;
    QByteArray instanceArray;
    Methods::toJson(instanceArray, instance);

    // 向AM注册应用实例信息进行校验
    result = client.get(instanceArray);
    Methods::Task task;
    Methods::fromJson(result, task);       // fromJson TODO 数据解析异常
    qInfo() << "[Task] " << result;

    // 校验task内容
    App app = parseApp(task.runId);
    qInfo() << "[App] " 
        << "prefix:" << QString::fromStdString(app.prefix) 
        << "type:" << QString::fromStdString(app.type) 
        << "id:" << QString::fromStdString(app.id);
    if (task.id.isEmpty() || app.id.empty() || app.type.empty() || app.prefix.empty()) {
        std::cout << "get task error" << std::endl;
        return -4;
    }
    if (app.prefix != "freedesktop"
        && app.prefix != "linglong"
        && app.prefix != "android") {
        qWarning() << "error app prefix :" << QString::fromStdString(app.type);
        return -1;
    }

    pthread_attr_t attr;
    size_t stack_size;
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr, &stack_size);
    pthread_attr_destroy(&attr);

    /* 先将SIGCHLD信号阻塞 保证在子进程结束前设置父进程的捕捉函数 */
    sigset_t nmask, omask;
    // sigemptyset(&nmask);
    // sigaddset(&nmask, SIGCHLD);
    // sigprocmask(SIG_BLOCK, &nmask, &omask);

    //char* stack = (char*) malloc(stack_size);
    //pid_t pid   = clone(child, stack + stack_size, CLONE_NEWPID | SIGCHLD, static_cast<void*>(&task));
    pid_t pid = -1;
    if (app.prefix == "freedesktop") {
        pid = childFreedesktop(&task, task.filePath.toStdString());
    } else if (app.prefix == "linglong") {
        pid = childLinglong(&task, task.filePath.toStdString());
    } else if (app.prefix == "android") {
        pid = childAndroid(&task, task.filePath.toStdString());
    } else {
        qWarning() << "error app prefix:" << QString::fromStdString(app.prefix);
    }

    if(pid != -1) {
        Methods::ProcessStatus processSuccess;
        processSuccess.code = 0;
        processSuccess.id   = task.id;
        processSuccess.type = "success";
        processSuccess.data = QString::number(pid);
        QByteArray processArray;
        Methods::toJson(processArray, processSuccess);
        client.send(processArray);
    }

    // TODO: 启动线程，创建新的连接去接受服务器的消息

    // TODO:信号处理有问题
    /* 设置捕捉函数 */
    // struct sigaction sig;
    // sig.sa_handler = sig_handler;
    // sigemptyset(&sig.sa_mask);
    // sig.sa_flags = 0;
    // sigaction(SIGCHLD, &sig, NULL);
    /* 然后再unblock */
    // sigdelset(&omask, SIGCHLD);
    // sigprocmask(SIG_SETMASK, &omask, NULL);

    int exitCode;
    waitpid(pid, &exitCode, 0);
    qInfo() << "app exitCode:" << exitCode;

    Methods::ProcessStatus quit;
    quit.code = exitCode;
    quit.id   = task.id;
    quit.type = "quit";
    QByteArray quitArray;
    Methods::toJson(quitArray, quit);
    client.send(quitArray);

    return exitCode;
}
