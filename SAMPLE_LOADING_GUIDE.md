# Sample Loading Guide

## Overview
The synth has been replaced with a sampler that loads AIF/AIFF audio files. Each file represents a single note/key that will be triggered when that MIDI note plays.

## Setup Instructions

### 1. Prepare Your AIF Files
Place all your AIF/AIFF files in the `assets` folder at the root of the project:
```
chord_gen_plugin/
├── assets/
│   ├── 60.aif     (or C4.aif, or note60.aif)
│   ├── 61.aif     (or C#4.aif)
│   ├── 62.aif     (or D4.aif)
│   └── ...
├── Source/
├── Builds/
└── ...
```

### 2. File Naming Conventions
The sampler supports three naming formats:

**Option 1: MIDI Note Numbers (Recommended)**
- `60.aif` → Middle C (C4)
- `61.aif` → C#4
- `62.aif` → D4
- etc.

**Option 2: Note Names**
- `C4.aif` → Middle C
- `C#4.aif` or `Db4.aif` → C#4
- `D4.aif` → D4
- `A3.aif` → A below middle C
- etc.

**Option 3: "note" Prefix**
- `note60.aif` → Middle C
- `note61.aif` → C#4
- etc.

### 3. MIDI Note Reference
Common MIDI note numbers:
- C0 = 12
- C1 = 24
- C2 = 36
- C3 = 48
- **C4 (Middle C) = 60**
- C5 = 72
- C6 = 84
- C7 = 96
- C8 = 108

For full keyboard: You'll need samples from MIDI 21 (A0) to MIDI 108 (C8), or at minimum the range you plan to use.

### 4. Sample Requirements
- **Format**: AIF or AIFF
- **Sample Rate**: Any (will be automatically handled)
- **Channels**: Mono or Stereo
- **Bit Depth**: 16-bit or 24-bit recommended

## How It Works

1. **On Startup**: The application looks for the `assets` folder
2. **Sample Loading**: All AIF/AIFF files are scanned and loaded into memory
3. **Note Mapping**: Each file is mapped to its MIDI note based on the filename
4. **Playback**: When a MIDI note is triggered, the corresponding sample plays
5. **Envelope**: A fast attack and moderate release envelope is applied automatically

## Debugging

If samples aren't loading, check the debug console for messages:
- `"Loading samples from: [path]"` - Shows where it's looking
- `"Found X audio files"` - Shows how many files were detected
- `"Loaded sample: [filename] -> MIDI note X"` - Confirms each successful load
- `"Could not parse MIDI note from filename: [name]"` - Shows problem files
- `"Warning: assets folder not found"` - The assets folder doesn't exist

## Advanced: Multiple Velocity Layers

To add velocity layers in the future, you could organize samples like:
```
assets/
├── C4_soft.aif
├── C4_medium.aif
├── C4_loud.aif
└── ...
```
(This would require code modifications to support)

## Tips

1. **Start Small**: Test with just a few notes first (e.g., C4, D4, E4)
2. **Memory Usage**: More samples = more RAM usage. Consider your target system.
3. **Sample Quality**: Higher quality samples will sound better but take more memory
4. **Looping**: Samples play once through. For sustained notes, ensure your samples are long enough or implement looping.

## Troubleshooting

**Problem**: No sound when playing
- Check that the assets folder exists
- Verify AIF files are properly named
- Look at debug console for loading messages
- Ensure audio device is properly configured

**Problem**: Some notes work, others don't
- Check that all MIDI notes in your range have corresponding AIF files
- Verify file naming matches one of the supported formats

**Problem**: Audio quality issues
- Check your AIF file sample rate matches the project
- Verify bit depth is adequate (16-bit minimum)
- Ensure files aren't corrupted
