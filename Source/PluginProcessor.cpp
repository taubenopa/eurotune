#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
const juce::StringArray& EuroTuneAudioProcessor::scaleNames()
{
    static const juce::StringArray n { "Chromatic", "Major", "Minor", "Harmonic Minor", "Melodic Minor",
                                       "Pentatonic Major", "Pentatonic Minor", "Blues", "Dorian", "Phrygian",
                                       "Mixolydian", "Lydian", "Phrygian Dominant", "Custom" };
    return n;
}
const juce::StringArray& EuroTuneAudioProcessor::keyNames()
{
    static const juce::StringArray n { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return n;
}
const juce::StringArray& EuroTuneAudioProcessor::rangeNames()
{
    static const juce::StringArray n { "Auto", "Low", "Mid", "High" };
    return n;
}

bool EuroTuneAudioProcessor::noteInScale (int scale, int key, int note)
{
    static const std::vector<std::vector<int>> iv = {
        { 0,1,2,3,4,5,6,7,8,9,10,11 }, { 0,2,4,5,7,9,11 }, { 0,2,3,5,7,8,10 }, { 0,2,3,5,7,8,11 },
        { 0,2,3,5,7,9,11 }, { 0,2,4,7,9 }, { 0,3,5,7,10 }, { 0,3,5,6,7,10 }, { 0,2,3,5,7,9,10 },
        { 0,1,3,5,7,8,10 }, { 0,2,4,5,7,9,10 }, { 0,2,4,6,7,9,11 }, { 0,1,4,5,7,8,10 } };
    if (scale < 0 || scale >= (int) iv.size()) return true;
    const int rel = ((note - key) % 12 + 12) % 12;
    for (int v : iv[(size_t) scale]) if (v == rel) return true;
    return false;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout EuroTuneAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto flt = [&] (const char* id, const char* name, float lo, float hi, float step, float skew, float def, const char* unit)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                     NormalisableRange<float> (lo, hi, step, skew), def,
                     AudioParameterFloatAttributes().withLabel (unit)));
    };

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                 StringArray { "Clean", "Classic", "Robot", "Extreme" }, 2));
    flt ("speed",   "Retune Speed", 0.0f, 400.0f, 0.1f, 0.4f, 25.0f, " ms");
    flt ("amount",  "Amount",       0.0f, 100.0f, 0.1f, 1.0f, 100.0f, " %");
    flt ("tol",     "Tolerance",    0.0f, 50.0f,  0.1f, 1.0f, 0.0f,  " ct");
    flt ("human",   "Humanize",     0.0f, 100.0f, 0.1f, 1.0f, 0.0f,  " %");
    flt ("detune",  "Detune",       -50.0f, 50.0f, 0.1f, 1.0f, 0.0f, " ct");
    flt ("vibdepth","Vibrato",      0.0f, 100.0f, 0.1f, 1.0f, 0.0f,  " ct");
    flt ("vibrate", "Vib Rate",     0.5f, 9.0f,   0.01f, 1.0f, 5.5f, " Hz");
    flt ("thresh",  "Gate",         -80.0f, -20.0f, 0.1f, 1.0f, -55.0f, " dB");
    flt ("mix",     "Mix",          0.0f, 100.0f, 0.1f, 1.0f, 100.0f, " %");
    flt ("out",     "Output",       -24.0f, 12.0f, 0.1f, 1.0f, 0.0f, " dB");
    flt ("refa",    "Reference A",  415.0f, 466.0f, 0.1f, 1.0f, 440.0f, " Hz");
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "range", 1 }, "Range", rangeNames(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", keyNames(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", scaleNames(), 0));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));
    for (int i = 0; i < 12; ++i)
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "note_" + String (i), 1 },
                     "Note " + keyNames()[i], true));

    return { p.begin(), p.end() };
}

//==============================================================================
EuroTuneAudioProcessor::EuroTuneAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pMode = apvts.getRawParameterValue ("mode");   pSpeed = apvts.getRawParameterValue ("speed");
    pAmount = apvts.getRawParameterValue ("amount"); pTol = apvts.getRawParameterValue ("tol");
    pHuman = apvts.getRawParameterValue ("human");  pVibD = apvts.getRawParameterValue ("vibdepth");
    pVibR = apvts.getRawParameterValue ("vibrate"); pDetune = apvts.getRawParameterValue ("detune");
    pRef = apvts.getRawParameterValue ("refa");     pThresh = apvts.getRawParameterValue ("thresh");
    pRange = apvts.getRawParameterValue ("range");  pMix = apvts.getRawParameterValue ("mix");
    pOut = apvts.getRawParameterValue ("out");      pBypass = apvts.getRawParameterValue ("bypass");
    for (int i = 0; i < 12; ++i) pNote[i] = apvts.getRawParameterValue ("note_" + juce::String (i));
}

