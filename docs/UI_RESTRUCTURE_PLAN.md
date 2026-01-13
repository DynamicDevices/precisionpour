# UI Restructure Plan - PrecisionPour Application

## Current State Analysis

### Existing Screens
1. **Splashscreen** - Startup screen with logo and progress bar
2. **Production Mode (QR Code Screen)** - Main waiting screen with QR code
3. **Pouring Mode** - Active pouring screen with flow rate, volume, cost
4. **Missing: Finished Screen** - Completion screen after pouring

### Current Issues
1. **Code Duplication**: Logo, WiFi icon, and data icon are recreated for each screen
2. **Memory Usage**: Logo image data is shared (good), but UI objects are duplicated
3. **No Standard Layout**: Each screen implements its own layout logic
4. **State Management**: Basic enum in main.cpp, no centralized state machine
5. **No Finished Screen**: Missing completion screen
6. **No Timeout**: No automatic return to QR code screen after pouring completes

### UX Flow Requirements
```
Splashscreen (startup)
    ↓
QR Code Screen (waiting for payment)
    ↓ (MQTT "paid" message received)
Pouring Screen (active pouring)
    ↓ (pouring completes)
Finished Screen (completion message)
    ↓ (timeout: 5-10 seconds)
QR Code Screen (ready for next customer)
```

## Proposed Architecture

### 1. Screen Manager Component
**File**: `include/screen_manager.h` / `src/screen_manager.cpp`

**Purpose**: Centralized screen state management and transitions

**Features**:
- State machine for screen transitions
- Screen lifecycle management (init, update, cleanup)
- Timeout handling for finished screen
- Event callbacks for state changes

**Screen States**:
```cpp
enum ScreenState {
    SCREEN_SPLASH,      // Startup splashscreen
    SCREEN_QR_CODE,     // QR code / payment waiting
    SCREEN_POURING,     // Active pouring
    SCREEN_FINISHED     // Pouring complete
};
```

### 2. Base Screen Layout Component
**File**: `include/base_screen.h` / `src/base_screen.cpp`

**Purpose**: Standardized screen layout with shared UI elements

**Layout Structure**:
```
┌─────────────────────────┐
│   Logo (top center)     │  ← Shared component
├─────────────────────────┤
│                         │
│   Content Area          │  ← Screen-specific content
│   (QR code, pouring     │
│    info, finished msg)  │
│                         │
├─────────────────────────┤
│ WiFi Icon │ Data Icon   │  ← Shared components
│ (bot left)│ (bot right) │
└─────────────────────────┘
```

**Functions**:
- `base_screen_create()` - Create standard layout
- `base_screen_get_content_area()` - Get content container for screen-specific elements
- `base_screen_update_wifi_icon()` - Update WiFi status
- `base_screen_update_data_icon()` - Update MQTT/data status
- `base_screen_cleanup()` - Clean up shared elements

**Memory Optimization**:
- Logo image object created once, reused across screens
- WiFi icon created once, reused
- Data icon created once, reused
- Only content area is recreated per screen

### 3. Screen-Specific Components

#### 3.1 QR Code Screen
**File**: `include/qr_code_screen.h` / `src/qr_code_screen.cpp`

**Content**: QR code centered in content area

**Functions**:
- `qr_code_screen_init()` - Initialize QR code
- `qr_code_screen_update()` - Update QR code if needed
- `qr_code_screen_cleanup()` - Clean up QR code

#### 3.2 Pouring Screen
**File**: `include/pouring_screen.h` / `src/pouring_screen.cpp`

**Content**: Flow rate, volume, cost information

**Functions**:
- `pouring_screen_init()` - Initialize pouring display
- `pouring_screen_update()` - Update flow rate, volume, cost
- `pouring_screen_set_params()` - Set pour parameters from MQTT
- `pouring_screen_cleanup()` - Clean up pouring display

#### 3.3 Finished Screen
**File**: `include/finished_screen.h` / `src/finished_screen.cpp`

**Content**: Completion message, final volume, final cost

**Functions**:
- `finished_screen_init()` - Initialize finished display
- `finished_screen_set_data()` - Set final pour data
- `finished_screen_update()` - Handle timeout countdown
- `finished_screen_cleanup()` - Clean up finished display

### 4. Shared UI Components

#### 4.1 Logo Component
**File**: `include/ui_logo.h` / `src/ui_logo.cpp`

**Purpose**: Shared logo image object

**Functions**:
- `ui_logo_create()` - Create logo object (returns pointer)
- `ui_logo_get_object()` - Get existing logo object
- `ui_logo_update()` - Update logo if needed (future: animations)

#### 4.2 WiFi Icon Component
**File**: `include/ui_wifi_icon.h` / `src/ui_wifi_icon.cpp`

**Purpose**: Shared WiFi status icon

