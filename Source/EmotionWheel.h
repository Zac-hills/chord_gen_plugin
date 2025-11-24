#pragma once

#include <map>
#include <vector>
#include <string>

class EmotionWheel
{
public:
    enum class Emotion
    {
        // Happy/Major emotions
        Happy_Maj6,
        Happy_Maj69,
        Happy_Maj9,
        Happy_LydianMaj9,
        
        // Happy/Minor emotions
        Happy_Min6,
        Happy_Min69,
        Happy_Dorian9,
        Happy_MinAdd9,
        
        // Sad/Major emotions
        Sad_Maj7b6,
        Sad_Maj7add9,
        Sad_Maj9sus4,
        Sad_MajAdd4,
        
        // Sad/Minor emotions
        Sad_Min7,
        Sad_MinAdd11,
        Sad_Min7Add9,
        Sad_MinMaj7,
        
        // Warm/Major emotions
        Warm_Maj7,
        Warm_Maj9,
        Warm_69,
        Warm_Maj13,
        
        // Warm/Minor emotions
        Warm_Min9,
        Warm_Min11,
        Warm_Min69,
        Warm_Min13no11,
        
        // Tense/Major emotions
        Tense_7sus4,
        Tense_7b9,
        Tense_7sharp9,
        Tense_7alt,
        
        // Tense/Minor emotions
        Tense_MinAddFlat2,
        Tense_Min7b5,
        Tense_Min7b9,
        Tense_Min7b9b13,
        
        // Calm/Major emotions
        Calm_Sus2,
        Calm_69no3,
        Calm_Sus4add9,
        Calm_Quartal,
        
        // Calm/Minor emotions
        Calm_MinAdd9,
        Calm_MinAdd2no3,
        Calm_Min7no5,
        Calm_MinQuartal,
        
        // Dark/Major emotions
        Dark_MajorMinMaj7,
        Dark_Maj7b13,
        Dark_Maj7b9,
        Dark_7b9b13,
        
        // Dark/Minor emotions
        Dark_MinorMaj7,
        Dark_MinorMaj7Add9,
        Dark_Min7b13,
        Dark_Min7b9b13
    };
    
    enum class Tonality
    {
        Major,
        Minor
    };
    
    struct EmotionProfile
    {
        std::string name;
        Tonality tonality;
        std::vector<int> intervals; // Semitone intervals from root note
        std::string description;
    };
    
    EmotionWheel()
    {
        initializeEmotions();
    }
    
    // Get the intervals for a specific emotion
    std::vector<int> getIntervalsForEmotion(Emotion emotion) const
    {
        auto it = emotionProfiles.find(emotion);
        if (it != emotionProfiles.end())
            return it->second.intervals;
        
        return {0, 4, 7}; // Default to major triad
    }
    
    // Get the emotion profile
    const EmotionProfile* getEmotionProfile(Emotion emotion) const
    {
        auto it = emotionProfiles.find(emotion);
        if (it != emotionProfiles.end())
            return &it->second;
        
        return nullptr;
    }
    
    // Apply emotion to a root note to get chord notes
    std::vector<int> applyEmotion(int rootNote, Emotion emotion) const
    {
        std::vector<int> intervals = getIntervalsForEmotion(emotion);
        std::vector<int> notes;
        
        for (int interval : intervals)
        {
            notes.push_back(rootNote + interval);
        }
        
        return notes;
    }
    
    // Get all available emotions
    std::vector<Emotion> getAllEmotions() const
    {
        std::vector<Emotion> emotions;
        for (const auto& pair : emotionProfiles)
        {
            emotions.push_back(pair.first);
        }
        return emotions;
    }
    
    // Get emotions filtered by tonality (Major or Minor)
    std::vector<Emotion> getEmotionsByTonality(Tonality tonality) const
    {
        std::vector<Emotion> emotions;
        for (const auto& pair : emotionProfiles)
        {
            if (pair.second.tonality == tonality)
            {
                emotions.push_back(pair.first);
            }
        }
        return emotions;
    }
    
    // Get tonality of a specific emotion
    Tonality getEmotionTonality(Emotion emotion) const
    {
        auto it = emotionProfiles.find(emotion);
        if (it != emotionProfiles.end())
        {
            return it->second.tonality;
        }
        return Tonality::Major;
    }
    
