#include "KeyManager.h"

//==============================================================================
// KeyManager Implementation

KeyManager::KeyManager() : currentKey(Key::C), currentTonality(Tonality::Major), preferSharps(true)
{
    noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    majorScalePattern = {0, 2, 4, 5, 7, 9, 11}; // W-W-H-W-W-W-H pattern
    minorScalePattern = {0, 2, 3, 5, 7, 8, 10}; // W-H-W-W-H-W-W pattern (natural minor)
    initializeProgressions();
}

void KeyManager::setCurrentKey(Key key, Tonality tonality, bool preferSharps)
{
    currentKey = key;
    currentTonality = tonality;
    this->preferSharps = preferSharps;
}

KeyManager::Key KeyManager::getCurrentKey() const
{
    return currentKey;
}

KeyManager::Tonality KeyManager::getCurrentTonality() const
{
    return currentTonality;
}

std::string KeyManager::getKeyName(Key key) const
{
    return noteNames[static_cast<int>(key)];
}

std::string KeyManager::getKeyName(Key key, Tonality tonality) const
{
    std::string name = noteNames[static_cast<int>(key)];
    if (tonality == Tonality::Minor)
        name += "m";
    return name;
}

std::vector<int> KeyManager::getScaleNotes() const
{
    std::vector<int> scaleNotes;
    int rootNote = static_cast<int>(currentKey);
    const auto& scalePattern = (currentTonality == Tonality::Major) ? majorScalePattern : minorScalePattern;
    
    for (int interval : scalePattern)
    {
        scaleNotes.push_back((rootNote + interval) % 12);
    }
    
    return scaleNotes;
}

std::vector<std::string> KeyManager::getScaleNoteNames() const
{
    std::vector<std::string> names;
    auto notes = getScaleNotes();
    
    for (int note : notes)
    {
        names.push_back(noteNames[note]);
    }
    
    return names;
}

std::vector<std::string> KeyManager::getScaleNoteNamesWithProperSpelling() const
{
    std::vector<std::string> properNames;
    int rootNote = static_cast<int>(currentKey);
    const auto& scalePattern = (currentTonality == Tonality::Major) ? majorScalePattern : minorScalePattern;
    
    // Letter names cycle through the musical alphabet
    const std::vector<std::string> letterNames = {"C", "D", "E", "F", "G", "A", "B"};
    
    // Determine starting letter based on root note and sharp/flat preference
    int startingLetter = 0;
    switch (rootNote)
    {
        case 0: startingLetter = 0; break; // C
        case 1: startingLetter = preferSharps ? 0 : 1; break; // C# (starts on C) or Db (starts on D)
        case 2: startingLetter = 1; break; // D
        case 3: startingLetter = preferSharps ? 1 : 2; break; // D# (starts on D) or Eb (starts on E)
        case 4: startingLetter = 2; break; // E
        case 5: startingLetter = 3; break; // F
        case 6: startingLetter = preferSharps ? 3 : 4; break; // F# (starts on F) or Gb (starts on G)
        case 7: startingLetter = 4; break; // G
        case 8: startingLetter = preferSharps ? 4 : 5; break; // G# (starts on G) or Ab (starts on A)
        case 9: startingLetter = 5; break; // A
        case 10: startingLetter = preferSharps ? 5 : 6; break; // A# (starts on A) or Bb (starts on B)
        case 11: startingLetter = 6; break; // B
    }
    
    // Build scale with proper letter sequence
    for (size_t i = 0; i < scalePattern.size(); ++i)
    {
        int interval = scalePattern[i];
        int actualPitch = (rootNote + interval) % 12;
        int expectedLetter = (startingLetter + i) % 7;
        
        std::string noteName = letterNames[expectedLetter];
        
        // Calculate expected pitch for this letter
        const std::vector<int> naturalPitches = {0, 2, 4, 5, 7, 9, 11}; // C D E F G A B
        int naturalPitch = naturalPitches[expectedLetter];
        int difference = actualPitch - naturalPitch;
        
        // Handle wraparound
        if (difference < -6) difference += 12;
        if (difference > 6) difference -= 12;
        
        // Add accidentals
        if (difference == 1)
            noteName += "♯";
        else if (difference == 2)
            noteName += "♯♯";
        else if (difference == -1)
            noteName += "♭";
        else if (difference == -2)
            noteName += "♭♭";
        
        properNames.push_back(noteName);
    }
    
    return properNames;
}

std::vector<int> KeyManager::getChromaticNotes() const
{
    std::vector<int> chromaticNotes;
    for (int i = 0; i < 12; ++i)
    {
        chromaticNotes.push_back(i);
    }
    return chromaticNotes;
}

std::vector<std::string> KeyManager::getChromaticNoteNames() const
{
    return noteNames;
}