bool EuroTuneAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& o = l.getMainOutputChannelSet();
    if (o != juce::AudioChannelSet::mono() && o != juce::AudioChannelSet::stereo()) return false;
    return l.getMainInputChannelSet() == o;
}

void EuroTuneAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    invSr = (float) (1.0 / sampleRate);
    yin.prepare (sampleRate);
    hopSec = yin.hopSeconds();
    const int size = (int) (0.2 * sampleRate);
    dl[0].prepare (size); dl[1].prepare (size);
    latency = (int) (0.0175 * sampleRate);
    setLatencySamples (latency);

    histCount = 0; missCount = 0; prevTarget = -1; voicedNow = false; holdSec = 0; corrCents = 0; heldCents = 0;
    targetCents = 0; ratioSm = 1.0f; Wtarget = Wsm = (float) (0.03 * sampleRate); phase = 0; vibPhase = 0;
    active = activeTarget = 0.0f;
    actCoeff = 1.0f - std::exp (-1.0f / (0.008f * (float) sampleRate));
    wCoeff   = 1.0f - std::exp (-1.0f / (0.060f * (float) sampleRate));
}

void EuroTuneAudioProcessor::refreshParams()
{
    P.mode = (int) pMode->load();
    P.speedMs = pSpeed->load();
    P.amount = pAmount->load() * 0.01f;
    P.tolCents = pTol->load();
    P.humanize = pHuman->load() * 0.01f;
    P.vibDepth = pVibD->load();
    P.vibRate = pVibR->load();
    P.detune = pDetune->load();
    P.refA = pRef->load();
    P.threshDb = pThresh->load();
    P.mix = pMix->load() * 0.01f;
    P.outGain = juce::Decibels::decibelsToGain (pOut->load());
    for (int i = 0; i < 12; ++i) P.noteOn[i] = pNote[i]->load() > 0.5f;

    switch ((int) pRange->load())
    {
        case 1:  P.minHz = 60.0f;  P.maxHz = 350.0f;  break;
        case 2:  P.minHz = 90.0f;  P.maxHz = 600.0f;  break;
        case 3:  P.minHz = 170.0f; P.maxHz = 1100.0f; break;
        default: P.minHz = 60.0f;  P.maxHz = 1000.0f; break;
    }
    const float tc = (P.mode == 3) ? 0.0006f : 0.0018f;
    ratioCoeff = 1.0f - std::exp (-1.0f / (tc * (float) sr));
}

int EuroTuneAudioProcessor::pickTarget (float midi)
{
    const int base = (int) std::floor (midi);
    float best = 1.0e9f; int bestN = -1;
    for (int n = base - 2; n <= base + 3; ++n)
    {
        const int pc = ((n % 12) + 12) % 12;
        if (! P.noteOn[pc]) continue;
        const float d = std::fabs ((float) n - midi);
        if (d < best) { best = d; bestN = n; }
    }
    if (bestN < 0) return -1;
    if (prevTarget >= 0 && prevTarget != bestN)
    {
        const int ppc = ((prevTarget % 12) + 12) % 12;
        if (P.noteOn[ppc] && std::fabs ((float) prevTarget - midi) < best + 0.15f) bestN = prevTarget;   // Hysterese
    }
    return bestN;
}

