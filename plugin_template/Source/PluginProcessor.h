#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <atomic>
#include <vector>
#include "NeuralModelEngine.h"

class GenericNeuralAudioProcessor : public juce::AudioProcessor {
public:
    GenericNeuralAudioProcessor();
    ~GenericNeuralAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    // preset manager
    struct FactoryPreset {
        const char* name;
        float drive;
        float output;
    };
    static const std::vector<FactoryPreset>& getFactoryPresets();
    void loadPresetByIndex(int index);
    bool savePresetToFile(const juce::File& file);
    bool loadPresetFromFile(const juce::File& file);
    juce::String getCurrentPresetName() const { return currentPresetName; }

    // a/b state comparison
    bool isUsingStateB() const noexcept { return usingStateB; }
    void setStateSlot(bool useB);
    void copyActiveStateToInactive();

    bool isDawPlaying() const noexcept { return dawPlaying.load(); }
    bool isAudioActive() const noexcept { return audioActive.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void loadEmbeddedNeuralModel();

    // 1 conditioning parameter (drive) -> NumInputs = 2 (audio + drive)
    // change NumInputs to 3 for Drive + Tone, etc.
    NeuralModelEngine<2, 48> engine;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedOutput;

    std::atomic<bool> dawPlaying { false };
    std::atomic<bool> audioActive { false };

    int currentPresetIndex = 0;
    juce::String currentPresetName { "Default" };
    bool usingStateB = false;
    juce::ValueTree slotStateA;
    juce::ValueTree slotStateB;

    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* outputParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenericNeuralAudioProcessor)
};
