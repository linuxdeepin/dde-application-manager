# DConfig 环境变量配置

本文档介绍 `dde-application-manager` 提供的 DConfig 配置项，用于控制应用启动时的环境变量。第三方开发者可通过这些配置在应用启动时注入或过滤环境变量。

## 配置概览

| 配置项 | 权限 | Subpath | 说明 |
|--------|------|---------|------|
| `appExtraEnvironments` | readwrite | `/<appid>` | 启动应用时附加的额外环境变量 |
| `appEnvironmentsBlacklist` | readwrite | `/<appid>` | 启动应用时需移除的环境变量 |
| `skipEventAppIds` | readonly | *（无 — 全局）* | 跳过事件上报的应用 ID 列表 |

**配置资源：** `org.deepin.dde.application-manager`

## Per-App Subpath 机制

`appExtraEnvironments` 和 `appEnvironmentsBlacklist` 使用 **per-app subpath**，格式为 `/<appid>`，其中 `<appid>` 为应用的 desktop ID。desktop ID 从 desktop 文件在 `applications` 目录下的相对路径派生，目录分隔符替换为连字符（如 `foo/bar.desktop` → `foo-bar`）。这意味着您可以为每个应用独立配置不同的环境变量。

`skipEventAppIds` 为**全局**配置 — 不使用 subpath，对所有应用生效。

### DSG_APP_ID

`dde-application-manager` 启动应用时会自动注入 `DSG_APP_ID` 环境变量，值为应用的 ID。应用可读取此变量进行自身标识。

## 配置项说明

### appExtraEnvironments

- **类型：** `string[]`（`KEY=VALUE` 字符串列表）
- **权限：** readwrite
- **Subpath：** `/<appid>`
- **默认值：** `[]`

每个条目为 `KEY=VALUE` 格式的环境变量赋值。这些变量会在启动时追加到应用环境中。

### appEnvironmentsBlacklist

- **类型：** `string[]`（环境变量名列表）
- **权限：** readwrite
- **Subpath：** `/<appid>`
- **默认值：** `[]`

每个条目为环境变量**名称**（不含 `=VALUE`）。这些变量会在启动前从应用环境中移除。

### skipEventAppIds

- **类型：** `string[]`（应用 ID 列表）
- **权限：** readonly
- **Subpath：** *（无 — 全局）*
- **默认值：** `["dde-file-manager"]`

此列表中的应用不会由 `dde-application-manager` 上报启动事件。当应用自行处理事件上报时使用此项以避免重复上报。

## 修改配置

### 使用 dconfig CLI（readwrite 配置项）

对于 `appExtraEnvironments` 和 `appEnvironmentsBlacklist`，可使用 `dconfig` CLI 工具：

```bash
# 为应用设置额外环境变量
dconfig set -d org.deepin.dde.application-manager \
  -r /org/desktopspec/ApplicationManager1 \
  -k appExtraEnvironments \
  -s '["MY_VAR=hello","DEBUG=1"]' \
  -p /my-app

# 为应用设置环境变量黑名单
dconfig set -d org.deepin.dde.application-manager \
  -r /org/desktopspec/ApplicationManager1 \
  -k appEnvironmentsBlacklist \
  -s '["GTK_MODULES"]' \
  -p /my-app
```

### 使用覆写文件（readonly 配置项）

对于 `skipEventAppIds`（readonly）或需要设置系统级默认值时，使用覆写文件。

**覆写文件路径：**

- **系统级：** `/usr/share/dsg/configs/overrides/org.deepin.dde.application-manager/<appid>.json`
- **用户级：** `~/.config/dsg/configs/overrides/org.deepin.dde.application-manager/<appid>.json`

对于全局配置项（无 subpath），文件路径为：

- **系统级：** `/usr/share/dsg/configs/overrides/org.deepin.dde.application-manager.json`
- **用户级：** `~/.config/dsg/configs/overrides/org.deepin.dde.application-manager.json`

**覆写文件格式：**

文件使用 `dsg.config.override` magic 头：

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "appExtraEnvironments": ["MY_VAR=hello", "DEBUG=1"]
    }
}
```

## 示例

### 示例 1：为特定应用添加环境变量

要在启动 `my-app` 时添加 `MY_VAR=hello` 和 `DEBUG=1`：

创建 `~/.config/dsg/configs/overrides/org.deepin.dde.application-manager/my-app.json`：

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "appExtraEnvironments": ["MY_VAR=hello", "DEBUG=1"]
    }
}
```

### 示例 2：为特定应用移除环境变量

要在启动 `my-app` 时移除 `GTK_MODULES` 环境变量：

创建 `~/.config/dsg/configs/overrides/org.deepin.dde.application-manager/my-app.json`：

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "appEnvironmentsBlacklist": ["GTK_MODULES"]
    }
}
```

### 示例 3：跳过应用的事件上报（全局）

要跳过 `my-app` 的事件上报：

创建 `/usr/share/dsg/configs/overrides/org.deepin.dde.application-manager.json`：

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "skipEventAppIds": ["dde-file-manager", "my-app"]
    }
}
```
