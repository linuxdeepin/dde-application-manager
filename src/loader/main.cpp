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

#include "../modules/methods/basic.h"
#include "../modules/methods/instance.hpp"
#include "../modules/methods/quit.hpp"
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

static App parseApp(const std::string& app)
{
    std::vector<std::string> strings;
    std::istringstream       stream(app);
    std::string              s;
    while (getline(stream, s, '/')) {
        if (s.empty()) {
            continue;
        }
        strings.push_back(s);
    }

    App result;
    result.prefix = strings[0];
    result.type   = strings[1];
    result.id     = strings[2];

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

int runLinglong(void* _arg)
{
    return 0;
}

int child(void* _arg)
{
    Methods::Task* task = (Methods::Task*) _arg;

    prctl(PR_SET_PDEATHSIG, SIGKILL);
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    App         app = parseApp(task->runId);
    std::string path{ "/usr/share/applications/" + app.id + ".desktop" };
    if (app.type == "user") {
        struct passwd* user = getpwuid(getuid());
        path                = std::string(user->pw_dir) + "/.local/share/applications/" + app.id + ".desktop";
    }
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
    std::filesystem::path container_root_path(annotations.container_root_path);
    if (!std::filesystem::exists(container_root_path)) {
        if (!std::filesystem::create_directories(container_root_path)) {
          std::cout << "[Loader] [Warning] cannot create container root path." << std::endl;
          return -1;
        }
    }
    std::transform(task->environments.begin(), task->environments.end(), std::back_inserter(runtime.process.env),
                   [](const std::pair<std::string, std::string>& pair) -> std::string { return pair.first + "=" + pair.second; });

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
        runtime.process.args.push_back(s);
    }

    std::cout << nlohmann::json(runtime).dump() << std::endl;

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
        (void) close(pipeEnds[1]);
        if (dup2(pipeEnds[0], LINGLONG) == -1) {
            return EXIT_FAILURE;
        }
        (void) close(pipeEnds[0]);
        char const* const args[] = { "/usr/bin/ll-box", LL_TOSTRING(LINGLONG), nullptr };
        int               ret    = execvp(args[0], (char**) args);
        std::cout << "[Loader] [Fork] " << ret << std::endl;
        //std::filesystem::remove(container_root_path);
        exit(ret);
    }
    else {
        nlohmann::json    json = runtime;
        const std::string data = std::move(json.dump());
        close(pipeEnds[0]);
        write(pipeEnds[1], data.c_str(), data.size());
        close(pipeEnds[1]);
    }

    return pid;
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

    // TODO: move to a utils.h
    std::string socketPath{ "/run/user/1000/deepin-application-manager.socket" };

    // register client and run quitConnect
    Socket::Client client;
    client.connect(socketPath);

    Methods::Registe registe;
    registe.id   = dam_task_type;
    registe.hash = dam_task_hash;

    Methods::Registe registe_result;
    registe_result.state = false;
    auto result          = client.get(registe);
    if (!result.is_null()) {
        registe_result = result;
    }
    if (!registe_result.state) {
        return -3;
    }

    Methods::Instance instance;
    instance.hash = registe_result.hash;
    std::cout << "get task" << std::endl;
    result             = client.get(instance);
    Methods::Task task = result;
    std::cout << "[result] " << result << std::endl;

    pthread_attr_t attr;
    size_t         stack_size;
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr, &stack_size);
    pthread_attr_destroy(&attr);

    /* 先将SIGCHLD信号阻塞 保证在子进程结束前设置父进程的捕捉函数 */
    sigset_t nmask, omask;
    sigemptyset(&nmask);
    sigaddset(&nmask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &nmask, &omask);

    //char* stack = (char*) malloc(stack_size);
    //pid_t pid   = clone(child, stack + stack_size, CLONE_NEWPID | SIGCHLD, static_cast<void*>(&task));
    pid_t pid = child(&task);
    // TODO: 启动线程，创建新的连接去接受服务器的消息

    /* 设置捕捉函数 */
    struct sigaction sig;
    sig.sa_handler = sig_handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sigaction(SIGCHLD, &sig, NULL);
    /* 然后再unblock */
    sigdelset(&omask, SIGCHLD);
    sigprocmask(SIG_SETMASK, &omask, NULL);

    int exitCode;
    waitpid(pid, &exitCode, 0);

    Methods::Quit quit;
    quit.code = exitCode;
    quit.id   = task.id;
    client.send(quit);

    return exitCode;
}
