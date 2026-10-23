#pragma once
#include <JuceHeader.h>
#include "DSP.h"

class EuroTuneAudioProcessor : public juce::AudioProcessor
{
public:
    EuroTuneAudioProcessor();
    ~EuroTuneAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EuroTune"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& scaleNames();
    static const juce::StringArray& keyNames();
    static const juce::StringArray& rangeNames();
    static bool noteInScale (int scaleIndex, int key, int note);
    static int customScaleIndex() { return scaleNames().size() - 1; }

    juce::AudioProcessorValueTreeState apvts;

    // Telemetrie fuer die GUI (-1 = kein Pitch)
    std::atomic<float> tInMidi { -1.0f }, tOutMidi { -1.0f }, tTargetMidi { -1.0f }, tLevel { 0.0f };

private:
    void refreshParams();
    void analyse();
    int pickTarget (float midi);

    struct Params
    {
        int mode = 2;
        float speedMs = 25.0f, amount = 1.0f, tolCents = 0.0f, humanize = 0.0f;
        float vibDepth = 0.0f, vibRate = 5.5f, detune = 0.0f, refA = 440.0f, threshDb = -55.0f;
        float minHz = 60.0f, maxHz = 1000.0f, mix = 1.0f, outGain = 1.0f;
        bool noteOn[12] {};
    } P;

    std::atomic<float>* pMode = nullptr; std::atomic<float>* pSpeed = nullptr; std::atomic<float>* pAmount = nullptr;
    std::atomic<float>* pTol = nullptr; std::atomic<float>* pHuman = nullptr; std::atomic<float>* pVibD = nullptr;
    std::atomic<float>* pVibR = nullptr; std::atomic<float>* pDetune = nullptr; std::atomic<float>* pRef = nullptr;
    std::atomic<float>* pThresh = nullptr; std::atomic<float>* pRange = nullptr; std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOut = nullptr; std::atomic<float>* pBypass = nullptr;
    std::atomic<float>* pNote[12] {};

    eurotune::YinDetector yin;
    eurotune::DelayLine dl[2];

    double sr = 44100.0;
    float invSr = 1.0f / 44100.0f;
    int latency = 770;
    double hopSec = 0.006;

    // Analyse-Zustand
    float hist[3] { 0, 0, 0 };
    int histCount = 0, missCount = 0, prevTarget = -1, stairCount = 0;
    bool voicedNow = false;
    float holdSec = 0.0f, corrCents = 0.0f, heldCents = 0.0f;

    // Audio-Zustand
    float targetCents = 0.0f, ratioSm = 1.0f, Wsm = 1323.0f, Wtarget = 1323.0f, phase = 0.0f, vibPhase = 0.0f;
    float active = 0.0f, activeTarget = 0.0f;
    float ratioCoeff = 0.01f, wCoeff = 0.001f, actCoeff = 0.01f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EuroTuneAudioProcessor)
};