void EuroTuneAudioProcessor::analyse()
{
    float f0 = 0.0f, rms = 0.0f;
    const bool ok = yin.detect (P.minHz, P.maxHz, f0, rms);
    const float rmsDb = 20.0f * std::log10 (rms + 1.0e-9f);
    const bool v = ok && rmsDb > P.threshDb;
    const float hs = (float) hopSec;

    if (! v)
    {
        if (voicedNow && ++missCount <= 2) return;   // kurze Aussetzer ueberbruecken
        voicedNow = false; histCount = 0; prevTarget = -1; holdSec = 0.0f;
        corrCents *= 0.8f; heldCents = 0.0f; targetCents = corrCents;
        activeTarget = 0.0f;
        tInMidi.store (-1.0f); tOutMidi.store (-1.0f); tTargetMidi.store (-1.0f);
        return;
    }
    missCount = 0; voicedNow = true;

    hist[2] = hist[1]; hist[1] = hist[0]; hist[0] = f0;
    if (histCount < 3) ++histCount;
    float f = f0;
    if (histCount >= 3)
        f = std::max (std::min (hist[0], hist[1]), std::min (std::max (hist[0], hist[1]), hist[2]));  // Median

    const float midi = 69.0f + 12.0f * std::log2 (f / P.refA);
    const int tgt = pickTarget (midi);

    float desired = 0.0f, targetMidi = midi;
    if (tgt >= 0)
    {
        targetMidi = (float) tgt + P.detune * 0.01f;
        if (tgt != prevTarget) holdSec = 0.0f; else holdSec += hs;
        prevTarget = tgt;

        const float errC = (targetMidi - midi) * 100.0f;
        const float tol = (P.mode == 3) ? 0.0f : P.tolCents;
        const float mag = std::max (0.0f, std::fabs (errC) - tol);
        const float amount = (P.mode == 3) ? 1.0f : P.amount;
        desired = (errC < 0 ? -mag : mag) * amount;
    }

    float sp = P.speedMs;
    switch (P.mode)
    {
        case 0:  sp = std::max (sp, 40.0f); break;
        case 1:  sp = std::max (sp, 10.0f); break;
        default: sp = 0.0f; break;
    }
    if (P.mode < 2) sp += P.humanize * std::min (holdSec, 1.5f) * 250.0f;

    const float alpha = sp <= 0.5f ? 1.0f : 1.0f - std::exp (-hs * 1000.0f / sp);
    corrCents += alpha * (desired - corrCents);

    if (P.mode == 3)   // Extreme: Treppenstufen-Update fuer den digitalen "Glitch-Robot"
    {
        if (++stairCount >= 4) { stairCount = 0; heldCents = corrCents; }
        targetCents = heldCents;
    }
    else targetCents = corrCents;

    // Pitch-synchrone Fensterlaenge: 2*n Perioden (~30 ms)
    const int n = std::max (1, (int) std::lround (0.016f * f));
    const float w = 2.0f * (float) n * (float) sr / f;
    Wtarget = juce::jlimit (0.012f * (float) sr, 0.07f * (float) sr, w);

    activeTarget = (std::fabs (targetCents) > 0.4f || P.vibDepth > 0.1f) ? 1.0f : 0.0f;

    tInMidi.store (midi);
    tOutMidi.store (midi + targetCents * 0.01f);
    tTargetMidi.store (tgt >= 0 ? targetMidi : -1.0f);
}

void EuroTuneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch) buffer.clear (ch, 0, n);
    if (nCh == 0) return;

    if (pBypass->load() > 0.5f)
    {
        tInMidi.store (-1.0f); tOutMidi.store (-1.0f); tTargetMidi.store (-1.0f);
        return;
    }

    refreshParams();
    float* chan[2] = { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };
    const int numProc = (chan[1] != nullptr) ? 2 : 1;
    double sumSq = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const float in0 = chan[0][i];
        const float in1 = chan[1] != nullptr ? chan[1][i] : in0;
        const float mono = chan[1] != nullptr ? 0.5f * (in0 + in1) : in0;
        sumSq += (double) mono * (double) mono;

        if (yin.push (mono)) analyse();

        vibPhase += P.vibRate * invSr;
        if (vibPhase >= 1.0f) vibPhase -= 1.0f;
        const float vibCents = P.vibDepth * std::sin (6.2831853f * vibPhase);

        const float ratioT = std::exp2 ((targetCents + vibCents) / 1200.0f);
        ratioSm += (ratioT - ratioSm) * ratioCoeff;
        ratioSm = juce::jlimit (0.5f, 2.0f, ratioSm);
        Wsm += (Wtarget - Wsm) * wCoeff;
        active += (activeTarget - active) * actCoeff;

        phase += (1.0f - ratioSm) / Wsm;
        if (phase >= 1.0f) phase -= 1.0f; else if (phase < 0.0f) phase += 1.0f;
        float p1 = phase + 0.5f; if (p1 >= 1.0f) p1 -= 1.0f;
        const float s0 = std::sin (3.14159265f * phase), s1 = std::sin (3.14159265f * p1);
        const float g0 = s0 * s0, g1 = s1 * s1;
        const float d0 = 2.0f + phase * Wsm, d1 = 2.0f + p1 * Wsm;
        const float amt = P.mix * active;

        for (int c = 0; c < numProc; ++c)
        {
            const float x = (c == 0) ? in0 : in1;
            dl[c].write (x);
            const float dry = dl[c].readInt (latency);
            const float wet = g0 * dl[c].readFrac (d0) + g1 * dl[c].readFrac (d1);
            chan[c][i] = (dry + amt * (wet - dry)) * P.outGain;
        }
    }

    const float lvl = (float) std::sqrt (sumSq / (double) n);
    tLevel.store (tLevel.load() * 0.6f + lvl * 0.4f);
}

//==============================================================================
juce::AudioProcessorEditor* EuroTuneAudioProcessor::createEditor() { return new EuroTuneAudioProcessorEditor (*this); }

void EuroTuneAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, dest);
}

void EuroTuneAudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EuroTuneAudioProcessor(); }
