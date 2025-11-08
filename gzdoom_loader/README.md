# GZDoom Loader

A cross-platform, native GUI launcher for GZDoom built with ZWidget.

## Features

### Core Functionality
- **IWAD Selection**: Browse and select your base game WAD file (Doom, Doom 2, Heretic, Hexen, etc.)
- **PWAD/PK3 Management**: Add multiple mod files with proper load order control
- **File Reordering**: Move files up/down in the load order (critical for compatibility)
- **GZDoom Executable**: Configure path to your GZDoom installation

### Launch Options
- **Skill Level**: Select difficulty (1-5)
- **Warp**: Jump directly to a specific map
- **Custom Parameters**: Add any additional command-line arguments

### Preset System
- **Save Configurations**: Store your favorite mod combinations
- **Quick Load**: One-click loading of saved presets
- **Preset Management**: Delete unwanted presets
- **Persistent Storage**: All settings saved to `gzdoom_launcher_config.txt`

### Cross-Platform
- **Linux**: X11 native backend with DBus file dialogs
- **Windows**: Win32 native backend with native file dialogs
- **macOS**: Cocoa native backend with native file dialogs
- **SDL Support**: Optional SDL2/SDL3 backends

## Building

### Requirements
- CMake 3.11+
- C++20 compatible compiler
- Platform-specific dependencies:
  - **Linux**: libx11-dev, libxi-dev, fontconfig, glib-2.0
  - **Windows**: Windows SDK
  - **macOS**: Xcode Command Line Tools

### Build Instructions

```bash
# Clone or navigate to the ZWidget repository
cd ZWidget

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the GZDoom loader
make gzdoom_loader -j$(nproc)

# Run the launcher
./gzdoom_loader
```

### CMake Options

```bash
# Disable the GZDoom loader if you only want ZWidget library
cmake -DZWIDGET_BUILD_GZDOOM_LOADER=OFF ..

# Disable the example application
cmake -DZWIDGET_BUILD_EXAMPLE=OFF ..
```

## Usage

1. **First Time Setup**:
   - Click "Browse..." next to "GZDoom Executable" and select your GZDoom binary
   - Click "Browse..." next to "IWAD" and select your base game WAD

2. **Adding Mods**:
   - Click "Add Files..." under the mods section
   - Select one or more PWAD/PK3 files (hold Ctrl/Cmd for multi-select)
   - Use "Move Up"/"Move Down" to adjust load order
   - Click "Remove" to delete selected files from the list

3. **Configuring Launch Options**:
   - Select skill level from the dropdown
   - Enter a map number in "Warp" field (e.g., "01" for MAP01)
   - Add any custom parameters in the "Custom Params" field

4. **Using Presets**:
   - Configure your IWAD, mods, and settings
   - Click "Save" to create a new preset
   - Select a preset from the dropdown to auto-load it
   - Click "Delete" to remove unwanted presets

5. **Launch**:
   - Click "LAUNCH GZDOOM" to start the game with your configuration

## Command-Line Generation

The launcher generates proper GZDoom command lines following this format:

```bash
gzdoom -iwad "path/to/doom2.wad" -file "mod1.pk3" "mod2.wad" -skill 4 -warp 01
```

## File Structure

```
gzdoom_loader/
├── main.cpp                  # Entry point
├── gzdoom_launcher.h         # Main window class header
├── gzdoom_launcher.cpp       # Main window implementation
└── README.md                 # This file

Generated files:
├── gzdoom_launcher_config.txt  # Saved configuration and presets
```

## Configuration File Format

The launcher saves configurations in a simple text format:

```
GZDOOM_PATH=/path/to/gzdoom
PRESET_COUNT=1
PRESET_NAME=Brutal Doom
PRESET_IWAD=/path/to/doom2.wad
PRESET_GZDOOM=/path/to/gzdoom
PRESET_SKILL=4
PRESET_WARP=01
PRESET_CUSTOM=
PRESET_PWAD_COUNT=1
PRESET_PWAD=/path/to/brutaldoom.pk3
```

## Known Limitations

- No placeholder text in input fields (ZWidget limitation)
- Preset names are auto-generated (manual naming not yet implemented)
- No WAD metadata display (planned for future)
- No drag-and-drop support (depends on ZWidget backend)

## Future Enhancements

### Phase 2 Features
- IWAD auto-detection in common directories
- Multiple source port support (Zandronum, LZDoom, etc.)
- Recent configurations history
- Custom preset naming

### Phase 3 Features
- Directory scanning for available WADs
- WAD metadata display (author, map count, etc.)
- Preview screenshots
- Compatibility warnings

### Phase 4 Features
- Drag-and-drop file reordering
- Theme customization
- Command-line argument validation
- Process monitoring (show GZDoom output)

## Technical Details

### Architecture
- **Main Window**: `GZDoomLauncher` class inheriting from `Widget`
- **UI Framework**: ZWidget (native widgets, not OpenGL/SDL-based)
- **Process Launching**:
  - Windows: CreateProcess API
  - Unix: fork/exec
- **File Dialogs**: Native system dialogs via ZWidget

### Dependencies
Uses only ZWidget components:
- `TextLabel` - Static text display
- `LineEdit` - Text input fields
- `PushButton` - Clickable buttons
- `ListView` - File list display
- `Dropdown` - Skill level and preset selection
- `OpenFileDialog` - Native file picker

### Load Order Importance

Doom mod load order matters! The launcher preserves file order because:
- Later files override earlier files
- Some mods depend on specific load order
- Maps should typically load before gameplay mods

Recommended order:
1. Maps (megawads)
2. Music packs
3. Gameplay mods (Brutal Doom, Project Brutality, etc.)
4. Patches and fixes
5. HUD/UI mods

## License

This GZDoom Loader follows the same license as ZWidget (zlib license).

## Contributing

Contributions welcome! Areas for improvement:
- IWAD auto-detection algorithms
- Better preset management UI
- WAD file parsing for metadata
- Additional source port profiles

## Credits

- Built with [ZWidget](https://github.com/dpjudas/ZWidget) by dpjudas
- Created for the Doom community
- Inspired by ZDL, Doom Runner, and Rocket Launcher 2
