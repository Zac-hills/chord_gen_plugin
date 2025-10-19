#include "KeyManager.h"

//==============================================================================
// KeyManager Implementation

KeyManager::KeyManager() : currentKey(Key::C)
{
    noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    majorScalePattern = {0, 2, 4, 5, 7, 9, 11}; // W-W-H-W-W-W-H pattern
    initializeProgressions();
}

void KeyManager::setCurrentKey(Key key)
{
    currentKey = key;
}

KeyManager::Key KeyManager::getCurrentKey() const
{
    return currentKey;
}

std::string KeyManager::getKeyName(Key key) const
{
    return noteNames[static_cast<int>(key)];
}

std::vector<int> KeyManager::getScaleNotes() const
{
    std::vector<int> scaleNotes;
    int rootNote = static_cast<int>(currentKey);
    
    for (int interval : majorScalePattern)
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
        // Root, Third, Fifth
        chord.push_back(scaleNotes[degreeIndex]);
        chord.push_back(scaleNotes[(degreeIndex + 2) % 7]);
        chord.push_back(scaleNotes[(degreeIndex + 4) % 7]);
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
        // Add seventh
        chord.push_back(scaleNotes[(degreeIndex + 6) % 7]);
    }
    
    return chord;
}

std::vector<int> KeyManager::generateChord(ScaleDegree degree, ChordType type) const
{
    std::vector<int> chord;
    int rootNote = getNoteFromDegree(degree);
    auto intervals = getChordIntervals(type);
    
    for (int interval : intervals)
    {
        chord.push_back((rootNote + interval) % 12);
    }
    
    return chord;
}

std::vector<std::vector<int>> KeyManager::generateProgression(const std::vector<ScaleDegree>& degrees) const
{
    std::vector<std::vector<int>> progression;
    
    for (ScaleDegree degree : degrees)
    {
        progression.push_back(generateTriad(degree));
    }
    
    return progression;
}

std::vector<std::vector<int>> KeyManager::getCommonProgression(const std::string& progressionName) const
{
    auto it = commonProgressions.find(progressionName);
    if (it != commonProgressions.end())
    {
        return generateProgression(it->second);
    }
    
    return {};
}

KeyManager::ChordType KeyManager::analyzeTriad(ScaleDegree degree) const
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

KeyManager::ChordType KeyManager::analyzeSeventh(ScaleDegree degree) const
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

std::string KeyManager::getChordName(ScaleDegree degree, ChordType type) const
{
    std::string rootName = noteNames[getNoteFromDegree(degree)];
    
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
