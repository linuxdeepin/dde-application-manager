# DDE Application Manager

## WORK IN PROGRESS

**This branch is used to totally refactor.**

[Refactor plan](./docs/TODO.md).

---

DDE Application Manager is the app manager of Deepin Desktop Environment.

### Dependencies

You can also check the "Depends" provided in the `debian/control` file.

### Build dependencies

You can also check the "Build-Depends" provided in the `debian/control` file.

## Installation

### Build from source code

1. Make sure you have installed all dependencies.

2. Build:
```shell
$ cd dde-application-manager
$ cmake -B build
$ cmake --build build -j`nproc`
```
3. Install

```
sudo cmake --install build
```

## Package Manager Integration

**⚠️ Important for Package Maintainers and Porters**

DDE Application Manager requires a **package manager hook** to automatically detect application installations, updates, and removals. Without this hook, the application list in the desktop environment will NOT automatically update when users install or remove applications.

The hook triggers a D-Bus service (`app-update-notifier`) that notifies the application manager to reload the application list. This ensures the launcher, application menu, and other desktop components stay synchronized with installed packages.

### Quick Setup

For **Debian/Ubuntu** systems, the dpkg hook is automatically installed to:
```
/etc/dpkg/dpkg.cfg.d/am-update-hook
```

For **other distributions** (Fedora, Arch Linux, openSUSE, etc.), you need to configure the appropriate package manager hook. See the detailed guide:

📖 **[Package Manager Hook Guide](docs/package-manager-hook.md)**

This guide includes:
- Architecture and workflow explanation
- Hook configurations for different package managers (dpkg, RPM, Pacman, etc.)
- Testing and troubleshooting instructions
- Verification checklist for package maintainers

## Getting help

* [Matrix](https://matrix.to/#/#deepin-community:matrix.org)
* [WiKi](https://wiki.deepin.org)
* [Forum](https://bbs.deepin.org)
* [Developer Center](https://github.com/linuxdeepin/developer-center/issues) 

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## License

dde-application-manager is licensed under [GPL-3.0-or-later](LICENSE).
