# 包管理器钩子指南

## 概述

DDE Application Manager 使用包管理器钩子来自动检测应用程序的安装、更新和卸载。钩子触发 D-Bus 服务，通知应用程序管理器重新加载应用程序列表。

**⚠️ 如果没有这个钩子，当用户安装或卸载应用程序时，应用程序列表将不会自动更新。**

## 工作原理

```
包管理器操作 → 钩子触发 → D-Bus 信号 → 应用程序列表重新加载
```

钩子调用:
```bash
busctl call org.desktopspec.ApplicationUpdateNotifier1 \
    /org/desktopspec/ApplicationUpdateNotifier1 \
    org.freedesktop.DBus.Peer Ping
```

## 钩子配置

### Debian/Ubuntu (dpkg/apt)

文件: `/etc/dpkg/dpkg.cfg.d/am-update-hook`
```bash
post-invoke="busctl call org.desktopspec.ApplicationUpdateNotifier1 /org/desktopspec/ApplicationUpdateNotifier1 org.freedesktop.DBus.Peer Ping &> /dev/null || /bin/true"
```

## 打包维护者注意事项

- 必须安装钩子才能实现应用程序列表的自动更新
- 钩子不应导致包操作失败（使用 `|| /bin/true`）
- 确保正确安装 D-Bus 服务文件和 systemd 单元文件
