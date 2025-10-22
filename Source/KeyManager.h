#pragma once

#include <JuceHeader.h>
#include <vector>
#include <string>
#include <map>

class KeyManager
{
public:
    enum class Key
    {
        C = 0, C_Sharp, D, D_Sharp, E, F, F_Sharp, G, G_Sharp, A, A_Sharp, B
    };
    
    enum class ChordType
    {
        Major,
        Minor,
        Diminished,
        Augmented,
        Major7,
        Minor7,
        Dominant7,
        Diminished7,
        HalfDiminished7,
        Sus2,
        Sus4,
        Add9,
        Major9,
        Minor9,
        Dominant9
    };
    
    enum class ScaleDegree
    {
        I = 1, II, III, IV, V, VI, VII
    };
    
    KeyManager();
    ~KeyManager() = default;
    
    // Key management
    void setCurrentKey(Key key);
    Key getCurrentKey() const;
    std::string getKeyName(Key key) const;
    
    // Scale and note functions
    std::vector<int> getScaleNotes() const;
    std::vector<std::string> getScaleNoteNames() const;
    std::vector<int> getChromaticNotes() const;
    std::vector<std::string> getChromaticNoteNames() const;
    bool isNoteInKey(int note) const;
    
    // Chord generation functions
    std::vector<int> generateTriad(ScaleDegree degree) const;
    std::vector<int> generateSeventh(ScaleDegree degree) const;
    std::vector<int> generateChord(ScaleDegree degree, ChordType type) const;
    
    // Chord progression functions
    std::vector<std::vector<int>> generateProgression(const std::vector<ScaleDegree>& degrees) const;
    std::vector<std::vector<int>> generateProgression(const std::vector<ScaleDegree>& degrees, bool useSevenths) const;
    std::vector<std::vector<int>> getCommonProgression(const std::string& progressionName) const;
    std::vector<std::vector<int>> getCommonProgression(const std::string& progressionName, bool useSevenths) const;
    
    // Chord analysis
    ChordType analyzeTriad(ScaleDegree degree) const;
    ChordType analyzeSeventh(ScaleDegree degree) const;
    std::string getChordName(ScaleDegree degree, ChordType type) const;
    
    // Utility functions
    int getNoteFromDegree(ScaleDegree degree) const;
    ScaleDegree getDegreeFromNote(int note) const;
    std::vector<std::string> getAvailableProgressions() const;
    
    // Helper functions
    std::vector<int> getChordIntervals(ChordType type) const;
    int transposeNote(int note, int semitones) const;

private:
    Key currentKey;
    std::vector<std::string> noteNames;
    std::vector<int> majorScalePattern;
    std::map<std::string, std::vector<ScaleDegree>> commonProgressions;
    
    void initializeProgressions();
};