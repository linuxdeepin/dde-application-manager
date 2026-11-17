# DConfig 会话感知应用覆写配置

本文档介绍 `dde-application-manager` 提供的 DConfig 会话感知应用覆写配置项。第三方开发者可通过这些配置根据当前会话类型（Wayland 或 X11）覆写应用的 `Exec`、`TryExec` 和 `Icon` 字段。

## 配置概览

| 配置项 | 权限 | Subpath | 说明 |
|--------|------|---------|------|
| `Exec` | readonly | `/<desktopId>/<wayland\|x11>` | 覆写 `Exec` 字段；支持 `!AM_FULL!` 占位符 |
| `TryExec` | readonly | `/<desktopId>/<wayland\|x11>` | 覆写 `TryExec` 字段 |
| `Icon` | readwrite | `/<desktopId>/<wayland\|x11>` | 覆写 `Icon` 字段 |

**配置资源：** `org.deepin.dde.am.appoverride`

## Per-App + Per-Session-Type Subpath 机制

所有配置项使用 **per-app + per-session-type subpath**，格式为 `/<desktopId>/<sessionType>`，其中：

- `<desktopId>` 为应用的 desktop ID，从 desktop 文件在 `applications` 目录下的相对路径派生，目录分隔符替换为连字符（如 `foo/bar.desktop` → `foo-bar`）
- `<sessionType>` 为 `wayland` 或 `x11`

这允许您为每个应用在每个会话类型下独立配置不同的覆写。

### 会话类型判断

会话类型在启动时按以下逻辑判断：

1. 如果 `WAYLAND_DISPLAY` 环境变量已设置且非空 → **Wayland**
2. 如果 `XDG_SESSION_TYPE` 为 `wayland`（不区分大小写）→ **Wayland**
3. 如果 `XDG_SESSION_TYPE` 为 `x11`（不区分大小写）→ **X11**
4. 其他情况 → **Unknown**

当会话类型为 **Unknown** 时，会话覆写**被禁用** — 不会创建 DConfig subpath，也不会应用任何覆写。

## 配置项说明

### Exec

- **类型：** `string`
- **权限：** readonly
- **Subpath：** `/<desktopId>/<wayland|x11>`
- **默认值：** `""`（空 — 不覆写）

覆写应用 desktop 文件中的 `Exec` 字段。支持 `!AM_FULL!` 占位符，该占位符会被替换为 desktop 文件中的原始 `Exec` 值（未经兼容性处理的值）。占位符可出现多次。

### TryExec

- **类型：** `string`
- **权限：** readonly
- **Subpath：** `/<desktopId>/<wayland|x11>`
- **默认值：** `""`（空 — 不覆写）

覆写应用 desktop 文件中的 `TryExec` 字段。设置为非空值时，应用的可见性由指定可执行文件是否存在于 `PATH` 中（或为绝对路径且存在且可执行）决定。

> **TryExec 强制显示限制：** 虽然 DConfig 描述中提到"空字符串强制显示应用"，但在当前版本中，此行为通过 DConfig 覆写路径**不可达**：
> 1. `updateOverride()` 会跳过空的 `TryExec` 值 — 不会存入覆写缓存。
> 2. `getValue()` 对空 `TryExec` 返回 `std::nullopt`。
> 3. 因此 `tryExecCheck` 中的空 `TryExec` 强制显示分支永远不会通过 DConfig 覆写触发。

### Icon

- **类型：** `string`
- **权限：** readwrite
- **Subpath：** `/<desktopId>/<wayland|x11>`
- **默认值：** `""`（空 — 不覆写）

覆写应用 desktop 文件中的 `Icon` 字段。可通过 `dconfig` CLI 工具修改。

## 覆写优先级

启动应用时，有效的 `Exec` 值按以下优先级确定（从高到低）：

1. **会话覆写**（DConfig `org.deepin.dde.am.appoverride`）— 最高优先级
2. **兼容性文件**（`/var/lib/compatible/compatibleDesktop.json`）
3. **Desktop 文件** — `.desktop` 文件中的原始 `Exec`

