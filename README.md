# Alex Rome - Chord Builder

A JUCE-based audio plugin featuring an emotion wheel for generating chords based on musical moods and tonalities.

## Building for macOS

### Prerequisites

- macOS (tested on macOS 10.15+)
- Xcode (download from the App Store or Apple Developer)
- Xcode Command Line Tools

To install Command Line Tools if not already installed:
```bash
xcode-select --install
```

### Build Steps

#### Option 1: Using Xcode (Recommended)

1. Navigate to the Xcode project directory:
   ```bash
   cd Builds/MacOSX
   ```

2. Open the project in Xcode:
   ```bash
   open NewProject.xcodeproj
   ```

3. In Xcode:
   - Select your build scheme (Debug or Release)
   - Choose your target device/architecture
   - Press `Cmd+B` to build, or `Cmd+R` to build and run

#### Option 2: Using Command Line

1. Navigate to the Xcode project directory:
   ```bash
   cd Builds/MacOSX
   ```

2. Build Debug version:
   ```bash
   xcodebuild -project NewProject.xcodeproj -configuration Debug build
   ```

3. Build Release version:
   ```bashp
   xcodebuild -project NewProject.xcodeproj -configuration Release build
   ```

4. Build as a standalone macOS App:
   ```bash
   xcodebuild -project NewProject.xcodeproj -configuration Release -target "NewProject - App" build
   ```

#### Option 3: Using VS Code Tasks

If you're using VS Code, you can use the pre-configured build tasks:

1. Press `Cmd+Shift+P` to open the command palette
2. Type "Run Task" and select "Tasks: Run Task"
3. Choose one of:
   - `Build JUCE Project (macOS Debug)`
   - `Build JUCE Project (macOS Release)`
   - `Clean JUCE Project (macOS)`

Or use the keyboard shortcut:
- `Cmd+Shift+B` to run the default build task

### Build Output

After a successful build, you'll find the built application in:
- Debug builds: `Builds/MacOSX/build/Debug/`
- Release builds: `Builds/MacOSX/build/Release/`
- Standalone App: `Builds/MacOSX/build/Release/NewProject.app`

#### Running the Standalone App

Once built, you can run the app directly:
```bash
open Builds/MacOSX/build/Release/NewProject.app
```

Or copy it to your Applications folder:
```bash
cp -r Builds/MacOSX/build/Release/NewProject.app /Applications/
```

#### Creating a Distribution Package

To package the app for distribution to others:

1. Create a zip file of the app:
   ```bash
   cd Builds/MacOSX/build/Release
   zip -r AlexRome-ChordBuilder.zip NewProject.app
   ```

2. The zip file will be created in the same directory. You can now share `AlexRome-ChordBuilder.zip` with others.

**For users installing the app:**
1. Download and unzip `AlexRome-ChordBuilder.zip`
2. Drag `NewProject.app` to the Applications folder
3. On first launch, right-click the app and select "Open" to bypass Gatekeeper (if not code-signed)

**Note:** For wider distribution, consider code-signing the app with an Apple Developer account to avoid security warnings.

### Troubleshooting

**Error: "xcodebuild: command not found"**
- Install Xcode Command Line Tools: `xcode-select --install`

**Error: "No such file or directory"**
- Make sure you're in the correct directory
- Verify the project structure matches the paths above

**Build fails with missing dependencies**
- Ensure all JUCE modules are properly included in the `JuceLibraryCode/` directory
- Check that the `.jucer` file is properly configured

### Project Structure

```
chord_gen_plugin/
├── Source/                 # Source code files
│   ├── EmotionWheel.h     # Emotion wheel chord definitions
│   ├── MainComponent.cpp  # Main UI component
│   ├── MainComponent.h
│   └── ...
├── Builds/
│   └── MacOSX/            # macOS-specific build files
│       └── NewProject.xcodeproj/
├── JuceLibraryCode/       # JUCE framework modules
└── assets/                # Asset files
```

## Features

- Emotion-based chord generation
- Major and Minor tonality wheels
- 6 emotion categories: Happy, Sad, Warm, Tense, Calm, Dark
- 4 chord variations per emotion/tonality combination
- Real-time chord preview on hover
- Modern UI with theming support

## Development

To modify the emotion wheel chords, edit `Source/EmotionWheel.h` and update the interval definitions in the `initializeEmotions()` method.


# Build ARM64 Release
cd /Users/zac/Programming/chord_gen_plugin/Builds/MacOSX && \
xcodebuild -project NewProject.xcodeproj -configuration Release -arch arm64 build && \
\
rm -rf "build/Release/NewProject.app/Contents/Resources/assets" && \
mkdir -p "build/Release/NewProject.app/Contents/Resources/assets" && \
cp /Users/zac/Programming/chord_gen_plugin/assets/*.aif "build/Release/NewProject.app/Contents/Resources/assets/" && \
cd build/Release && \
rm -f ChordBuilder-ARM64.zip && \
zip -r ChordBuilder-ARM64.zip NewProject.app