    // Get emotion name as string
    static std::string getEmotionName(Emotion emotion)
    {
        switch (emotion)
        {
            case Emotion::Happy_Maj6: return "Happy (Maj6)";
            case Emotion::Happy_Maj69: return "Happy (Maj6/9)";
            case Emotion::Happy_Maj9: return "Happy (Maj9)";
            case Emotion::Happy_LydianMaj9: return "Happy (Lydian Maj9)";
            case Emotion::Happy_Min6: return "Happy (Min6)";
            case Emotion::Happy_Min69: return "Happy (Min6/9)";
            case Emotion::Happy_Dorian9: return "Happy (Dorian9)";
            case Emotion::Happy_MinAdd9: return "Happy (Min(add9))";
            case Emotion::Sad_Maj7b6: return "Sad (Maj7b6)";
            case Emotion::Sad_Maj7add9: return "Sad (Maj7add9)";
            case Emotion::Sad_Maj9sus4: return "Sad (Maj9sus4)";
            case Emotion::Sad_MajAdd4: return "Sad (Maj(add4))";
            case Emotion::Sad_Min7: return "Sad (Min7)";
            case Emotion::Sad_MinAdd11: return "Sad (Min(add11))";
            case Emotion::Sad_Min7Add9: return "Sad (Min7(add9))";
            case Emotion::Sad_MinMaj7: return "Sad (Min(maj7))";
            case Emotion::Warm_Maj7: return "Warm (Maj7)";
            case Emotion::Warm_Maj9: return "Warm (Maj9)";
            case Emotion::Warm_69: return "Warm (6/9)";
            case Emotion::Warm_Maj13: return "Warm (Maj13)";
            case Emotion::Warm_Min9: return "Warm (Min9)";
            case Emotion::Warm_Min11: return "Warm (Min11)";
            case Emotion::Warm_Min69: return "Warm (Min6/9)";
            case Emotion::Warm_Min13no11: return "Warm (Min13(no11))";
            case Emotion::Tense_7sus4: return "Tense (7sus4)";
            case Emotion::Tense_7b9: return "Tense (7♭9)";
            case Emotion::Tense_7sharp9: return "Tense (7#9)";
            case Emotion::Tense_7alt: return "Tense (7alt)";
            case Emotion::Tense_MinAddFlat2: return "Tense (Min(add♭2))";
            case Emotion::Tense_Min7b5: return "Tense (Min7♭5)";
            case Emotion::Tense_Min7b9: return "Tense (Min7♭9)";
            case Emotion::Tense_Min7b9b13: return "Tense (Min7♭9♭13)";
            case Emotion::Calm_Sus2: return "Calm (Sus2)";
            case Emotion::Calm_69no3: return "Calm (6/9(no3))";
            case Emotion::Calm_Sus4add9: return "Calm (Sus4add9)";
            case Emotion::Calm_Quartal: return "Calm (Quartal)";
            case Emotion::Calm_MinAdd9: return "Calm (Min(add9))";
            case Emotion::Calm_MinAdd2no3: return "Calm (Min(add2 no3))";
            case Emotion::Calm_Min7no5: return "Calm (Min7(no5))";
            case Emotion::Calm_MinQuartal: return "Calm (Quartal)";
            case Emotion::Dark_MajorMinMaj7: return "Dark (mMaj7)";
            case Emotion::Dark_Maj7b13: return "Dark (Maj7♭13)";
            case Emotion::Dark_Maj7b9: return "Dark (Maj7♭9)";
            case Emotion::Dark_7b9b13: return "Dark (7♭9♭13)";
            case Emotion::Dark_MinorMaj7: return "Dark (Min(maj7))";
            case Emotion::Dark_MinorMaj7Add9: return "Dark (Min(maj7)(add9))";
            case Emotion::Dark_Min7b13: return "Dark (Min7♭13)";
            case Emotion::Dark_Min7b9b13: return "Dark (Min7♭9♭13))";
            default: return "Unknown";
        }
    }
    
private:
    std::map<Emotion, EmotionProfile> emotionProfiles;
    
