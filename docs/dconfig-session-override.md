# DConfig Session-Aware App Override

This document describes the DConfig configuration items provided by `dde-application-manager` for session-aware application overrides. Third-party developers can use these configurations to override an application's `Exec`, `TryExec`, and `Icon` fields based on the current session type (Wayland or X11).

## Configuration Overview

| Key | Permissions | Subpath | Description |
|-----|-------------|---------|-------------|
| `Exec` | readonly | `/<desktopId>/<wayland\|x11>` | Overrides the `Exec` field; supports `!AM_FULL!` placeholder |
| `TryExec` | readonly | `/<desktopId>/<wayland\|x11>` | Overrides the `TryExec` field |
| `Icon` | readwrite | `/<desktopId>/<wayland\|x11>` | Overrides the `Icon` field |

**Config resource:** `org.deepin.dde.am.appoverride`

## Per-App + Per-Session-Type Subpath Mechanism

All keys use a **per-app + per-session-type subpath** of the form `/<desktopId>/<sessionType>`, where:

- `<desktopId>` is the application's desktop ID, derived from the desktop file's relative path under the `applications` directory with directory separators replaced by hyphens (e.g., `foo/bar.desktop` → `foo-bar`)
- `<sessionType>` is either `wayland` or `x11`

This allows you to configure different overrides for each application in each session type independently.

### Session Type Detection

The session type is determined at startup using the following logic:

1. If the `WAYLAND_DISPLAY` environment variable is set and non-empty → **Wayland**
2. If `XDG_SESSION_TYPE` is `wayland` (case-insensitive) → **Wayland**
3. If `XDG_SESSION_TYPE` is `x11` (case-insensitive) → **X11**
4. Otherwise → **Unknown**

When the session type is **Unknown**, session overrides are **disabled** — no DConfig subpath is created and no overrides are applied.

## Configuration Keys

### Exec

- **Type:** `string`
- **Permissions:** readonly
- **Subpath:** `/<desktopId>/<wayland|x11>`
- **Default:** `""` (empty — no override)

Overrides the `Exec` field of the application's desktop file. Supports the `!AM_FULL!` placeholder, which is replaced with the original `Exec` value from the desktop file (before compatibility processing). The placeholder can appear multiple times.

### TryExec

- **Type:** `string`
- **Permissions:** readonly
- **Subpath:** `/<desktopId>/<wayland|x11>`
- **Default:** `""` (empty — no override)

Overrides the `TryExec` field of the application's desktop file. When set to a non-empty value, the application's visibility is determined by whether the specified executable exists in `PATH` (or is an absolute path that exists and is executable).

> **TryExec Force-Show Limitation:** Although the DConfig description states "Empty string forces the app to be shown," this behavior is **not reachable** through the DConfig override path in the current version:
> 1. `updateOverride()` skips empty `TryExec` values — they are not stored in the override cache.
> 2. `getValue()` returns `std::nullopt` for empty `TryExec`.
> 3. Therefore, the empty `TryExec` force-show branch in `tryExecCheck` is never executed via DConfig overrides.

### Icon

- **Type:** `string`
- **Permissions:** readwrite
- **Subpath:** `/<desktopId>/<wayland|x11>`
- **Default:** `""` (empty — no override)

Overrides the `Icon` field of the application's desktop file. This can be modified via the `dconfig` CLI tool.

## Override Priority

When launching an application, the effective `Exec` value is determined by the following priority (highest to lowest):

1. **Session override** (DConfig `org.deepin.dde.am.appoverride`) — highest priority
2. **Compatibility file** (`/var/lib/compatible/compatibleDesktop.json`)
3. **Desktop file** — the original `Exec` from the `.desktop` file

If a session override for `Exec` is set, it takes precedence over both the compatibility file and the desktop file.

### !AM_FULL! Placeholder

When the `Exec` override value contains `!AM_FULL!`, it is replaced with the original `Exec` value (from the desktop file, before compatibility processing). This allows you to wrap or extend the original command:

- Override: `env GDK_BACKEND=wayland !AM_FULL!`
- Original Exec: `my-app --option`
- Effective Exec: `env GDK_BACKEND=wayland my-app --option`

The placeholder can appear multiple times in the override value.

## Dynamic Reload

When a DConfig value changes, the override is reloaded automatically:

1. DConfig emits the `valueChanged` signal for the changed key
2. `SessionOverrideConfig::updateOverride()` is called to refresh the in-memory cache
3. `overrideChanged(desktopId, key)` and `configChanged()` signals are emitted
4. The upper layer (`ApplicationManager1Service`) receives these signals, reloads the application configuration, and emits corresponding D-Bus signals (`iconsChanged` / `execsChanged`)

This means changes to override files or DConfig values take effect without restarting `dde-application-manager`.

## Modifying Configuration

### Using dconfig CLI (readwrite keys)

For `Icon`, you can use the `dconfig` CLI tool:

```bash
# Override the Icon for an app in Wayland session
dconfig set -d org.deepin.dde.am.appoverride \
  -r /org/desktopspec/ApplicationManager1 \
  -k Icon \
  -s '"my-app-wayland-icon"' \
  -p /my-app/wayland
```

### Using Override Files (readonly keys)

For `Exec` and `TryExec` (readonly) or to apply system-wide overrides, use override files.

**Override file locations:**

- **System-level:** `/usr/share/dsg/configs/overrides/org.deepin.dde.am.appoverride/<desktopId>/<sessionType>.json`
- **User-level:** `~/.config/dsg/configs/overrides/org.deepin.dde.am.appoverride/<desktopId>/<sessionType>.json`

**Override file format:**

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "Exec": "env GDK_BACKEND=wayland !AM_FULL!"
    }
}
```

## Examples

### Example 1: Override Exec for Wayland session

To override the `Exec` for `my-app` in Wayland sessions to prepend a Wayland-specific environment:

Create `~/.config/dsg/configs/overrides/org.deepin.dde.am.appoverride/my-app/wayland.json`:

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "Exec": "env GDK_BACKEND=wayland !AM_FULL!"
    }
}
```

### Example 2: Override TryExec for X11 session

To override `TryExec` for `my-app` in X11 sessions:

Create `~/.config/dsg/configs/overrides/org.deepin.dde.am.appoverride/my-app/x11.json`:

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "TryExec": "/usr/bin/my-app-x11"
    }
}
```

> **Note:** An empty `TryExec` value will not trigger the force-show behavior via DConfig. See [TryExec Force-Show Limitation](#tryexec) above.

### Example 3: Override Icon for Wayland session

To override the `Icon` for `my-app` in Wayland sessions:

Create `~/.config/dsg/configs/overrides/org.deepin.dde.am.appoverride/my-app/wayland.json`:

```json
{
    "magic": "dsg.config.override",
    "version": "1.0",
    "contents": {
        "Icon": "my-app-wayland-icon"
    }
}
```

Alternatively, use the `dconfig` CLI:

```bash
dconfig set -d org.deepin.dde.am.appoverride \
  -r /org/desktopspec/ApplicationManager1 \
  -k Icon \
  -s '"my-app-wayland-icon"' \
  -p /my-app/wayland
```

### Example 4: Multiple overrides in one file

You can override multiple fields in a single file:

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
