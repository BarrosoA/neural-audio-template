#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "RTNeural/RTNeural.h"
#include <vector>
#include <cmath>
#include <algorithm>

template <int NumInputs = 2, int HiddenSize = 48>
class NeuralModelEngine {
public:
    using ModelType = RTNeural::ModelT<float, NumInputs, 1,
        RTNeural::GRULayerT<float, NumInputs, HiddenSize>,
        RTNeural::DenseT<float, HiddenSize, 1>
    >;

    NeuralModelEngine() = default;

    void prepare(double sampleRate, int maxBlockSize, bool enableOversampling2x = false) {
        fs = sampleRate;
        blockSize = maxBlockSize;
        useOversampling = enableOversampling2x;

        if (useOversampling) {
            // 2x oversampling for high-gain clipping anti-aliasing
            oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
                2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false
            );
            oversampler->initProcessing(static_cast<size_t>(maxBlockSize));
            oversampler->reset();
            procFs = sampleRate * 2.0;
        } else {
            oversampler.reset();
            procFs = sampleRate;
        }

        // dc blocker at ~10 hz
        dcR = (float)(1.0 - (2.0 * 3.14159265358979323846 * 10.0 / procFs));
        reset();

        silenceEnvelope = 0.0f;
        envAttack  = (float)(1.0 - std::exp(-1.0 / (0.001 * procFs)));
        envRelease = (float)(1.0 - std::exp(-1.0 / (0.030 * procFs)));

        monoScratchR.assign(static_cast<size_t>(std::max(1, maxBlockSize * (useOversampling ? 2 : 1))), 0.0f);
        warmUp();
    }

    void reset() {
        if (modelsLoaded) {
            modelL.reset();
            modelR.reset();
        }
        dcX1L = dcY1L = dcX1R = dcY1R = 0.0f;
        silenceEnvelope = 0.0f;
    }

    bool loadModelFromJson(const std::string& jsonString) {
        try {
            auto json = nlohmann::json::parse(jsonString);
            modelL.parseJson(json);
            modelR.parseJson(json);
            modelL.reset();
            modelR.reset();
            modelsLoaded = true;
            warmUp();
            return true;
        } catch (...) {
            modelsLoaded = false;
            return false;
        }
    }

    void warmUp() {
        if (!modelsLoaded) return;
        modelL.reset();
        modelR.reset();

        float in[NumInputs] = { 0.0f };
        for (int i = 0; i < 512; ++i) {
            modelL.forward(in);
            modelR.forward(in);
        }
    }

    int getLatencySamples() const noexcept {
        return (useOversampling && oversampler) ? (int)oversampler->getLatencyInSamples() : 0;
    }

    // process block with audio and conditioned scalar parameters
    void process(
        float* left,
        float* right,
        int numSamples,
        const float* conditionParams, // array of (NumInputs - 1) parameter values
        float outputLinearGain = 1.0f
    ) {
        if (!modelsLoaded || numSamples <= 0) return;

        if (useOversampling && oversampler) {
            juce::AudioBuffer<float> tempBuffer(2, numSamples);
            tempBuffer.copyFrom(0, 0, left, numSamples);
            if (right != nullptr)
                tempBuffer.copyFrom(1, 0, right, numSamples);
            else
                tempBuffer.copyFrom(1, 0, left, numSamples);

            juce::dsp::AudioBlock<float> block(tempBuffer);
            auto oversampledBlock = oversampler->processSamplesUp(block);

            float* osL = oversampledBlock.getChannelPointer(0);
            float* osR = oversampledBlock.getChannelPointer(1);
            const int osSamples = (int)oversampledBlock.getNumSamples();

            processSamplesDirect(osL, osR, osSamples, conditionParams, outputLinearGain);
            oversampler->processSamplesDown(block);

            FloatVectorOperations::copy(left, tempBuffer.getReadPointer(0), numSamples);
            if (right != nullptr)
                FloatVectorOperations::copy(right, tempBuffer.getReadPointer(1), numSamples);
        } else {
            processSamplesDirect(left, right, numSamples, conditionParams, outputLinearGain);
        }
    }

private:
    void processSamplesDirect(
        float* left,
        float* right,
        int numSamples,
        const float* conditionParams,
        float outGain
    ) {
        float inL[NumInputs];
        float inR[NumInputs];

        // copy conditioning knob values
        for (int k = 0; k < NumInputs - 1; ++k) {
            inL[1 + k] = conditionParams[k];
            inR[1 + k] = conditionParams[k];
        }

        const bool isStereo = (right != nullptr);

        for (int i = 0; i < numSamples; ++i) {
            const float rawInL = left[i];
            const float rawInR = isStereo ? right[i] : rawInL;

            // envelope tracking for silence auto-gate
            const float inMag = std::max(std::abs(rawInL), std::abs(rawInR));
            if (inMag > silenceEnvelope)
                silenceEnvelope += envAttack * (inMag - silenceEnvelope);
            else
                silenceEnvelope += envRelease * (inMag - silenceEnvelope);

            inL[0] = rawInL;
            inR[0] = rawInR;

            modelL.forward(inL);
            if (isStereo)
                modelR.forward(inR);

            float outL = modelL.getOutputs()[0];
            float outR = isStereo ? modelR.getOutputs()[0] : outL;

            // dc blocker
            float blockedL = outL - dcX1L + dcR * dcY1L;
            dcX1L = outL;
            dcY1L = blockedL;

            float blockedR = outR - dcX1R + dcR * dcY1R;
            dcX1R = outR;
            dcY1R = blockedR;

            // smooth silence gate prevents idle noise
            float gate = (silenceEnvelope < 0.00005f) ? (silenceEnvelope / 0.00005f) : 1.0f;

            left[i] = blockedL * gate * outGain;
            if (isStereo)
                right[i] = blockedR * gate * outGain;
        }
    }

    double fs = 44100.0;
    double procFs = 44100.0;
    int blockSize = 512;
    bool useOversampling = false;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    ModelType modelL;
    ModelType modelR;
    bool modelsLoaded = false;

    float dcR = 0.9986f;
    float dcX1L = 0.0f, dcY1L = 0.0f;
    float dcX1R = 0.0f, dcY1R = 0.0f;

    float silenceEnvelope = 0.0f;
    float envAttack = 0.05f;
    float envRelease = 0.001f;

    std::vector<float> monoScratchR;
};
