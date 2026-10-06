#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class FlatHeaderLookAndFeel : public juce::LookAndFeel_V4 {
public:
    FlatHeaderLookAndFeel();

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText, const juce::Drawable* icon,
                           const juce::Colour* textColourToUse) override;

    juce::Font getPopupMenuFont() override;
};

class GenericNeuralAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::Timer {
public:
    explicit GenericNeuralAudioProcessorEditor(GenericNeuralAudioProcessor&);
    ~GenericNeuralAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    GenericNeuralAudioProcessor& processorRef;
    FlatHeaderLookAndFeel headerLookAndFeel;

    // header controls
    juce::TextButton presetPrevBtn { "prev" };
    juce::TextButton presetMenuBtn { "Default" };
    juce::TextButton presetNextBtn { "next" };
    juce::TextButton undoBtn       { "undo" };
    juce::TextButton redoBtn       { "redo" };
    juce::TextButton stateABtn     { "A" };
    juce::TextButton stateBBtn     { "B" };
    juce::TextButton copyStateBtn  { "A>B" };
    juce::TextButton zoomBtn       { "100%" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    int copyFlashFrames = 0;

    // hardware controls
    juce::Slider driveSlider;
    juce::Slider outputSlider;
    juce::Label driveLabel  { {}, "DRIVE" };
    juce::Label outputLabel { {}, "OUTPUT" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttach;

    void updateHeaderDisplay();
    void showPresetMenu();
    void showZoomMenu();
    void applyZoomFactor(float factor);

    // floating readout pill
    juce::Slider* activePillSlider = nullptr;
    juce::String pillText;
    float pillCanvasX = 0.0f;
    float pillCanvasY = 0.0f;
    float pillAlpha = 0.0f;
    int pillHoldFrames = 0;
    void updateValuePillForSlider(juce::Slider* s);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenericNeuralAudioProcessorEditor)
};
