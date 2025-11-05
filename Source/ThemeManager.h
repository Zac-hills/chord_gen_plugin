// ThemeManager.h
#pragma once
#include <JuceHeader.h>

class ThemeManager
{
public:
    enum class Theme
    {
        Dark,
        Light,
        Blue,
        Purple,
        Custom,
        Default,
    };
    
    struct ColorScheme
    {
        // Background colors
        juce::Colour backgroundMain;
        juce::Colour backgroundSecondary;
        juce::Colour backgroundControl;
        
        // Text colors
        juce::Colour textPrimary;
        juce::Colour textSecondary;
        juce::Colour textHighlight;
        
        // UI element colors
        juce::Colour buttonBackground;
        juce::Colour buttonText;
        juce::Colour buttonHighlight;
        
        // Dropdown/ComboBox colors
        juce::Colour comboBoxBackground;
        juce::Colour comboBoxText;
        juce::Colour comboBoxOutline;
        
        // Label colors
        juce::Colour labelText;
        
        // Accent colors
        juce::Colour accentPrimary;
        juce::Colour accentSecondary;
        
        // Border/outline colors
        juce::Colour border;
        juce::Colour outline;
    };
    
    ThemeManager();
    
    void setTheme(Theme theme);
    Theme getCurrentTheme() const { return currentTheme; }
    const ColorScheme& getColors() const { return currentColors; }
    
    // Preset themes
    static ColorScheme getDarkTheme();
    static ColorScheme getLightTheme();
    static ColorScheme getBlueTheme();
    static ColorScheme getPurpleTheme();
    static ColorScheme getDefaultTheme();
    
private:
    Theme currentTheme;
    ColorScheme currentColors;
};