// ThemeManager.cpp
#include "ThemeManager.h"

ThemeManager::ThemeManager()
{
    setTheme(Theme::Dark); // Default theme
}

void ThemeManager::setTheme(Theme theme)
{
    currentTheme = theme;
    
    switch (theme)
    {
        case Theme::Dark:
            currentColors = getDarkTheme();
            break;
        case Theme::Light:
            currentColors = getLightTheme();
            break;
        case Theme::Blue:
            currentColors = getBlueTheme();
            break;
        case Theme::Purple:
            currentColors = getPurpleTheme();
            break;
        case Theme::Default:
            currentColors = getDefaultTheme();
            break;
        default:
            currentColors = getDarkTheme();
    }
}
ThemeManager::ColorScheme ThemeManager::getDefaultTheme() {
    ColorScheme def;

    // Background colors
    def.backgroundMain = juce::Colour(0xfffff0d5);
    def.backgroundSecondary = juce::Colour(0xff142d52);
    def.backgroundControl = juce::Colour(0xfffff0d5);

    // Text colors
    def.textPrimary = juce::Colour(0xff142d52);
    def.textSecondary = juce::Colour(0xfffff0d5);
    def.textHighlight = juce::Colour(0xff0066cc);

    // Button colors
    def.buttonBackground = juce::Colour(0xff142d52);
    def.buttonText = juce::Colour(0xfffff0d5);
    def.buttonHighlight = juce::Colour(0xff3385dd);

    // Dropdown colors
    def.comboBoxBackground = juce::Colour(0xff142d52);
    def.comboBoxText = juce::Colour(0xfffff0d5);
    def.comboBoxOutline = juce::Colour(0xff142d52);
    
    // Label colors
    def.labelText = juce::Colour(0xff142d52);
    
    // Accent colors
    def.accentPrimary = juce::Colour(0xff569cd6);         // Blue
    def.accentSecondary = juce::Colour(0xff4ec9b0);       // Teal
    
    // Border colors
    def.border = juce::Colour(0xff142d52);
    def.outline = juce::Colour(0xff142d52);
    return def;
}
ThemeManager::ColorScheme ThemeManager::getDarkTheme()
{
    ColorScheme dark;
    
    // Background colors
    dark.backgroundMain = juce::Colour(0xff1e1e1e);        // Dark grey
    dark.backgroundSecondary = juce::Colour(0xff2d2d30);   // Slightly lighter
    dark.backgroundControl = juce::Colour(0xff3e3e42);     // Control background
    
    // Text colors
    dark.textPrimary = juce::Colour(0xffffffff);           // White
    dark.textSecondary = juce::Colour(0xffcccccc);         // Light grey
    dark.textHighlight = juce::Colour(0xff569cd6);         // Blue highlight
    
    // Button colors
    dark.buttonBackground = juce::Colour(0xff007acc);      // Blue
    dark.buttonText = juce::Colour(0xffffffff);            // White
    dark.buttonHighlight = juce::Colour(0xff1c97ea);       // Lighter blue
    
    // Dropdown colors
    dark.comboBoxBackground = juce::Colour(0xff3e3e42);
    dark.comboBoxText = juce::Colour(0xffffffff);
    dark.comboBoxOutline = juce::Colour(0xff555555);
    
    // Label colors
    dark.labelText = juce::Colour(0xffcccccc);
    
    // Accent colors
    dark.accentPrimary = juce::Colour(0xff569cd6);         // Blue
    dark.accentSecondary = juce::Colour(0xff4ec9b0);       // Teal
    
    // Border colors
    dark.border = juce::Colour(0xff555555);
    dark.outline = juce::Colour(0xff007acc);
    
    return dark;
}

