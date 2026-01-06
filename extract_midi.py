#!/usr/bin/env python3
import struct
import glob
import os

def parse_midi_file(filename):
    """Extract note-on events from a MIDI file."""
    with open(filename, 'rb') as f:
        data = f.read()
    
    chords = []
    current_chord = []
    i = 0
    
    while i < len(data) - 2:
        # Look for note-on events (0x90 channel 0)
        if data[i] == 0x90:
            # Collect all notes with delta-time 0 (simultaneous)
            note = data[i+1]
            velocity = data[i+2]
            
            if velocity > 0:
                current_chord.append(note)
            
            i += 3
            
            # Check if next byte is 0x00 (delta time 0) followed by another note
            while i < len(data) - 2 and data[i] == 0x00:
                i += 1
                if i < len(data) - 2:
                    next_note = data[i]
                    next_vel = data[i+1]
                    if next_vel > 0 and next_note < 128:  # Valid MIDI note
                        current_chord.append(next_note)
                        i += 2
                    else:
                        break
        # Look for note-off events (0x80) - chord is complete
        elif data[i] == 0x80:
            if current_chord:
                chords.append(sorted(current_chord))
                current_chord = []
            i += 3
        else:
            i += 1
    
    # Add last chord if any
    if current_chord:
        chords.append(sorted(current_chord))
    
    return chords

# Process all MIDI files
midi_dir = "Chord progression 4 bar"
files = sorted(glob.glob(os.path.join(midi_dir, "*.mid")))

print("// Extracted chord progressions from MIDI files")
print("std::vector<AnalyzedChord> analyzedChords;")
print()

for idx, filepath in enumerate(files, 1):
    basename = os.path.basename(filepath)
    chords = parse_midi_file(filepath)
    
    print(f"case {idx + 1}: // {basename}")
    print("    {")
    
    for chord_idx, chord in enumerate(chords, 1):
        notes_str = ", ".join(str(n) for n in chord)
        print(f"        AnalyzedChord c{chord_idx}; c{chord_idx}.midiNotes = {{{notes_str}}}; "
              f"c{chord_idx}.scaleDegree = 1; c{chord_idx}.quality = KeyManager::ChordType::Major; "
              f"c{chord_idx}.inversion = 0;")
    
    chord_vars = ", ".join(f"c{i}" for i in range(1, len(chords) + 1))
    print(f"        analyzedChords = {{{chord_vars}}};")
    print("    }")
    print("    break;")
    print()
