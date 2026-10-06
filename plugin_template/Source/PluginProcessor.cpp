#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "EmbeddedModel.h"

GenericNeuralAudioProcessor::GenericNeuralAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "Parameters", createParameterLayout()) {

    driveParam  = apvts.getRawParameterValue("drive");
    outputParam = apvts.getRawParameterValue("output");

    slotStateA = apvts.copyState();
    slotStateB = apvts.copyState();

    loadEmbeddedNeuralModel();
}

GenericNeuralAudioProcessor::~GenericNeuralAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout GenericNeuralAudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("drive", 1),
        "Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("output", 1),
        "Output",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f
    ));

    return { params.begin(), params.end() };
}

void GenericNeuralAudioProcessor::loadEmbeddedNeuralModel() {
    engine.loadModelFromJson(getEmbeddedModelJson());
}

const juce::String GenericNeuralAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool GenericNeuralAudioProcessor::acceptsMidi() const { return false; }
bool GenericNeuralAudioProcessor::producesMidi() const { return false; }
bool GenericNeuralAudioProcessor::isMidiEffect() const { return false; }
double GenericNeuralAudioProcessor::getTailLengthSeconds() const { return 0.02; }

const std::vector<GenericNeuralAudioProcessor::FactoryPreset>& GenericNeuralAudioProcessor::getFactoryPresets() {
    static const std::vector<FactoryPreset> presets = {
        { "Default",      0.50f,  0.0f },
        { "Clean Boost",  0.20f,  3.0f },
        { "Warm Crunch",  0.65f, -1.0f },
        { "High Gain",    0.85f, -3.0f },
        { "Dimed",        1.00f, -5.0f }
    };
    return presets;
}

void GenericNeuralAudioProcessor::loadPresetByIndex(int index) {
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int)presets.size()) return;

    undoManager.beginNewTransaction("Load Preset");
    const auto& p = presets[(size_t)index];

    auto setVal = [this](const char* id, float v) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(v));
    };

    setVal("drive",  p.drive);
    setVal("output", p.output);

    currentPresetIndex = index;
    currentPresetName = p.name;
}

bool GenericNeuralAudioProcessor::savePresetToFile(const juce::File& file) {
    auto state = apvts.copyState();
    state.setProperty("presetName", file.getFileNameWithoutExtension(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml == nullptr) return false;
    if (xml->writeTo(file)) {
        currentPresetName = file.getFileNameWithoutExtension();
        return true;
    }
    return false;
}

bool GenericNeuralAudioProcessor::loadPresetFromFile(const juce::File& file) {
    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType())) return false;

    undoManager.beginNewTransaction("Load User Preset");
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    currentPresetName = file.getFileNameWithoutExtension();
    return true;
}

void GenericNeuralAudioProcessor::setStateSlot(bool useB) {
    if (usingStateB == useB) return;

    if (usingStateB) slotStateB = apvts.copyState();
    else            slotStateA = apvts.copyState();

    usingStateB = useB;
    const auto& target = usingStateB ? slotStateB : slotStateA;
    if (target.isValid()) {
        undoManager.beginNewTransaction("Switch A/B State");
        for (int i = 0; i < target.getNumChildren(); ++i) {
            auto child = target.getChild(i);
            if (child.hasType("PARAM")) {
                auto id = child.getProperty("id").toString();
                float val = (float)child.getProperty("value");
                if (auto* param = apvts.getParameter(id))
                    param->setValueNotifyingHost(param->convertTo0to1(val));
            }
        }
    }
}

void GenericNeuralAudioProcessor::copyActiveStateToInactive() {
    if (usingStateB) slotStateA = apvts.copyState();
    else            slotStateB = apvts.copyState();
}

int GenericNeuralAudioProcessor::getNumPrograms() { return (int)getFactoryPresets().size(); }
int GenericNeuralAudioProcessor::getCurrentProgram() { return currentPresetIndex; }
void GenericNeuralAudioProcessor::setCurrentProgram(int index) { loadPresetByIndex(index); }
const juce::String GenericNeuralAudioProcessor::getProgramName(int index) {
    const auto& p = getFactoryPresets();
    return (index >= 0 && index < (int)p.size()) ? p[(size_t)index].name : juce::String{};
}
void GenericNeuralAudioProcessor::changeProgramName(int, const juce::String&) {}

void GenericNeuralAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // set to true for 2x oversampling on high-gain pedals
    engine.prepare(sampleRate, samplesPerBlock, false);
    setLatencySamples(engine.getLatencySamples());

    smoothedDrive.reset(sampleRate, 0.040);
    smoothedOutput.reset(sampleRate, 0.040);

    smoothedDrive.setCurrentAndTargetValue(driveParam ? driveParam->load() : 0.5f);
    smoothedOutput.setCurrentAndTargetValue(outputParam ? outputParam->load() : 0.0f);
}

void GenericNeuralAudioProcessor::releaseResources() {
    engine.reset();
}

bool GenericNeuralAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void GenericNeuralAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    auto inChans  = getTotalNumInputChannels();
    auto outChans = getTotalNumOutputChannels();

    for (auto i = inChans; i < outChans; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    const float curDrive  = driveParam ? driveParam->load() : 0.5f;
    const float curOutDb  = outputParam ? outputParam->load() : 0.0f;
    smoothedDrive.setTargetValue(curDrive);
    smoothedOutput.setTargetValue(curOutDb);

    float* channelL = buffer.getWritePointer(0);
    float* channelR = (inChans > 1) ? buffer.getWritePointer(1) : nullptr;

    bool isPlaying = false;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            isPlaying = pos->getIsPlaying();
        }
    }
    dawPlaying.store(isPlaying);

    // condition params array (drive)
    const float condParams[1] = { smoothedDrive.getNextValue() };
    const float outGain = juce::Decibels::decibelsToGain(smoothedOutput.getNextValue());

    engine.process(channelL, channelR, numSamples, condParams, outGain);

    // audio activity detection
    float peak = buffer.getMagnitude(0, numSamples);
    audioActive.store(peak > 0.0001f);
}

bool GenericNeuralAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* GenericNeuralAudioProcessor::createEditor() {
    return new GenericNeuralAudioProcessorEditor(*this);
}

void GenericNeuralAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GenericNeuralAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new GenericNeuralAudioProcessor();
}
