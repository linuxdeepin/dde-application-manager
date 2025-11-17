# Package Manager Hook Guide

## Overview

DDE Application Manager uses a package manager hook to automatically detect application installations, updates, and removals. The hook triggers a D-Bus service that notifies the application manager to reload the application list.

**⚠️ Without this hook, the application list will NOT automatically update when users install or remove applications.**

## How It Works

```
Package Manager Operation → Hook Triggered → D-Bus Signal → Application List Reloaded
```

The hook calls:
```bash
busctl call org.desktopspec.ApplicationUpdateNotifier1 \
    /org/desktopspec/ApplicationUpdateNotifier1 \
    org.freedesktop.DBus.Peer Ping
```

## Hook Configuration

### Debian/Ubuntu (dpkg/apt)

File: `/etc/dpkg/dpkg.cfg.d/am-update-hook`
```bash
post-invoke="busctl call org.desktopspec.ApplicationUpdateNotifier1 /org/desktopspec/ApplicationUpdateNotifier1 org.freedesktop.DBus.Peer Ping &> /dev/null || /bin/true"
```

## Notes for Package Maintainers

- The hook must be installed for automatic application list updates to work
- The hook should never fail package operations (use `|| /bin/true`)
- Ensure D-Bus service files and systemd unit files are properly installed