    void initializeEmotions()
    {
        // Happy/Major chord emotions
        // Intervals in semitones: R=0, m3=3, M3=4, 4=5, 5=7, 6=9, b7=10, M7=11, b9=13, 9=14, #11=18, b13=20, 13=21
        
        emotionProfiles[Emotion::Happy_Maj6] = {
            "Happy (Maj6)",
            Tonality::Major,
            {0, 4, 7, 9},  // R, M3, 5, 6
            "Bright and joyful"
        };
        
        emotionProfiles[Emotion::Happy_Maj69] = {
            "Happy (Maj6/9)",
            Tonality::Major,
            {0, 4, 7, 9, 14},  // R, M3, 5, 6, 9
            "Sophisticated happiness"
        };
        
        emotionProfiles[Emotion::Happy_Maj9] = {
            "Happy (Maj9)",
            Tonality::Major,
            {0, 4, 7, 11, 14},  // R, M3, 5, M7, 9
            "Elevated joy"
        };
        
        emotionProfiles[Emotion::Happy_LydianMaj9] = {
            "Happy (Lydian Maj9)",
            Tonality::Major,
            {0, 4, 7, 11, 14, 18},  // R, M3, 5, M7, 9, #11
            "Ethereal bliss"
        };
        
        // Happy/Minor chord emotions
        emotionProfiles[Emotion::Happy_Min6] = {
            "Happy (Min6)",
            Tonality::Minor,
            {0, 3, 7, 9},  // R, m3, 5, 6
            "Bittersweet joy"
        };
        
        emotionProfiles[Emotion::Happy_Min69] = {
            "Happy (Min6/9)",
            Tonality::Minor,
            {0, 3, 7, 9, 14},  // R, m3, 5, 6, 9
            "Complex happiness"
        };
        
        emotionProfiles[Emotion::Happy_Dorian9] = {
            "Happy (Dorian9)",
            Tonality::Minor,
            {0, 3, 7, 10, 14, 21},  // R, m3, 5, b7, 9, 13
            "Modal brightness"
        };
        
        emotionProfiles[Emotion::Happy_MinAdd9] = {
            "Happy (Min(add9))",
            Tonality::Minor,
            {0, 3, 7, 14},  // R, m3, 5, 9
            "Introspective joy"
        };
        
        // Sad/Major chord emotions
        emotionProfiles[Emotion::Sad_Maj7b6] = {
            "Sad (Maj7b6)",
            Tonality::Major,
            {0, 4, 7, 11, 20},  // R, M3, 5, M7, b13
            "Wistful longing"
        };
        
        emotionProfiles[Emotion::Sad_Maj7add9] = {
            "Sad (Maj7add9)",
            Tonality::Major,
            {0, 4, 7, 11, 14},  // R, M3, 5, M7, 9
            "Reflective melancholy"
        };
        
        emotionProfiles[Emotion::Sad_Maj9sus4] = {
            "Sad (Maj9sus4)",
            Tonality::Major,
            {0, 5, 7, 11, 14},  // R, 4, 5, M7, 9
            "Suspended sorrow"
        };
        
        emotionProfiles[Emotion::Sad_MajAdd4] = {
            "Sad (Maj(add4))",
            Tonality::Major,
            {0, 4, 5, 7},  // R, M3, 4, 5
            "Suspended longing"
        };
        
        // Sad/Minor chord emotions
        emotionProfiles[Emotion::Sad_Min7] = {
            "Sad (Min7)",
            Tonality::Minor,
            {0, 3, 7, 10},  // R, m3, 5, b7
            "Classic sadness and melancholy"
        };
        
        emotionProfiles[Emotion::Sad_MinAdd11] = {
            "Sad (Min(add11))",
            Tonality::Minor,
            {0, 3, 7, 17},  // R, m3, 5, 11
            "Deep introspective sorrow"
        };
        
        emotionProfiles[Emotion::Sad_Min7Add9] = {
            "Sad (Min7(add9))",
            Tonality::Minor,
            {0, 3, 7, 10, 14},  // R, m3, 5, b7, 9
            "Contemplative and heavy"
        };
        
        emotionProfiles[Emotion::Sad_MinMaj7] = {
            "Sad (Min(maj7))",
            Tonality::Minor,
            {0, 3, 7, 11},  // R, m3, 5, M7
            "Gentle sadness with hope"
        };
        
        // Warm/Major chord emotions
        emotionProfiles[Emotion::Warm_Maj7] = {
            "Warm (Maj7)",
            Tonality::Major,
            {0, 4, 7, 11},  // R, M3, 5, M7
            "Comfortable and inviting"
        };
        
        emotionProfiles[Emotion::Warm_Maj9] = {
            "Warm (Maj9)",
            Tonality::Major,
            {0, 4, 7, 11, 14},  // R, M3, 5, M7, 9
            "Rich and enveloping"
        };
        
        emotionProfiles[Emotion::Warm_69] = {
            "Warm (6/9)",
            Tonality::Major,
            {0, 4, 9, 14},  // R, M3, 6, 9
            "Cozy and nostalgic"
        };
        
        emotionProfiles[Emotion::Warm_Maj13] = {
            "Warm (Maj13)",
            Tonality::Major,
            {0, 4, 7, 11, 14, 21},  // R, M3, 5, M7, 9, 13
            "Luxurious warmth"
        };
        
        // Warm/Minor chord emotions
        emotionProfiles[Emotion::Warm_Min9] = {
            "Warm (Min9)",
            Tonality::Minor,
            {0, 3, 7, 10, 14},  // R, m3, 5, b7, 9
            "Soothing depth"
        };
        
        emotionProfiles[Emotion::Warm_Min69] = {
            "Warm (Min6/9)",
            Tonality::Minor,
            {0, 3, 7, 9, 14},  // R, m3, 5, 6, 9
            "Tender and intimate"
        };
        
        emotionProfiles[Emotion::Warm_Min11] = {
            "Warm (Min11)",
            Tonality::Minor,
            {0, 3, 7, 10, 17},  // R, m3, 5, b7, 11
            "Embracing warmth"
        };
        
        emotionProfiles[Emotion::Warm_Min13no11] = {
            "Warm (Min13(no11))",
            Tonality::Minor,
            {0, 3, 7, 10, 14, 21},  // R, m3, 5, b7, 9, 13
            "Rich warmth"
        };
        
        // Tense/Major chord emotions
        emotionProfiles[Emotion::Tense_7sus4] = {
            "Tense (7sus4)",
            Tonality::Major,
            {0, 5, 7, 10},  // R, 4, 5, b7
            "Unresolved tension"
        };
        
        emotionProfiles[Emotion::Tense_7b9] = {
            "Tense (7♭9)",
            Tonality::Major,
            {0, 4, 7, 10, 13},  // R, M3, 5, b7, b9
            "Anxious dissonance"
        };
        
        emotionProfiles[Emotion::Tense_7sharp9] = {
            "Tense (7#9)",
            Tonality::Major,
            {0, 4, 7, 10, 15},  // R, M3, 5, b7, #9
            "Edgy and restless"
        };
        
        emotionProfiles[Emotion::Tense_7alt] = {
            "Tense (7alt)",
            Tonality::Major,
            {0, 4, 7, 10, 13, 15, 8},  // R, M3, 5, b7, b9, #9, #5
            "Maximum tension"
        };
        
        // Tense/Minor chord emotions
        emotionProfiles[Emotion::Tense_MinAddFlat2] = {
            "Tense (Min(add♭2))",
            Tonality::Minor,
            {0, 1, 3, 7},  // R, b2, m3, 5
            "Jarring dissonance"
        };
        
        emotionProfiles[Emotion::Tense_Min7b5] = {
            "Tense (Min7♭5)",
            Tonality::Minor,
            {0, 3, 6, 10},  // R, m3, b5, b7
            "Half-diminished unease"
        };
        
        emotionProfiles[Emotion::Tense_Min7b9] = {
            "Tense (Min7♭9)",
            Tonality::Minor,
            {0, 3, 7, 10, 13},  // R, m3, 5, b7, b9
            "Dark and brooding"
        };
        
        emotionProfiles[Emotion::Tense_Min7b9b13] = {
            "Tense (Min7♭9♭13)",
            Tonality::Minor,
            {0, 3, 7, 10, 13, 20},  // R, m3, 5, b7, b9, b13
            "Ominous and unstable"
        };
        
        // Calm/Major chord emotions
        emotionProfiles[Emotion::Calm_Sus2] = {
            "Calm (Sus2)",
            Tonality::Major,
            {0, 2, 7},  // R, 2, 5
            "Open and peaceful"
        };
        
        emotionProfiles[Emotion::Calm_69no3] = {
            "Calm (6/9(no3))",
            Tonality::Major,
            {0, 7, 9, 14},  // R, 5, 6, 9
            "Airy and spacious"
        };
        
        emotionProfiles[Emotion::Calm_Sus4add9] = {
            "Calm (Sus4add9)",
            Tonality::Major,
            {0, 5, 7, 14},  // R, 4, 5, 9
            "Floating tranquility"
        };
        
        emotionProfiles[Emotion::Calm_Quartal] = {
            "Calm (Quartal)",
            Tonality::Major,
            {0, 7, 14, 17},  // R, 5, 9, 11
            "Modern serenity"
        };
        
        // Calm/Minor chord emotions
        emotionProfiles[Emotion::Calm_MinAdd9] = {
            "Calm (Min(add9))",
            Tonality::Minor,
            {0, 3, 7, 14},  // R, m3, 5, 9
            "Gentle stillness"
        };
        
        emotionProfiles[Emotion::Calm_MinAdd2no3] = {
            "Calm (Min(add2 no3))",
            Tonality::Minor,
            {0, 2, 7},  // R, 2, 5
            "Delicate calm"
        };
        
        emotionProfiles[Emotion::Calm_Min7no5] = {
            "Calm (Min7(no5))",
            Tonality::Minor,
            {0, 3, 10},  // R, m3, b7
            "Suspended peace"
        };
        
        emotionProfiles[Emotion::Calm_MinQuartal] = {
            "Calm (Quartal)",
            Tonality::Minor,
            {0, 5, 10},  // R, 4, b7
            "Expansive tranquility"
        };
        
        // Dark/Major chord emotions
        emotionProfiles[Emotion::Dark_MajorMinMaj7] = {
            "Dark (mMaj7)",
            Tonality::Major,
            {0, 4, 7, 11, 18},  // R, M3, 5, M7, #11
            "Mysterious beauty"
        };
        
        emotionProfiles[Emotion::Dark_Maj7b13] = {
            "Dark (Maj7♭13)",
            Tonality::Major,
            {0, 4, 7, 11, 20},  // R, M3, 5, M7, b13
            "Shadowy elegance"
        };
        
        emotionProfiles[Emotion::Dark_Maj7b9] = {
            "Dark (Maj7♭9)",
            Tonality::Major,
            {0, 4, 7, 11, 13},  // R, M3, 5, M7, b9
            "Haunting dissonance"
        };
        
        emotionProfiles[Emotion::Dark_7b9b13] = {
            "Dark (7♭9♭13)",
            Tonality::Major,
            {0, 4, 7, 10, 13, 20},  // R, M3, 5, b7, b9, b13
            "Sinister and complex"
        };
        
        // Dark/Minor chord emotions
        emotionProfiles[Emotion::Dark_MinorMaj7] = {
            "Dark (Min(maj7))",
            Tonality::Minor,
            {0, 3, 7, 11},  // R, m3, 5, M7
            "Somber and brooding"
        };
        
        emotionProfiles[Emotion::Dark_MinorMaj7Add9] = {
            "Dark (Min(maj7)(add9))",
            Tonality::Minor,
            {0, 3, 7, 11, 14},  // R, m3, 5, M7, 9
            "Noir atmosphere"
        };
        
        emotionProfiles[Emotion::Dark_Min7b13] = {
            "Dark (Min7♭13)",
            Tonality::Minor,
            {0, 3, 7, 10, 20},  // R, m3, 5, b7, b13
            "Foreboding tension"
        };
        
        emotionProfiles[Emotion::Dark_Min7b9b13] = {
            "Dark (Min7♭9♭13))",
            Tonality::Minor,
            {0, 3, 7, 10, 13, 20},  // R, m3, 5, b7, b9, b13
            "Eerie and unsettling"
        };
    }
};