bool KeyManager::isNoteInKey(int note) const
{
    auto scaleNotes = getScaleNotes();
    return std::find(scaleNotes.begin(), scaleNotes.end(), note % 12) != scaleNotes.end();
}

std::vector<int> KeyManager::generateTriad(ScaleDegree degree) const
{
    std::vector<int> chord;
    auto scaleNotes = getScaleNotes();
    int degreeIndex = static_cast<int>(degree) - 1;
    
    if (degreeIndex >= 0 && degreeIndex < 7)
    {
        // Base octave (middle C is 60, so we start from C4)
        int baseOctave = 60;
        
        // Stack thirds diatonically using scale degrees
        // Root = 1st scale degree, Third = 3rd scale degree, Fifth = 5th scale degree
        int root = baseOctave + scaleNotes[degreeIndex];                    // Root (1st)
        int third = baseOctave + scaleNotes[(degreeIndex + 2) % 7];         // Third (3rd) 
        int fifth = baseOctave + scaleNotes[(degreeIndex + 4) % 7];         // Fifth (5th)
        
        // Ensure proper octave ordering (third and fifth above root)
        while (third <= root) third += 12;
        while (fifth <= third) fifth += 12;
        
        // Keep chords in a reasonable range
        while (root >= 84) { root -= 12; third -= 12; fifth -= 12; }
        while (root < 48) { root += 12; third += 12; fifth += 12; }
        
        chord.push_back(root);
        chord.push_back(third);
        chord.push_back(fifth);
    }
    
    return chord;
}

std::vector<int> KeyManager::generateSeventh(ScaleDegree degree) const
{
    std::vector<int> chord = generateTriad(degree);
    auto scaleNotes = getScaleNotes();
    int degreeIndex = static_cast<int>(degree) - 1;
    
    if (degreeIndex >= 0 && degreeIndex < 7 && !chord.empty())
    {
        // Base octave (middle C is 60, so we start from C4)
        int baseOctave = 60;
        
        // Add seventh diatonically - use the 7th scale degree from the root
        // This gives us proper diatonic seventh chords (not chromatic)
        int seventh = baseOctave + scaleNotes[(degreeIndex + 6) % 7];  // 7th scale degree
        
        // Ensure seventh is above the fifth (last note in chord)
        int fifth = chord.back();
        while (seventh <= fifth) seventh += 12;
        
        // Keep in reasonable range
        while (seventh >= 96) seventh -= 12;
        
        chord.push_back(seventh);
    }
    
    return chord;
}

std::vector<int> KeyManager::generateChord(ScaleDegree degree, ChordType type) const
{
    std::vector<int> chord;
    int rootNote = getNoteFromDegree(degree);
    auto intervals = getChordIntervals(type);
    
    // Base octave (middle C is 60, so we start from C4)
    int baseOctave = 60;
    
    for (int interval : intervals)
    {
        // Calculate the note in the proper octave
        int note = baseOctave + rootNote + interval;
        
        // Ensure we stay within MIDI range and don't go too high
        while (note >= 84) // Keep chords below C6
            note -= 12;
        while (note < 48)  // Keep chords above C3
            note += 12;
            
        chord.push_back(note);
    }
    
    return chord;
}

std::vector<std::vector<int>> KeyManager::generateProgression(const std::vector<ScaleDegree>& degrees) const
{
    return generateProgression(degrees, false); // Default to triads
}

std::vector<std::vector<int>> KeyManager::generateProgression(const std::vector<ScaleDegree>& degrees, bool useSevenths) const
{
    std::vector<std::vector<int>> progression;
    
    for (ScaleDegree degree : degrees)
    {
        if (useSevenths)
        {
            progression.push_back(generateSeventh(degree));
        }
        else
        {
            progression.push_back(generateTriad(degree));
        }
    }
    
    return progression;
}

std::vector<std::vector<int>> KeyManager::getCommonProgression(const std::string& progressionName) const
{
    return getCommonProgression(progressionName, false); // Default to triads
}

std::vector<std::vector<int>> KeyManager::getCommonProgression(const std::string& progressionName, bool useSevenths) const
{
    return getCommonProgression(progressionName, useSevenths, Voicing::Close);
}

std::vector<std::vector<int>> KeyManager::getCommonProgression(const std::string& progressionName, bool useSevenths, Voicing voicing) const
{
    auto it = commonProgressions.find(progressionName);
    if (it != commonProgressions.end())
    {
        auto progression = generateProgression(it->second, useSevenths);
        
        // Apply voicing to each chord in the progression
        for (auto& chord : progression)
        {
            chord = applyVoicing(chord, voicing);
        }
        
        return progression;
    }
    
    return {};
}

