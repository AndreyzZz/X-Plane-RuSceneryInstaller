# X-Plane RuScenery Installer - Copilot Instructions

## Repository Overview

**Project**: X-Plane RuScenery Installer  
**Purpose**: A cross-platform GUI installer application for downloading and installing Russian scenery, airport facilities, buildings, and Soviet/CIS-era aircraft for the X-Plane flight simulator.  
**License**: GNU General Public License v3.0  
**Repository Size**: Small (~168 KB)  
**Default Branch**: `master`

## Technology Stack

- **Primary Language**: C++ (96.9%)
- **Build System**: Qt qmake (Qt-based project)
- **UI Framework**: Qt Widgets
- **Additional Languages**: Prolog (3.1%, minimal)
- **Platform Support**: Cross-platform (Windows, macOS, Linux)
- **Key Features**:
  - Qt GUI application with QWidget-based UI
  - Network operations via QNetworkAccessManager for downloading files
  - Qt Internationalization (Russian translation included)
  - Settings persistence using QSettings
  - File I/O with progress tracking

## Project Structure

```
X-Plane-RuSceneryInstaller/
├── README                          # Brief project description (Russian)
├── LICENSE.txt                     # GPLv3 license
├── src/                           # All source code directory
│   ├── RuSceneryInstaller.pro     # Qt project file (qmake configuration)
│   ├── main.cpp                   # Application entry point
│   ├── widget.h                   # Main window header
│   ├── widget.cpp                 # Main window implementation (core logic)
│   ├── widget.ui                  # Qt Designer UI definition
│   ├── RuSceneryInstaller_ru.ts   # Russian translation file
│   ├── RuSceneryInstaller.qrc     # Qt resource file
│   ├── RuSceneryInstaller.rc      # Windows resource file
│   ├── RuSceneryInstaller.ico     # Windows icon
│   └── RuSceneryInstaller.icns    # macOS icon
```

## Key Source Files

### RuSceneryInstaller.pro (Qt Project File)
- Defines build configuration: `QT += core gui network`
- Sets application name: "RuSceneryInstaller"
- Links against: Qt Core, Qt GUI, Qt Network libraries
- Includes translation file: RuSceneryInstaller_ru.ts
- Platform-specific configurations:
  - Windows: Uses RC_FILE (resource file)
  - macOS: Uses ICON (icns file)
  - Linux/Unix: Sets RPATH for library linking

### main.cpp (Entry Point)
- Initializes QApplication with organization/app metadata
- Sets codec to UTF-8 for all text operations (critical for Russian text)
- Loads Russian translator if system language is Russian
- Creates and shows the main Widget window

### widget.h & widget.cpp (Core Logic)
The Widget class implements the main installer functionality:
- **UI Components**: Line edit for path selection, progress bars, labels
- **Configuration**: Stores X-Plane directory path and download URLs in INI file
- **Network Operations**: Uses QNetworkAccessManager to download version files and scenery data
- **State Management**: Tracks installation status (NotStarted, Installing, Finished, AbortWithError, etc.)
- **Download Mechanism**:
  1. Fetches version metadata file from remote server (max 3 MB)
  2. Parses file list and validates against local cache
  3. Downloads missing files sequentially (max 100 MB per file)
  4. Calculates download speed and ETA in real-time
- **Error Handling**: Validates file sizes, handles network errors, provides user feedback
- **Supported URLs**: http://www.x-plane.su/ruscenery/ (default, configurable)

### widget.ui (Qt Designer UI File)
XML-based UI definition with:
- X-Plane directory input field with browse button
- Progress bars for current file and overall installation
- Download status and ETA display
- Install/Abort button
- Update notification label

## Build Instructions

### Prerequisites
- **Qt Framework**: Qt 4.8+ or Qt 5.x (qmake-based)
- **Compiler**: GCC (Linux), Clang (macOS), MSVC (Windows) - any C++11 compatible compiler
- **Build Tools**: qmake, make (or nmake on Windows)

