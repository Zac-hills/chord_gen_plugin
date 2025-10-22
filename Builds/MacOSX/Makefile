# JUCE macOS Makefile for Command Line Tools

CXX = clang++

# Since JUCE includes Cocoa headers, compile everything as Objective-C++
CXXFLAGS = -x objective-c++ -std=c++17 -stdlib=libc++

DEFINES = -DMAC=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
          -DJUCE_STRICT_REFCOUNTEDPOINTER=1 -DJUCE_PROJUCER_VERSION=0x8000a \
          -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_audio_devices=1 \
          -DJUCE_MODULE_AVAILABLE_juce_audio_formats=1 -DJUCE_MODULE_AVAILABLE_juce_audio_processors=1 \
          -DJUCE_MODULE_AVAILABLE_juce_audio_utils=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 \
          -DJUCE_MODULE_AVAILABLE_juce_data_structures=1 -DJUCE_MODULE_AVAILABLE_juce_events=1 \
          -DJUCE_MODULE_AVAILABLE_juce_graphics=1 -DJUCE_MODULE_AVAILABLE_juce_gui_basics=1 \
          -DJUCE_MODULE_AVAILABLE_juce_gui_extra=1 -DJUCE_APP_VERSION=1.0.0 -DJUCE_APP_VERSION_HEX=0x10000

INCLUDES = -I../../JuceLibraryCode -I../../JuceLibraryCode/modules -I../../Source

FRAMEWORKS = -framework Cocoa -framework CoreAudio -framework CoreMIDI \
             -framework AudioUnit -framework AudioToolbox -framework Accelerate

# All source files
SOURCES = $(wildcard ../../Source/*.cpp) $(wildcard ../../JuceLibraryCode/include_*.cpp) $(wildcard ../../JuceLibraryCode/include_*.mm)

DEBUG_FLAGS = -DDEBUG=1 -D_DEBUG=1 -g
RELEASE_FLAGS = -DNDEBUG=1 -O2

BUILDDIR_DEBUG = build/Debug
BUILDDIR_RELEASE = build/Release
TARGET = NewProject

.PHONY: all debug release clean

all: debug

debug: $(BUILDDIR_DEBUG)/$(TARGET)

release: $(BUILDDIR_RELEASE)/$(TARGET)

$(BUILDDIR_DEBUG):
	mkdir -p $(BUILDDIR_DEBUG)

$(BUILDDIR_RELEASE):
	mkdir -p $(BUILDDIR_RELEASE)

$(BUILDDIR_DEBUG)/$(TARGET): $(BUILDDIR_DEBUG)
	@echo "Compiling all sources as Objective-C++..."
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(DEFINES) $(INCLUDES) $(SOURCES) $(FRAMEWORKS) -o $@
	@echo "Build complete: $@"

$(BUILDDIR_RELEASE)/$(TARGET): $(BUILDDIR_RELEASE)
	@echo "Compiling all sources as Objective-C++..."
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) $(DEFINES) $(INCLUDES) $(SOURCES) $(FRAMEWORKS) -o $@
	@echo "Build complete: $@"

clean:
	rm -rf build/