std::vector<int> KeyManager::applyVoicing(const std::vector<int>& chord, Voicing voicing) const
{
    if (chord.empty()) return chord;
    
    std::vector<int> voicedChord = chord;
    
    switch (voicing)
    {
        case Voicing::Close:
            // Default - already close position
            break;
            
        case Voicing::Open:
            // Spread notes: root stays, move other notes up by octave alternating
            if (voicedChord.size() >= 3)
            {
                voicedChord[1] += 12; // 3rd up an octave
                if (voicedChord.size() >= 4)
                    voicedChord[3] += 12; // 7th up an octave (if present)
            }
            break;
            
        case Voicing::Drop2:
            // Take 2nd highest note and drop it an octave
            if (voicedChord.size() >= 3)
            {
                int secondHighestIdx = voicedChord.size() - 2;
                voicedChord[secondHighestIdx] -= 12;
                std::sort(voicedChord.begin(), voicedChord.end());
            }
            break;
            
        case Voicing::Drop3:
            // Take 3rd highest note and drop it an octave
            if (voicedChord.size() >= 4)
            {
                int thirdHighestIdx = voicedChord.size() - 3;
                voicedChord[thirdHighestIdx] -= 12;
                std::sort(voicedChord.begin(), voicedChord.end());
            }
            break;
            
        case Voicing::FirstInversion:
            // Move root up an octave (3rd becomes bass)
            voicedChord[0] += 12;
            std::sort(voicedChord.begin(), voicedChord.end());
            break;
            
        case Voicing::SecondInversion:
            // Move root and 3rd up an octave (5th becomes bass)
            if (voicedChord.size() >= 3)
            {
                voicedChord[0] += 12; // Root up
                voicedChord[1] += 12; // 3rd up
                std::sort(voicedChord.begin(), voicedChord.end());
            }
            break;
            
        case Voicing::Spread:
            // Wide spacing - each note progressively higher
            for (size_t i = 1; i < voicedChord.size(); ++i)
            {
                voicedChord[i] += 12 * static_cast<int>(i);
            }
            break;
            
        case Voicing::RootPosition:
        default:
            // Keep as is
            break;
    }
    
    return voicedChord;
}

KeyManager::ChordType KeyManager::analyzeTriad(ScaleDegree degree) const
{
    if (currentTonality == Tonality::Major)
    {
        // In major key: I, IV, V are major; ii, iii, vi are minor; vii° is diminished
        switch (degree)
        {
            case ScaleDegree::I:
            case ScaleDegree::IV:
            case ScaleDegree::V:
                return ChordType::Major;
            case ScaleDegree::II:
            case ScaleDegree::III:
            case ScaleDegree::VI:
                return ChordType::Minor;
            case ScaleDegree::VII:
                return ChordType::Diminished;
            default:
                return ChordType::Major;
        }
    }
    else // Minor key
    {
        // In natural minor: i, iv, v are minor; III, VI, VII are major; ii° is diminished
        switch (degree)
        {
            case ScaleDegree::I:
            case ScaleDegree::IV:
            case ScaleDegree::V:
                return ChordType::Minor;
            case ScaleDegree::III:
            case ScaleDegree::VI:
            case ScaleDegree::VII:
                return ChordType::Major;
            case ScaleDegree::II:
                return ChordType::Diminished;
            default:
                return ChordType::Minor;
        }
    }
}

KeyManager::ChordType KeyManager::analyzeSeventh(ScaleDegree degree) const
{
    if (currentTonality == Tonality::Major)
    {
        // In major key seventh chord qualities
        switch (degree)
        {
            case ScaleDegree::I:
            case ScaleDegree::IV:
                return ChordType::Major7;
            case ScaleDegree::II:
            case ScaleDegree::III:
            case ScaleDegree::VI:
                return ChordType::Minor7;
            case ScaleDegree::V:
                return ChordType::Dominant7;
            case ScaleDegree::VII:
                return ChordType::HalfDiminished7;
            default:
                return ChordType::Major7;
        }
    }
    else // Minor key
    {
        // In natural minor key seventh chord qualities
        switch (degree)
        {
            case ScaleDegree::I:
            case ScaleDegree::IV:
            case ScaleDegree::V:
                return ChordType::Minor7;
            case ScaleDegree::III:
            case ScaleDegree::VI:
                return ChordType::Major7;
            case ScaleDegree::VII:
                return ChordType::Dominant7;
            case ScaleDegree::II:
                return ChordType::HalfDiminished7;
            default:
                return ChordType::Minor7;
        }
    }
}