ThemeManager::ColorScheme ThemeManager::getLightTheme()
{
    ColorScheme light;
    
    // Background colors
    light.backgroundMain = juce::Colour(0xfff5f5f5);
    light.backgroundSecondary = juce::Colour(0xffffffff);
    light.backgroundControl = juce::Colour(0xffe5e5e5);
    
    // Text colors
    light.textPrimary = juce::Colour(0xff000000);
    light.textSecondary = juce::Colour(0xff555555);
    light.textHighlight = juce::Colour(0xff0066cc);
    
    // Button colors
    light.buttonBackground = juce::Colour(0xff0066cc);
    light.buttonText = juce::Colour(0xffffffff);
    light.buttonHighlight = juce::Colour(0xff3385dd);
    
    // Dropdown colors
    light.comboBoxBackground = juce::Colour(0xffffffff);
    light.comboBoxText = juce::Colour(0xff000000);
    light.comboBoxOutline = juce::Colour(0xffaaaaaa);
    
    // Label colors
    light.labelText = juce::Colour(0xff333333);
    
    // Accent colors
    light.accentPrimary = juce::Colour(0xff0066cc);
    light.accentSecondary = juce::Colour(0xff00aa88);
    
    // Border colors
    light.border = juce::Colour(0xffcccccc);
    light.outline = juce::Colour(0xff0066cc);
    
    return light;
}

ThemeManager::ColorScheme ThemeManager::getBlueTheme()
{
    ColorScheme blue;
    
    // Ocean blue theme
    blue.backgroundMain = juce::Colour(0xff0a2540);
    blue.backgroundSecondary = juce::Colour(0xff143d59);
    blue.backgroundControl = juce::Colour(0xff1e5370);
    
    blue.textPrimary = juce::Colour(0xffffffff);
    blue.textSecondary = juce::Colour(0xffb8d4e8);
    blue.textHighlight = juce::Colour(0xff4fc3f7);
    
    blue.buttonBackground = juce::Colour(0xff1976d2);
    blue.buttonText = juce::Colour(0xffffffff);
    blue.buttonHighlight = juce::Colour(0xff42a5f5);
    
    blue.comboBoxBackground = juce::Colour(0xff1e5370);
    blue.comboBoxText = juce::Colour(0xffffffff);
    blue.comboBoxOutline = juce::Colour(0xff2980b9);
    
    blue.labelText = juce::Colour(0xffb8d4e8);
    
    blue.accentPrimary = juce::Colour(0xff4fc3f7);
    blue.accentSecondary = juce::Colour(0xff00bcd4);
    
    blue.border = juce::Colour(0xff2980b9);
    blue.outline = juce::Colour(0xff1976d2);
    
    return blue;
}

ThemeManager::ColorScheme ThemeManager::getPurpleTheme()
{
    ColorScheme purple;
    
    // Purple/Lavender theme
    purple.backgroundMain = juce::Colour(0xff2d1b3d);
    purple.backgroundSecondary = juce::Colour(0xff3e2753);
    purple.backgroundControl = juce::Colour(0xff4e3369);
    
    purple.textPrimary = juce::Colour(0xffffffff);
    purple.textSecondary = juce::Colour(0xffd4c5e8);
    purple.textHighlight = juce::Colour(0xffba68c8);
    
    purple.buttonBackground = juce::Colour(0xff7b1fa2);
    purple.buttonText = juce::Colour(0xffffffff);
    purple.buttonHighlight = juce::Colour(0xff9c27b0);
    
    purple.comboBoxBackground = juce::Colour(0xff4e3369);
    purple.comboBoxText = juce::Colour(0xffffffff);
    purple.comboBoxOutline = juce::Colour(0xff8e44ad);
    
    purple.labelText = juce::Colour(0xffd4c5e8);
    
    purple.accentPrimary = juce::Colour(0xffba68c8);
    purple.accentSecondary = juce::Colour(0xffab47bc);
    
    purple.border = juce::Colour(0xff8e44ad);
    purple.outline = juce::Colour(0xff7b1fa2);
    
    return purple;
}