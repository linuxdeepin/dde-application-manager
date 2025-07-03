# dde-am Usage Guide

`dde-am` is a command-line tool for launching applications via the ApplicationManager D-Bus interface. It features unified input handling with automatic format detection, supporting app IDs, desktop file paths, and file URIs without requiring specific options.

## Basic Usage

### 1. Launch app using app ID

```bash
dde-am dde-calendar
```

### 2. Launch app using desktop file path

```bash
dde-am /usr/share/applications/dde-calendar.desktop
dde-am ~/.local/share/applications/my-app.desktop
```

### 3. Launch app using file URI

```bash
dde-am file:///usr/share/applications/dde-calendar.desktop
dde-am "file:///home/user/Desktop/Clash Verge.desktop"
```

### 4. Launch app with environment variables (recommended)

```bash
dde-am -e "DEBUG=1" dde-calendar

# Multiple environment variables
dde-am -e "DEBUG=1" -e "LANG=en_US.UTF-8" -e "PATH=/custom/path" dde-file-manager
```


### 5. Launch app with action

```bash
dde-am --action new-window dde-file-manager
dde-am -a new-window /path/to/app.desktop
dde-am deepin-terminal quake-mode
```

### 6. Launch app with user tracking

```bash
dde-am --by-user dde-calendar
```

### 7. List all available app IDs

```bash
dde-am --list
```

## Command Line Options

- `-a, --action <action>`: Specify action identifier for the application
- `-e, --env <env>`: Set environment variable, format: NAME=VALUE (can be used multiple times)
- `--by-user`: Mark as launched by user (useful for counting launched times)
- `--list`: List all available app IDs
- `--help`: Show help information

## Features

### Environment Variable Support

The tool now supports passing environment variables to launched applications using the D-Bus interface's `env` option.

### Unified Input Support

- **Automatic format detection**: Uses `QUrl::fromUserInput()` to intelligently handle various input formats
- **Desktop file paths**: Supports absolute paths to .desktop files
- **File URIs**: Supports file:// URIs and automatically converts them to local paths
- **App IDs**: Traditional app ID format remains fully supported
- **Fallback mechanism**: For .desktop files in non-standard locations, uses filename without .desktop suffix

### Backward Compatibility

All existing functionality is preserved, and the tool maintains backward compatibility with previous versions.

## Input Format Examples

The tool now supports unified input handling - you can use any of these formats without specific options:

### App ID Format

```bash
dde-am dde-calendar
dde-am org.gnome.Calculator
```

### Desktop File Path Format

```bash
dde-am /usr/share/applications/dde-calendar.desktop
dde-am ~/.local/share/applications/custom-app.desktop
dde-am "/home/user/Desktop/My App.desktop"
```

### File URI Format

```bash
dde-am file:///usr/share/applications/dde-calendar.desktop
dde-am "file:///home/user/Desktop/Custom App.desktop"
```

### Combined with Other Options

```bash
# With action
dde-am file:///usr/share/applications/dde-file-manager.desktop --action new-window

# With environment variables
dde-am /usr/share/applications/gimp.desktop -e "LANG=en_US" -e "DEBUG=1"

# All together
dde-am ~/.local/share/applications/my-app.desktop --action open-file -e "VAR=value" --by-user
```

## Examples

### Launch Calculator with Debug Output

```bash
dde-am -e "DEBUG=1" dde-calculator
```

### Launch Browser with Custom Language

```bash
dde-am -e "LANG=en_US.UTF-8" firefox
```

### Launch App from Desktop File

```bash
dde-am ~/.local/share/applications/my-app.desktop
dde-am /usr/share/applications/gimp.desktop
```

### Launch App from URI with Environment Variable

```bash
dde-am file:///usr/share/applications/gimp.desktop -e "DISPLAY=:1"
dde-am "file:///home/user/Desktop/Custom App.desktop" -e "DEBUG=1"
```

### Launch App with Action

```bash
# Using --action option
dde-am --action new-window dde-file-manager
dde-am -a compose thunderbird

# Using positional argument
dde-am dde-text-editor new-document
```

### Launch App with Multiple Environment Variables

```bash
# Set multiple environment variables for development
dde-am -e "DEBUG=1" -e "VERBOSE=1" -e "LOG_LEVEL=trace" my-dev-app

# Launch with custom locale and display settings
dde-am -e "LANG=zh_CN.UTF-8" -e "DISPLAY=:0" -e "GTK_THEME=Adwaita" dde-file-manager
```

## Notes

- App IDs are typically derived from .desktop filenames
- Environment variables must use the format "NAME=VALUE"
- The `-e/--env` option is recommended over positional environment variables
- 只能通过 `--action` 选项指定 action，位置参数不再支持 action
- Input format is automatically detected - no need for `-d` or `-u` options
- For apps with spaces in their names, use quotes around the app ID or path
- The tool automatically handles URL encoding in file URIs