std::string KeyManager::getChordName(ScaleDegree degree, ChordType type) const
{
    // Get the properly spelled note name based on the key signature
    auto properNoteNames = getScaleNoteNamesWithProperSpelling();
    int degreeIndex = static_cast<int>(degree) - 1;
    
    std::string rootName;
    if (degreeIndex >= 0 && degreeIndex < properNoteNames.size())
    {
        rootName = properNoteNames[degreeIndex];
    }
    else
    {
        rootName = noteNames[getNoteFromDegree(degree)]; // Fallback to sharps
    }
    
    switch (type)
    {
        case ChordType::Major: return rootName;
        case ChordType::Minor: return rootName + "m";
        case ChordType::Diminished: return rootName + "°";
        case ChordType::Augmented: return rootName + "+";
        case ChordType::Major7: return rootName + "M7";
        case ChordType::Minor7: return rootName + "m7";
        case ChordType::Dominant7: return rootName + "7";
        case ChordType::Diminished7: return rootName + "°7";
        case ChordType::HalfDiminished7: return rootName + "ø7";
        case ChordType::Sus2: return rootName + "sus2";
        case ChordType::Sus4: return rootName + "sus4";
        case ChordType::Add9: return rootName + "add9";
        case ChordType::Major9: return rootName + "M9";
        case ChordType::Minor9: return rootName + "m9";
        case ChordType::Dominant9: return rootName + "9";
        default: return rootName;
    }
}

int KeyManager::getNoteFromDegree(ScaleDegree degree) const
{
    auto scaleNotes = getScaleNotes();
    int degreeIndex = static_cast<int>(degree) - 1;
    
    if (degreeIndex >= 0 && degreeIndex < 7)
    {
        return scaleNotes[degreeIndex];
    }
    
    return 0; // Default to C
}

KeyManager::ScaleDegree KeyManager::getDegreeFromNote(int note) const
{
    auto scaleNotes = getScaleNotes();
    auto it = std::find(scaleNotes.begin(), scaleNotes.end(), note % 12);
    
    if (it != scaleNotes.end())
    {
        int index = std::distance(scaleNotes.begin(), it);
        return static_cast<ScaleDegree>(index + 1);
    }
    
    return ScaleDegree::I; // Default
}

std::vector<std::string> KeyManager::getAvailableProgressions() const
{
    std::vector<std::string> progressionNames;
    for (const auto& pair : commonProgressions)
    {
        progressionNames.push_back(pair.first);
    }
    return progressionNames;
}

void KeyManager::initializeProgressions()
{
    commonProgressions["I-V-vi-IV"] = {ScaleDegree::I, ScaleDegree::V, ScaleDegree::VI, ScaleDegree::IV};
    commonProgressions["vi-IV-I-V"] = {ScaleDegree::VI, ScaleDegree::IV, ScaleDegree::I, ScaleDegree::V};
    commonProgressions["I-vi-IV-V"] = {ScaleDegree::I, ScaleDegree::VI, ScaleDegree::IV, ScaleDegree::V};
    commonProgressions["ii-V-I"] = {ScaleDegree::II, ScaleDegree::V, ScaleDegree::I};
    commonProgressions["I-IV-V-I"] = {ScaleDegree::I, ScaleDegree::IV, ScaleDegree::V, ScaleDegree::I};
    commonProgressions["vi-ii-V-I"] = {ScaleDegree::VI, ScaleDegree::II, ScaleDegree::V, ScaleDegree::I};
    commonProgressions["I-iii-vi-IV"] = {ScaleDegree::I, ScaleDegree::III, ScaleDegree::VI, ScaleDegree::IV};
    commonProgressions["IV-V-iii-vi"] = {ScaleDegree::IV, ScaleDegree::V, ScaleDegree::III, ScaleDegree::VI};
}

std::vector<int> KeyManager::getChordIntervals(ChordType type) const
{
    switch (type)
    {
        case ChordType::Major: return {0, 4, 7};
        case ChordType::Minor: return {0, 3, 7};
        case ChordType::Diminished: return {0, 3, 6};
        case ChordType::Augmented: return {0, 4, 8};
        case ChordType::Major7: return {0, 4, 7, 11};
        case ChordType::Minor7: return {0, 3, 7, 10};
        case ChordType::Dominant7: return {0, 4, 7, 10};
        case ChordType::Diminished7: return {0, 3, 6, 9};
        case ChordType::HalfDiminished7: return {0, 3, 6, 10};
        case ChordType::Sus2: return {0, 2, 7};
        case ChordType::Sus4: return {0, 5, 7};
        case ChordType::Add9: return {0, 4, 7, 14}; // 14 = 2 + 12 (octave)
        case ChordType::Major9: return {0, 4, 7, 11, 14};
        case ChordType::Minor9: return {0, 3, 7, 10, 14};
        case ChordType::Dominant9: return {0, 4, 7, 10, 14};
        default: return {0, 4, 7};
    }
}

int KeyManager::transposeNote(int note, int semitones) const
{
    return (note + semitones) % 12;
}
