# DDE Application Manager

当前分支用于完全重构。

DDE Application Manager 是深度桌面环境的应用程序管理器。

### 依赖

请查看“debian/control”文件中提供的“Depends”。

### 编译依赖

请查看“debian/control”文件中提供的“Build-Depends”。

## 安装

### 构建过程

1. 确保已经安装了所有的编译依赖。


2. 构建:

```shell
$ cd dde-application-manager
$ cmake -B build
$ cmake --build build -j`nproc`
```
3. 安装

```
sudo cmake --install build
```

## 包管理器集成

**⚠️ 打包维护者和移植者重要提示**

DDE Application Manager 需要**包管理器钩子**来自动检测应用程序的安装、更新和卸载。如果没有这个钩子，当用户安装或卸载应用程序时，桌面环境中的应用程序列表将**不会**自动更新。

该钩子会触发一个 D-Bus 服务（`app-update-notifier`），通知应用程序管理器重新加载应用程序列表。这确保了启动器、应用程序菜单和其他桌面组件与已安装的软件包保持同步。

### 快速设置

对于 **Debian/Ubuntu** 系统，dpkg 钩子会自动安装到:
```
/etc/dpkg/dpkg.cfg.d/am-update-hook
```

对于**其他发行版**（Fedora、Arch Linux、openSUSE 等），你需要配置相应的包管理器钩子。请参阅详细指南:

📖 **[包管理器钩子指南](docs/package-manager-hook.zh_CN.md)**

该指南包括:
- 架构和工作流程说明
- 不同包管理器的钩子配置（dpkg、RPM、Pacman 等）
- 测试和故障排查说明
- 打包维护者验证清单

## 帮助

* [Matrix](https://matrix.to/#/#deepin-community:matrix.org)
* [WiKi](https://wiki.deepin.org)
* [官方论坛](https://bbs.deepin.org)
* [开发者中心](https://github.com/linuxdeepin/developer-center/issues) 

## 贡献指南

我们鼓励您报告问题并做出更改

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## 开源许可证

dde-application-manager 在 [GPL-3.0-or-later](LICENSE)下发布。