### Building on All Platforms

1. **Clone and navigate to source directory**:
   ```bash
   cd X-Plane-RuSceneryInstaller/src
   ```

2. **Generate Makefile from .pro file**:
   ```bash
   qmake RuSceneryInstaller.pro
   ```

3. **Compile**:
   ```bash
   make
   ```
   (or `nmake` on Windows with MSVC, `make` with MinGW)

4. **Output**:
   - Linux/macOS: Executable at `src/RuSceneryInstaller`
   - Windows: Executable at `src/RuSceneryInstaller.exe`

### Runtime Dependencies
- Qt libraries (Core, Gui, Network) must be available in system PATH or bundled with executable
- On Linux: May require additional development packages (libqt4-dev or qt5-qmake)

## Known Behaviors and Important Notes

### UTF-8 Encoding
- The application **requires** UTF-8 codec setup in main.cpp
- Russian translations depend on proper codec configuration
- Do not modify the `QTextCodec::setCodecForTr()` and `QTextCodec::setCodecForCStrings()` calls

### Settings Persistence
- Application settings stored in: `{app_directory}/RuSceneryInstaller.ini`
- Stores: User-selected X-Plane directory and custom download URL
- Settings only saved if installation button is enabled (validation passed)

### Directory Structure Validation
Widget validates that selected X-Plane directory contains:
- `X-Plane.exe` (Windows), `X-Plane` (Linux), or `X-Plane.app` (macOS) executable
- `Custom Scenery/` subdirectory

Installation only proceeds if both exist.

### Network Download Details
- **Version File**: Downloaded first from `{url}ruscenery.ver` (plain text, max 3 MB)
- **Format**: Each line is space-separated: `{filename} {size} {date} {time}` or commands like `;u`, `;d`, `;v`
- **File List**: Only files NOT matching local cache (by size) are added to download queue
- **Download Speed**: Calculated every 10 seconds for ETA estimation
- **Maximum File Size**: 100 MB per file

### Error States
The application defines multiple error states with specific handling:
- `AbortByUser`: User clicked abort button
- `AbortWithError`: Network/file I/O error with user notification
- `AbortByApplication`: Application exiting during installation
- `Finished`: Installation completed successfully

### Configuration Variables
Key hardcoded values in widget.h:
- `SPEED_ESTIMATION_TIME = 10` (seconds)
- `MEGABYTE = 1048576.0`
- `KILOBYTE = 1024.0`
- `maxVersionFileSize = 3*1024*1024` (bytes)
- `maxDownloadFileSize = 100*1024*1024` (bytes)

## Testing & Validation

**No automated test suite exists** in the repository. Manual validation should include:
1. Verify compilation completes without errors
2. Test UI renders correctly on target platform
3. Verify file dialogs work properly
4. Test network operations with mock or real server (default is x-plane.su)
5. Verify Russian text displays correctly in UI and messages
6. Test installation abort functionality

## Important Code Patterns

### Qt Signal/Slot Connections
- Network operations use `QNetworkAccessManager` with signals for `readyRead()` and `finished()`
- Timer uses `timeout()` signal for speed estimation
- UI elements use `clicked()` and `textChanged()` signals
- Always ensure slots are properly connected before operations start

### File Handling
- Files are opened in `WriteOnly` mode during download
- Path validation uses `QDir().mkpath()` to create directories
- File size validation occurs before and after download completion

### Memory Management
- Network replies deleted with `deleteLater()` (safe Qt pattern)
- File objects cleaned up in `abort_install()` method
- Settings object deleted in destructor
- Use `new` for heap allocation; cleanup in destructor or abort methods

## Trust These Instructions

Follow these instructions for all code modifications. The information provided reflects the actual repository structure and build configuration. Only search for additional information if:
- You cannot compile following the steps above
- A required dependency version is unclear for your specific platform
- You need to understand undocumented platform-specific behavior
- The repository has been significantly modified since these instructions were written
