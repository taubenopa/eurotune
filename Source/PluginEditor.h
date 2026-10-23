#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI.h"

class EuroTuneAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit EuroTuneAudioProcessorEditor (EuroTuneAudioProcessor&);
    ~EuroTuneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void applyScaleToNotes();
    void drawGrid (juce::Graphics&);
    void drawParticles (juce::Graphics&);
    void drawFigure (juce::Graphics&);
    void drawEqualizer (juce::Graphics&);
    void drawTitle (juce::Graphics&);
    void rebuildOverlay();

    EuroTuneAudioProcessor& proc;
    ui::NeonLook look;

    ui::PitchGraph graph;
    ui::NoteBar noteBar;
    ui::ModeSelector modeSel;
    juce::ComboBox keyBox, scaleBox, rangeBox;
    juce::Slider refSlider;
    juce::ToggleButton bypassBtn { "BYPASS" };

    ui::KnobComp kSpeed, kAmount, kTol, kHuman, kDetune, kVibD, kVibR, kGate, kMix, kOut;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt, rangeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> refAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt;

    juce::Image figure, overlay;
    juce::Random rng;

    struct Particle { float x, y, vx, vy, size, phase; juce::Colour c; };
    std::vector<Particle> particles;

    float t = 0.0f, introT = 0.0f, lvl = 0.0f, gridPhase = 0.0f, glitchLeft = 0.0f, glitchPower = 0.0f;
    float dimSpeed = 1.0f, dimAmount = 1.0f, dimHuman = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EuroTuneAudioProcessorEditor)
};