如果设置了会话覆写的 `Exec`，它将优先于兼容性文件和 desktop 文件。

### !AM_FULL! 占位符

当 `Exec` 覆写值包含 `!AM_FULL!` 时，它会被替换为原始 `Exec` 值（来自 desktop 文件，未经兼容性处理的值）。这允许您包装或扩展原始命令：

- 覆写值：`env GDK_BACKEND=wayland !AM_FULL!`
- 原始 Exec：`my-app --option`
- 有效 Exec：`env GDK_BACKEND=wayland my-app --option`

占位符可在覆写值中出现多次。

## 动态重载

当 DConfig 值变更时，覆写会自动重新加载：

1. DConfig 发出变更 key 的 `valueChanged` 信号
2. 调用 `SessionOverrideConfig::updateOverride()` 刷新内存缓存
3. 发出 `overrideChanged(desktopId, key)` 和 `configChanged()` 信号
4. 上层（`ApplicationManager1Service`）接收这些信号，重新加载应用配置，并发出对应的 D-Bus 信号（`iconsChanged` / `execsChanged`）

这意味着对覆写文件或 DConfig 值的变更无需重启 `dde-application-manager` 即可生效。

## 修改配置

### 使用 dconfig CLI（readwrite 配置项）

对于 `Icon`，可使用 `dconfig` CLI 工具：

```bash
# 覆写 Wayland 会话下应用的 Icon
dconfig set -d org.deepin.dde.am.appoverride \
  -r /org/desktopspec/ApplicationManager1 \
  -k Icon \
  -s '"my-app-wayland-icon"' \
  -p /my-app/wayland
```

### 使用覆写文件（readonly 配置项）

对于 `Exec` 和 `TryExec`（readonly）或需要设置系统级覆写时，使用覆写文件。

**覆写文件路径：**

- **系统级：** `/usr/share/dsg/configs/overrides/org.deepin.dde.am.appoverride/<desktopId>/<sessionType>.json`
- **用户级：** `~/.config/dsg/configs/overrides/org.deepin.dde.am.appoverride/<desktopId>/<sessionType>.json`

**覆写文件格式：**

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "Exec": "env GDK_BACKEND=wayland !AM_FULL!"
    }
}
```

## 示例

### 示例 1：覆写 Wayland 会话的 Exec

要覆写 `my-app` 在 Wayland 会话下的 `Exec`，前置 Wayland 专用环境变量：

创建 `~/.config/dsg/configs/overrides/org.deepin.dde.am.appoverride/my-app/wayland.json`：

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "Exec": "env GDK_BACKEND=wayland !AM_FULL!"
    }
}
```

### 示例 2：覆写 X11 会话的 TryExec

要覆写 `my-app` 在 X11 会话下的 `TryExec`：

创建 `~/.config/dsg/configs/overrides/org.deepin.dde.am.appoverride/my-app/x11.json`：

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "TryExec": "/usr/bin/my-app-x11"
    }
}
```

> **注意：** 空的 `TryExec` 值不会通过 DConfig 触发强制显示行为。参见上方 [TryExec 强制显示限制](#tryexec)。

### 示例 3：覆写 Wayland 会话的 Icon

要覆写 `my-app` 在 Wayland 会话下的 `Icon`：

创建 `~/.config/dsg/configs/overrides/org.deepin.dde.am.appoverride/my-app/wayland.json`：

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "Icon": "my-app-wayland-icon"
    }
}
```

也可以使用 `dconfig` CLI：

```bash
dconfig set -d org.deepin.dde.am.appoverride \
  -r /org/desktopspec/ApplicationManager1 \
  -k Icon \
  -s '"my-app-wayland-icon"' \
  -p /my-app/wayland
```

### 示例 4：在一个文件中覆写多个字段

可以在单个文件中覆写多个字段：

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "Exec": "env QT_QPA_PLATFORM=wayland !AM_FULL!",
        "Icon": "my-app-wayland-icon"
    }
}
```
