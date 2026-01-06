#!/usr/bin/env python3

# Note names
NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

# Scale degrees
ROMAN_NUMERALS = ['I', 'II', 'III', 'IV', 'V', 'VI', 'VII']
MINOR_ROMAN = ['i', 'ii°', 'III', 'iv', 'v', 'VI', 'VII']

# Major scale intervals
MAJOR_SCALE = [0, 2, 4, 5, 7, 9, 11]

# Natural minor scale intervals
MINOR_SCALE = [0, 2, 3, 5, 7, 8, 10]

def analyze_chord_in_key(midi_notes, key_root=0, is_major=True):
    """Analyze a chord relative to a key and return scale degree and quality"""
    if not midi_notes:
        return "Empty", 0, "Major"
    
    # Get pitch classes (0-11) and find bass note
    bass_note = min(midi_notes) % 12
    pitch_classes = sorted(set(n % 12 for n in midi_notes))
    
    # Use bass note as root (or lowest note)
    root = bass_note
    
    # Calculate intervals from root
    intervals = sorted(set((pc - root) % 12 for pc in pitch_classes))
    intervals_set = set(intervals)
    
    # Determine chord quality
    quality_name = "Unknown"
    quality_code = "Major"
    
    if intervals_set == {0, 4, 7}:
        quality_name = "Major"
        quality_code = "Major"
    elif intervals_set == {0, 3, 7}:
        quality_name = "Minor"
        quality_code = "Minor"
    elif intervals_set == {0, 3, 6}:
        quality_name = "Diminished"
        quality_code = "Diminished"
    elif intervals_set == {0, 4, 8}:
        quality_name = "Augmented"
        quality_code = "Major"
    elif intervals_set == {0, 2, 7}:
        quality_name = "sus2"
        quality_code = "Sus2"
    elif intervals_set == {0, 5, 7}:
        quality_name = "sus4"
        quality_code = "Sus4"
    elif intervals_set == {0, 4, 7, 11}:
        quality_name = "Maj7"
        quality_code = "Major7"
    elif intervals_set == {0, 3, 7, 10}:
        quality_name = "min7"
        quality_code = "Minor7"
    elif intervals_set == {0, 4, 7, 10}:
        quality_name = "Dom7"
        quality_code = "Dominant7"
    elif intervals_set == {0, 3, 6, 10}:
        quality_name = "m7b5"
        quality_code = "HalfDiminished7"
    else:
        quality_name = f"({'-'.join(str(i) for i in intervals)})"
        quality_code = "Major"
    
    # Find scale degree
    scale = MAJOR_SCALE if is_major else MINOR_SCALE
    degree_offset = (root - key_root) % 12
    
    # Find which scale degree this is
    scale_degree = 0
    for i, interval in enumerate(scale):
        if interval == degree_offset:
            scale_degree = i + 1
            break
    
    # If not found in scale, find closest
    if scale_degree == 0:
        min_dist = 12
        for i, interval in enumerate(scale):
            dist = min(abs(degree_offset - interval), 12 - abs(degree_offset - interval))
            if dist < min_dist:
                min_dist = dist
                scale_degree = i + 1
    
    root_name = NOTE_NAMES[root]
    roman = ROMAN_NUMERALS[scale_degree - 1] if is_major else MINOR_ROMAN[scale_degree - 1]
    
    return f"{root_name} {quality_name} ({roman})", scale_degree, quality_code

# Read all progressions from extracted file
import re

with open('extracted_progressions.txt', 'r') as f:
    content = f.read()

# Extract all case blocks
case_pattern = r'case (\d+): // (.+?)\n.*?c1\.midiNotes = \{([^}]+)\}.*?c2\.midiNotes = \{([^}]+)\}.*?c3\.midiNotes = \{([^}]+)\}.*?c4\.midiNotes = \{([^}]+)\}'

matches = re.findall(case_pattern, content, re.DOTALL)

print("Analyzing all progressions in A minor (key root = A = 9):")
print("=" * 80)

KEY_ROOT = 9  # A
IS_MAJOR = False  # Minor key

for match in matches[:10]:  # First 10 progressions
    case_num, filename = match[0], match[1]
    chords = [[int(n.strip()) for n in match[i].split(',')] for i in range(2, 6)]
    
    print(f"\nCase {case_num}: {filename}")
    
    cpp_output = []
    for j, chord_notes in enumerate(chords, 1):
        analysis, degree, quality = analyze_chord_in_key(chord_notes, KEY_ROOT, IS_MAJOR)
        notes_str = ', '.join(str(n) for n in chord_notes)
        print(f"  Chord {j}: {analysis}")
        cpp_output.append(f"        AnalyzedChord c{j}; c{j}.midiNotes = {{{notes_str}}}; c{j}.scaleDegree = {degree}; c{j}.quality = KeyManager::ChordType::{quality}; c{j}.inversion = 0;")
    
    print("  C++ code:")
    for line in cpp_output:
        print(line)
