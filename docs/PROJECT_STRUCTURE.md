# Project Structure Guide

This document explains the organization of the ESP32 touchscreen firmware project.

## Directory Layout

```
precisionpour/
├── .editorconfig          # Editor configuration for consistent formatting
├── .clang-format          # ClangFormat style configuration
├── .gitignore             # Git ignore rules
├── .vscode/               # VS Code workspace settings
│   └── extensions.json    # Recommended extensions
├── platformio.ini         # PlatformIO project configuration
├── README.md              # Main project documentation
│
├── src/                   # Source Files
│   ├── main.cpp           # Main firmware entry point
│   ├── lvgl_display.cpp   # LVGL display driver
│   ├── lvgl_touch.cpp     # LVGL touch driver
│   ├── splashscreen.cpp   # Splashscreen implementation
│   ├── production_mode_ui.cpp  # Production mode UI
│   ├── test_mode_ui.cpp   # Test mode UI
│   ├── wifi_manager.cpp   # WiFi connection management
│   ├── mqtt_client.cpp    # MQTT client implementation
│   └── flow_meter.cpp     # Flow meter reading and calculations
│
├── include/               # Header Files
│   ├── config.h           # Hardware pin definitions and settings
│   ├── lvgl_display.h     # Display driver interface
│   ├── lvgl_touch.h       # Touch driver interface
│   ├── splashscreen.h     # Splashscreen interface
│   ├── production_mode_ui.h  # Production UI interface
│   ├── test_mode_ui.h     # Test mode UI interface
│   ├── wifi_manager.h     # WiFi manager interface
│   ├── mqtt_client.h      # MQTT client interface
│   ├── flow_meter.h       # Flow meter interface
│   ├── secrets.h          # WiFi/MQTT credentials (gitignored)
│   └── secrets.h.example  # Credentials template
│
├── lib/                   # Custom Libraries
│   └── TFT_eSPI_Config/   # TFT_eSPI display configuration
│       └── User_Setup.h
│
├── resources/             # Resource Files
│   └── images/            # Image files for conversion
│
├── test/                  # Unit Tests
│   └── (test files here)
│
├── data/                  # Filesystem Data
│   └── (files to upload to ESP32 filesystem)
│
└── docs/                  # Documentation
    ├── SETUP_GUIDE.md     # PlatformIO installation guide
    ├── QUICK_REFERENCE.md # Command reference
    ├── HARDWARE_SETUP.md  # Hardware configuration guide
    └── PROJECT_STRUCTURE.md # This file
```

## Directory Purposes

### `src/`
Contains all C++ source files (`.cpp`). PlatformIO automatically compiles all files in this directory.

**Best Practices:**
- Keep source files organized by functionality
- Use meaningful file names
- One class per file when possible

### `include/`
Contains header files (`.h`, `.hpp`). PlatformIO automatically adds this directory to the include path, so you can use:
```cpp
#include "config.h"  // No need for include/config.h
```

**Best Practices:**
- Keep headers organized by functionality
- Use header guards (`#ifndef`, `#define`, `#endif`)
- Prefer forward declarations when possible

### `lib/`
For custom libraries that aren't available via PlatformIO's library manager.

**When to use:**
- Custom driver implementations
- Proprietary libraries
- Modified versions of existing libraries
- Local development libraries

**Best Practices:**
- Each library in its own subdirectory
- Include a README in each library directory
- Follow PlatformIO library structure conventions

### `test/`
Unit tests for your firmware. PlatformIO uses Unity test framework by default.

**Usage:**
```bash
pio test              # Run all tests
pio test -v           # Verbose output
pio test -f test_*    # Run specific test
```

**Best Practices:**
- One test file per source file
- Use descriptive test names
- Test both success and failure cases

### `data/`
Files that will be uploaded to the ESP32's filesystem (SPIFFS or LittleFS).

**Usage:**
```bash
pio run -t uploadfs   # Upload filesystem data
```

**Common uses:**
- Web server assets (HTML, CSS, JS)
- Configuration files (JSON, XML)
- Font files
- Images
- Firmware update files

**Best Practices:**
- Keep file sizes reasonable
- Use efficient formats
- Organize in subdirectories if needed

### `docs/`
Project documentation and guides.

**Best Practices:**
- Keep documentation up to date
- Use Markdown format
- Include code examples
- Link between related documents

## PlatformIO Auto-Detection

PlatformIO automatically detects and uses these directories:
- `src/` - Source files
- `include/` - Header files (added to include path)
- `lib/` - Custom libraries
- `test/` - Unit tests
- `data/` - Filesystem data

No configuration needed in `platformio.ini` for these directories!

## Code Organization Best Practices

1. **Separation of Concerns**
   - Hardware configuration → `include/config.h`
   - Display logic → Separate files in `src/` and `include/`
   - Touch logic → Separate files in `src/` and `include/`
   - Main application → `src/main.cpp`

2. **Interface-Based Design**
   - Use abstract interfaces (`display.h`, `touch.h`)
   - Implement interfaces for specific hardware
   - Makes code testable and swappable

3. **Configuration Management**
   - Centralize pin definitions in `config.h`
   - Use `#define` for compile-time configuration
   - Consider runtime configuration for filesystem-based settings

4. **Library Management**
   - Use PlatformIO's `lib_deps` for public libraries
   - Use `lib/` only for custom/proprietary libraries
   - Document library dependencies in README

## Adding New Files

### Adding a Source File
1. Create `.cpp` file in `src/`
2. Create corresponding `.h` file in `include/`
3. Include the header in files that need it
4. PlatformIO will automatically compile it

### Adding a Custom Library
1. Create subdirectory in `lib/`
2. Add library files following PlatformIO conventions
3. Create `library.json` or `library.properties` if needed
4. Include in your code: `#include <YourLibrary.h>`

### Adding a Test
1. Create test file in `test/` (e.g., `test_display.cpp`)
2. Use Unity test framework
3. Run with `pio test`

### Adding Filesystem Data
1. Place files in `data/`
2. Upload with `pio run -t uploadfs`
3. Access in code using filesystem APIs

## Build Output

PlatformIO creates a `.pio/` directory (ignored by git) containing:
- Compiled binaries
- Build artifacts
- Downloaded libraries
- Platform files

Never commit `.pio/` to version control!