**Functions**:
- `ui_wifi_icon_create()` - Create WiFi icon
- `ui_wifi_icon_update()` - Update WiFi status (connected/disconnected/flashing)
- `ui_wifi_icon_set_flashing()` - Enable/disable flashing animation

#### 4.3 Data Icon Component
**File**: `include/ui_data_icon.h` / `src/ui_data_icon.cpp`

**Purpose**: Shared MQTT/data connection status icon

**Functions**:
- `ui_data_icon_create()` - Create data icon
- `ui_data_icon_update()` - Update data status (connected/disconnected/active)
- `ui_data_icon_set_active()` - Show activity animation

## Implementation Plan

### Phase 1: Create Base Components
1. Create `base_screen` component with standard layout
2. Create shared UI components (logo, WiFi icon, data icon)
3. Extract common code from existing screens

### Phase 2: Refactor Existing Screens
1. Refactor `production_mode_ui` → `qr_code_screen` (use base_screen)
2. Refactor `pouring_mode_ui` → `pouring_screen` (use base_screen)
3. Create `finished_screen` component

### Phase 3: Implement Screen Manager
1. Create `screen_manager` with state machine
2. Integrate timeout handling for finished screen
3. Update `main.cpp` to use screen manager

### Phase 4: Cleanup
1. Remove old screen files
2. Update includes and references
3. Test all screen transitions

## File Structure

```
include/
  ├── base_screen.h          # Standard layout component
  ├── screen_manager.h       # Screen state management
  ├── qr_code_screen.h       # QR code screen
  ├── pouring_screen.h       # Pouring screen
  ├── finished_screen.h      # Finished screen
  ├── ui_logo.h              # Shared logo component
  ├── ui_wifi_icon.h         # Shared WiFi icon component
  └── ui_data_icon.h         # Shared data icon component

src/
  ├── base_screen.cpp
  ├── screen_manager.cpp
  ├── qr_code_screen.cpp
  ├── pouring_screen.cpp
  ├── finished_screen.cpp
  ├── ui_logo.cpp
  ├── ui_wifi_icon.cpp
  └── ui_data_icon.cpp
```

## Memory Optimization

### Current Memory Usage (per screen)
- Logo image data: ~44KB (shared, good)
- Logo UI object: ~200 bytes × 3 screens = 600 bytes
- WiFi icon objects: ~800 bytes × 3 screens = 2.4KB
- Data icon objects: ~800 bytes × 3 screens = 2.4KB
- **Total duplication**: ~5.4KB

### Optimized Memory Usage
- Logo image data: ~44KB (shared)
- Logo UI object: ~200 bytes × 1 = 200 bytes
- WiFi icon objects: ~800 bytes × 1 = 800 bytes
- Data icon objects: ~800 bytes × 1 = 800 bytes
- **Total**: ~46KB (saves ~5.4KB)

### Additional Benefits
- Faster screen transitions (reuse objects)
- Consistent UI appearance
- Easier maintenance (single source of truth)

## State Machine Flow

```
┌─────────────┐
│   SPLASH    │ (startup, auto-transition)
└──────┬──────┘
       │
       ↓
┌─────────────┐
│  QR_CODE    │ (waiting for payment)
└──────┬──────┘
       │ MQTT "paid" message
       ↓
┌─────────────┐
│  POURING    │ (active pouring)
└──────┬──────┘
       │ pouring completes
       ↓
┌─────────────┐
│  FINISHED   │ (completion, timeout: 5-10s)
└──────┬──────┘
       │ timeout
       ↓
┌─────────────┐
│  QR_CODE    │ (back to waiting)
└─────────────┘
```

## Configuration

### Timeout Settings
```cpp
#define FINISHED_SCREEN_TIMEOUT_MS 5000  // 5 seconds
```

### Layout Constants
```cpp
#define BASE_SCREEN_LOGO_HEIGHT 90
#define BASE_SCREEN_CONTENT_Y 100
#define BASE_SCREEN_ICON_SIZE 20
#define BASE_SCREEN_ICON_MARGIN 10
```

## Testing Checklist

- [ ] Splashscreen displays correctly
- [ ] QR code screen shows logo, QR code, WiFi icon, data icon
- [ ] MQTT "paid" message transitions to pouring screen
- [ ] Pouring screen updates flow rate, volume, cost
- [ ] Pouring completion transitions to finished screen
- [ ] Finished screen shows completion message
- [ ] Finished screen timeout returns to QR code screen
- [ ] WiFi icon updates correctly on all screens
- [ ] Data icon updates correctly on all screens
- [ ] Screen transitions are smooth (no flicker)
- [ ] Memory usage is optimized (shared components)

## Migration Notes

1. **Keep old files during migration** - Rename to `.old` for reference
2. **Gradual migration** - Implement new components alongside old ones
3. **Test each phase** - Verify functionality before moving to next phase
4. **Update main.cpp last** - Switch to screen manager after all components are ready
