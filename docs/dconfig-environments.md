# DConfig Environment Configuration

This document describes the DConfig configuration items provided by `dde-application-manager` for controlling application launch environments. Third-party developers can use these configurations to inject or filter environment variables when applications are launched.

## Configuration Overview

| Key | Permissions | Subpath | Description |
|-----|-------------|---------|-------------|
| `appExtraEnvironments` | readwrite | `/<appid>` | Extra environment variables appended when launching an app |
| `appEnvironmentsBlacklist` | readwrite | `/<appid>` | Environment variables to remove before launching an app |
| `skipEventAppIds` | readonly | *(none — global)* | App IDs whose event reporting is skipped |

**Config resource:** `org.deepin.dde.application-manager`

## Per-App Subpath Mechanism

`appExtraEnvironments` and `appEnvironmentsBlacklist` use a **per-app subpath** of the form `/<appid>`, where `<appid>` is the application's desktop ID. The desktop ID is derived from the desktop file's relative path under the `applications` directory, with directory separators replaced by hyphens (e.g., `foo/bar.desktop` → `foo-bar`). This means you can configure different environment variables for each application independently.

`skipEventAppIds` is a **global** configuration — it does not use a subpath and applies to all applications.

### DSG_APP_ID

When `dde-application-manager` launches an application, it automatically injects the `DSG_APP_ID` environment variable set to the app's ID. Applications can read this variable to identify themselves.

## Configuration Keys

### appExtraEnvironments

- **Type:** `string[]` (list of `KEY=VALUE` strings)
- **Permissions:** readwrite
- **Subpath:** `/<appid>`
- **Default:** `[]`

Each entry is an environment variable assignment in the form `KEY=VALUE`. These are appended to the application's environment at launch time.

### appEnvironmentsBlacklist

- **Type:** `string[]` (list of environment variable names)
- **Permissions:** readwrite
- **Subpath:** `/<appid>`
- **Default:** `[]`

Each entry is an environment variable **name** (without `=VALUE`). These variables are removed from the application's environment before launch.

### skipEventAppIds

- **Type:** `string[]` (list of app IDs)
- **Permissions:** readonly
- **Subpath:** *(none — global)*
- **Default:** `["dde-file-manager"]`

Applications listed here will not have their launch events reported by `dde-application-manager`. This is used when an application handles its own event reporting to avoid duplicates.

## Modifying Configuration

### Using dconfig CLI (readwrite keys)

For `appExtraEnvironments` and `appEnvironmentsBlacklist`, you can use the `dconfig` CLI tool:

```bash
# Set extra environment variables for an app
dconfig set -d org.deepin.dde.application-manager \
  -r /org/desktopspec/ApplicationManager1 \
  -k appExtraEnvironments \
  -s '["MY_VAR=hello","DEBUG=1"]' \
  -p /my-app

# Set environment blacklist for an app
dconfig set -d org.deepin.dde.application-manager \
  -r /org/desktopspec/ApplicationManager1 \
  -k appEnvironmentsBlacklist \
  -s '["GTK_MODULES"]' \
  -p /my-app
```

### Using Override Files (readonly keys)

For `skipEventAppIds` (readonly) or to apply system-wide defaults, use override files.

**Override file locations:**

- **System-level:** `/usr/share/dsg/configs/overrides/org.deepin.dde.application-manager/<appid>.json`
- **User-level:** `~/.config/dsg/configs/overrides/org.deepin.dde.application-manager/<appid>.json`

For global keys (no subpath), the file path is:

- **System-level:** `/usr/share/dsg/configs/overrides/org.deepin.dde.application-manager.json`
- **User-level:** `~/.config/dsg/configs/overrides/org.deepin.dde.application-manager.json`

**Override file format:**

The file uses the `dsg.config.override` magic header:

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "appExtraEnvironments": ["MY_VAR=hello", "DEBUG=1"]
    }
}
```

## Examples

### Example 1: Add environment variables for a specific app

To add `MY_VAR=hello` and `DEBUG=1` when launching `my-app`:

Create `~/.config/dsg/configs/overrides/org.deepin.dde.application-manager/my-app.json`:

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "appExtraEnvironments": ["MY_VAR=hello", "DEBUG=1"]
    }
}
```

### Example 2: Remove environment variables for a specific app

To remove `GTK_MODULES` from the environment when launching `my-app`:

Create `~/.config/dsg/configs/overrides/org.deepin.dde.application-manager/my-app.json`:

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "appEnvironmentsBlacklist": ["GTK_MODULES"]
    }
}
```

### Example 3: Skip event reporting for an app (global)

To skip event reporting for `my-app`:

Create `/usr/share/dsg/configs/overrides/org.deepin.dde.application-manager.json`:

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "skipEventAppIds": ["dde-file-manager", "my-app"]
    }
}
```
