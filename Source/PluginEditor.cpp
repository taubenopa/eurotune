#include "PluginEditor.h"
#include "BinaryData.h"

using namespace ui;

static float easeOutBack (float x)
{
    x = juce::jlimit (0.0f, 1.0f, x);
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow (x - 1.0f, 3.0f) + c1 * std::pow (x - 1.0f, 2.0f);
}

EuroTuneAudioProcessorEditor::EuroTuneAudioProcessorEditor (EuroTuneAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      graph (p.apvts, p), noteBar (p.apvts, p), modeSel (p.apvts),
      kSpeed (p.apvts, "speed", "Speed"), kAmount (p.apvts, "amount", "Amount"), kTol (p.apvts, "tol", "Tolerance"),
      kHuman (p.apvts, "human", "Humanize"), kDetune (p.apvts, "detune", "Detune"),
      kVibD (p.apvts, "vibdepth", "Vibrato"), kVibR (p.apvts, "vibrate", "Vib Rate"),
      kGate (p.apvts, "thresh", "Gate"), kMix (p.apvts, "mix", "Mix"), kOut (p.apvts, "out", "Output")
{
    setLookAndFeel (&look);
    setSize (980, 640);

    // Bild laden und einmal auf Anzeigegroesse skalieren
    {
        auto img = juce::ImageFileFormat::loadFrom (BinaryData::figure_png, (size_t) BinaryData::figure_pngSize);
        if (img.isValid())
        {
            const int dh = 560;
            const int dw = (int) ((float) img.getWidth() * (float) dh / (float) img.getHeight());
            figure = img.rescaled (dw, dh, juce::Graphics::highResamplingQuality);
        }
    }

    for (int i = 0; i < 80; ++i)
    {
        Particle q;
        q.x = rng.nextFloat() * 980.0f; q.y = rng.nextFloat() * 640.0f;
        q.vx = (rng.nextFloat() - 0.5f) * 0.3f; q.vy = 0.2f + rng.nextFloat() * 0.8f;
        q.size = 1.0f + rng.nextFloat() * 2.5f; q.phase = rng.nextFloat() * 6.28f;
        const int k = rng.nextInt (3);
        q.c = k == 0 ? kAcid : (k == 1 ? kPink : kCyan);
        particles.push_back (q);
    }

    for (auto* c : std::initializer_list<juce::Component*> { &graph, &noteBar, &modeSel, &keyBox, &scaleBox, &rangeBox,
                                                              &refSlider, &bypassBtn, &kSpeed, &kAmount, &kTol, &kHuman,
                                                              &kDetune, &kVibD, &kVibR, &kGate, &kMix, &kOut })
        addAndMakeVisible (c);

    keyBox.addItemList (EuroTuneAudioProcessor::keyNames(), 1);
    scaleBox.addItemList (EuroTuneAudioProcessor::scaleNames(), 1);
    rangeBox.addItemList (EuroTuneAudioProcessor::rangeNames(), 1);
    keyAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "key", keyBox);
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "scale", scaleBox);
    rangeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "range", rangeBox);
    keyBox.onChange   = [this] { applyScaleToNotes(); };
    scaleBox.onChange = [this] { applyScaleToNotes(); };

    refSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    refSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 80, 20);
    refAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "refa", refSlider);
    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "bypass", bypassBtn);

    glitchLeft = 0.5f; glitchPower = 1.5f;   // Intro-Glitch
    startTimerHz (60);
}

EuroTuneAudioProcessorEditor::~EuroTuneAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void EuroTuneAudioProcessorEditor::applyScaleToNotes()
{
    const int scale = scaleBox.getSelectedItemIndex();
    const int key = keyBox.getSelectedItemIndex();
    if (scale < 0 || key < 0 || scale == EuroTuneAudioProcessor::customScaleIndex()) return;
    for (int i = 0; i < 12; ++i)
        if (auto* p = proc.apvts.getParameter ("note_" + juce::String (i)))
        {
            const float v = EuroTuneAudioProcessor::noteInScale (scale, key, i) ? 1.0f : 0.0f;
            if (std::abs (p->getValue() - v) > 0.01f)
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (v);
                p->endChangeGesture();
            }
        }
}

void EuroTuneAudioProcessorEditor::resized()
{
    const int x0 = 370, w = 590;
    graph.setBounds (x0, 88, w, 150);
    noteBar.setBounds (x0, 250, w, 42);
    modeSel.setBounds (x0, 312, 285, 34);
    keyBox.setBounds (x0 + 295, 316, 58, 28);
    scaleBox.setBounds (x0 + 361, 316, 140, 28);
    rangeBox.setBounds (x0 + 509, 316, 81, 28);

    KnobComp* row1[] = { &kSpeed, &kAmount, &kTol, &kHuman, &kDetune };
    KnobComp* row2[] = { &kVibD, &kVibR, &kGate, &kMix, &kOut };
    for (int i = 0; i < 5; ++i)
    {
        row1[i]->setBounds (x0 + i * 118, 354, 118, 118);
        row2[i]->setBounds (x0 + i * 118, 478, 118, 118);
    }
    refSlider.setBounds (x0 + 62, 604, 250, 24);
    bypassBtn.setBounds (x0 + w - 110, 604, 110, 26);
    rebuildOverlay();
}

void EuroTuneAudioProcessorEditor::rebuildOverlay()
{
    overlay = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
    juce::Graphics g (overlay);
    g.setColour (juce::Colours::black.withAlpha (0.13f));
    for (int y = 0; y < getHeight(); y += 3) g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    juce::ColourGradient vg (juce::Colours::transparentBlack, (float) getWidth() * 0.5f, (float) getHeight() * 0.5f,
                             juce::Colours::black.withAlpha (0.7f), 0.0f, 0.0f, true);
    g.setGradientFill (vg);
    g.fillAll();
}

//==============================================================================
void EuroTuneAudioProcessorEditor::timerCallback()
{
    const float dt = 1.0f / 60.0f;
    t += dt;
    introT = juce::jmin (1.0f, introT + dt / 1.6f);
    const float raw = juce::jlimit (0.0f, 1.0f, proc.tLevel.load() * 4.0f);
    lvl += (raw - lvl) * 0.3f;
    gridPhase += 0.012f + lvl * 0.06f;
    look.t = t;

    if (glitchLeft > 0.0f) glitchLeft -= dt;
    else if (rng.nextFloat() < 0.005f + lvl * 0.012f) { glitchLeft = 0.12f + rng.nextFloat() * 0.25f; glitchPower = 0.5f + rng.nextFloat(); }

    for (auto& q : particles)
    {
        q.y -= q.vy * (1.0f + lvl * 5.0f);
        q.x += q.vx + std::sin (t + q.phase) * 0.2f;
        if (q.y < -5.0f) { q.y = 645.0f; q.x = rng.nextFloat() * 980.0f; }
        if (q.x < -5.0f) q.x = 985.0f;
        if (q.x > 985.0f) q.x = -5.0f;
    }

    modeSel.tick();
    noteBar.tick();
    graph.tick (t);

    const int mode = (int) proc.apvts.getRawParameterValue ("mode")->load();
    dimSpeed  += ((mode >= 2 ? 0.28f : 1.0f) - dimSpeed) * 0.15f;
    dimAmount += ((mode == 3 ? 0.28f : 1.0f) - dimAmount) * 0.15f;
    dimHuman  += ((mode >= 2 ? 0.28f : 1.0f) - dimHuman) * 0.15f;

    const float ia = juce::jlimit (0.0f, 1.0f, (introT - 0.25f) * 1.8f);
    graph.setAlpha (ia); noteBar.setAlpha (ia); modeSel.setAlpha (ia); keyBox.setAlpha (ia);
    scaleBox.setAlpha (ia); rangeBox.setAlpha (ia); refSlider.setAlpha (ia); bypassBtn.setAlpha (ia);
    kSpeed.setAlpha (ia * dimSpeed); kAmount.setAlpha (ia * dimAmount); kTol.setAlpha (ia * dimAmount);
    kHuman.setAlpha (ia * dimHuman);
    for (auto* k : { &kDetune, &kVibD, &kVibR, &kGate, &kMix, &kOut }) k->setAlpha (ia);

    repaint();
}

//==============================================================================
void EuroTuneAudioProcessorEditor::drawGrid (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    const float horizon = h * 0.58f, vx = w * 0.5f;
    g.saveState();
    g.reduceClipRegion (0, (int) horizon, getWidth(), getHeight() - (int) horizon);
    g.setColour (kPurple.withAlpha (0.30f + lvl * 0.3f));
    for (int k = -16; k <= 16; ++k) g.drawLine (vx + (float) k * 10.0f, horizon, vx + (float) k * 115.0f, h, 1.0f);
    const float fr = gridPhase - std::floor (gridPhase);
    for (int i = 0; i < 16; ++i)
    {
        const float z = std::fmod (((float) i + fr) / 16.0f, 1.0f);
        const float y = horizon + (h - horizon) * z * z;
        g.setColour (kPink.withAlpha (0.45f * z));
        g.drawHorizontalLine ((int) y, 0.0f, w);
    }
    g.restoreState();
}

void EuroTuneAudioProcessorEditor::drawParticles (juce::Graphics& g)
{
    for (auto& q : particles)
    {
        const float a = 0.25f + 0.5f * (0.5f + 0.5f * std::sin (t * 2.0f + q.phase));
        g.setColour (q.c.withAlpha (a * 0.8f));
        const float s = q.size * (1.0f + lvl * 1.5f);
        g.fillEllipse (q.x, q.y, s, s);
    }
}

void EuroTuneAudioProcessorEditor::drawFigure (juce::Graphics& g)
{
    using namespace juce;
    // Glow hinter der Figur
    const float cx = 185.0f, cy = 310.0f;
    const float a = jlimit (0.0f, 1.0f, introT * 1.5f);
    {
        const float r = 250.0f + lvl * 80.0f + std::sin (t * 2.0f) * 15.0f;
        ColourGradient gl (kPink.withAlpha ((0.28f + lvl * 0.35f) * a), cx, cy, Colours::transparentBlack, cx + r, cy, true);
        g.setGradientFill (gl); g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        ColourGradient gl2 (kCyan.withAlpha ((0.14f + lvl * 0.2f) * a), cx + 60.0f, cy + 80.0f, Colours::transparentBlack, cx + 60.0f + r * 0.8f, cy + 80.0f, true);
        g.setGradientFill (gl2); g.fillEllipse (cx + 60.0f - r, cy + 80.0f - r, r * 2.0f, r * 2.0f);
    }
    if (! figure.isValid()) return;

    const float e = easeOutBack (introT);
    const float bob = std::sin (t * 1.7f) * 9.0f;
    const float sc = (0.7f + 0.3f * e) * (1.0f + lvl * 0.07f);
    const float tilt = std::sin (t * 0.9f) * 0.04f + (1.0f - juce::jlimit (0.0f, 1.0f, introT)) * -0.6f;
    const float slideX = (1.0f - juce::jlimit (0.0f, 1.0f, introT)) * -320.0f;

    const float fw = (float) figure.getWidth(), fh = (float) figure.getHeight();
    const float fx = cx - fw * 0.5f, fy = cy - fh * 0.5f + 5.0f;

    g.saveState();
    g.addTransform (AffineTransform::scale (sc, sc, cx, cy).rotated (tilt, cx, cy).translated (slideX, bob));

    // Neon-Outline (Silhouette in mehreren Richtungen versetzt)
    const float o = 3.0f + lvl * 9.0f + std::sin (t * 5.0f) * 1.0f;
    const Colour oc = kPink.interpolatedWith (kAcid, 0.5f + 0.5f * std::sin (t * 1.3f));
    for (int k = 0; k < 4; ++k)
    {
        const float ang = (float) k * MathConstants<float>::halfPi + MathConstants<float>::pi * 0.25f;
        g.setColour (oc.withAlpha (0.7f * a));
        g.drawImage (figure, Rectangle<float> (fx + std::cos (ang) * o, fy + std::sin (ang) * o, fw, fh),
                     RectanglePlacement::stretchToFit, true);
    }

    // RGB-Split
    const float split = 2.0f + lvl * 14.0f + (glitchLeft > 0 ? 10.0f * glitchPower : 0.0f);
    g.setColour (kCyan.withAlpha (0.55f * a));
    g.drawImage (figure, Rectangle<float> (fx - split, fy, fw, fh), RectanglePlacement::stretchToFit, true);
    g.setColour (kRed.withAlpha (0.5f * a));
    g.drawImage (figure, Rectangle<float> (fx + split, fy, fw, fh), RectanglePlacement::stretchToFit, true);

    // Hauptbild (bei Glitch in Streifen verschoben)
    g.setOpacity (a);
    if (glitchLeft > 0.0f)
    {
        const int strips = 10;
        const float sh = fh / (float) strips;
        for (int s = 0; s < strips; ++s)
        {
            g.saveState();
            g.reduceClipRegion (Rectangle<int> ((int) fx - 80, (int) (fy + (float) s * sh), (int) fw + 160, (int) sh + 1));
            const float dx = (rng.nextFloat() - 0.5f) * 55.0f * glitchPower * (rng.nextFloat() < 0.5f ? 0.0f : 1.0f);
            g.setOpacity (a);
            g.drawImageAt (figure, (int) (fx + dx), (int) fy);
            g.restoreState();
        }
    }
    else
        g.drawImageAt (figure, (int) fx, (int) fy);
    g.restoreState();
}

void EuroTuneAudioProcessorEditor::drawEqualizer (juce::Graphics& g)
{
    const int bars = 34;
    const float x0 = 22.0f, bw = 6.0f, gap = 4.0f, baseY = 628.0f;
    const float a = juce::jlimit (0.0f, 1.0f, introT * 1.5f);
    for (int i = 0; i < bars; ++i)
    {
        const float wob = 0.5f + 0.5f * std::sin (t * 6.0f + (float) i * 0.7f) * std::sin (t * 2.3f + (float) i * 0.25f);
        const float hgt = 3.0f + (lvl * 34.0f + 3.0f) * wob;
        const auto c = kPink.interpolatedWith (kCyan, (float) i / (float) bars);
        g.setColour (c.withAlpha (0.85f * a));
        g.fillRoundedRectangle (x0 + (float) i * (bw + gap), baseY - hgt, bw, hgt, 2.0f);
    }
}

void EuroTuneAudioProcessorEditor::drawTitle (juce::Graphics& g)
{
    using namespace juce;
    const Rectangle<float> r (372.0f, 8.0f, 590.0f, 62.0f);
    const float a = jlimit (0.0f, 1.0f, introT * 2.0f);
    const float off = (glitchLeft > 0.0f ? (2.0f + rng.nextFloat() * 8.0f) * glitchPower : 1.5f + std::sin (t * 3.0f)) ;
    const String s = "EuroTune";

    g.setFont (F (62.0f, Font::bold | Font::italic, 0.02f));
    g.setColour (kCyan.withAlpha (0.8f * a));
    g.drawText (s, r.translated (-off, 0.0f), Justification::centredLeft);
    g.setColour (kPink.withAlpha (0.8f * a));
    g.drawText (s, r.translated (off, 0.0f), Justification::centredLeft);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (a), r.getX(), r.getY(),
                                       kAcid.withAlpha (a), r.getX() + 300.0f, r.getBottom(), false));
    g.drawText (s, r, Justification::centredLeft);

    // Untertitel + wandernder Lichtstreifen
    g.setColour (kAcid.withAlpha (a));
    g.setFont (F (14.0f, Font::bold, 0.45f));
    g.drawText ("BY EUROJUNKIE", Rectangle<float> (376.0f, 68.0f, 300.0f, 16.0f), Justification::centredLeft);
    const float sx = 376.0f + std::fmod (t * 160.0f, 560.0f);
    g.setGradientFill (ColourGradient (Colours::transparentBlack, sx - 60.0f, 0.0f, kPink.withAlpha (0.9f * a), sx, 0.0f, false));
    g.fillRect (sx - 60.0f, 85.0f, 60.0f, 1.6f);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.fillRect (376.0f, 85.0f, 580.0f, 1.0f);

    g.setColour (Colours::white.withAlpha (0.35f * a));
    g.setFont (F (10.0f, Font::bold, 0.2f));
    g.drawText ("PRECISION VOCAL TUNING ENGINE", Rectangle<float> (700.0f, 12.0f, 256.0f, 14.0f), Justification::centredRight);
    g.setColour (kAcid.withAlpha (0.8f * a));
    g.drawText ("VST3  v1.0", Rectangle<float> (700.0f, 28.0f, 256.0f, 14.0f), Justification::centredRight);
}

void EuroTuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    const float w = (float) getWidth(), h = (float) getHeight();

    g.setGradientFill (ColourGradient (Colour (0xff170826), 0.0f, 0.0f, Colour (0xff030308), w, h, false));
    g.fillAll();

    for (int k = 0; k < 2; ++k)
    {
        const float bx = w * (0.25f + 0.55f * (float) k) + std::sin (t * (0.5f + 0.3f * (float) k)) * 40.0f;
        const float by = h * 0.35f + std::cos (t * 0.4f) * 30.0f;
        const float r = 380.0f;
        g.setGradientFill (ColourGradient ((k == 0 ? kPink : kCyan).withAlpha (0.08f + lvl * 0.14f), bx, by,
                                           Colours::transparentBlack, bx + r, by, true));
        g.fillEllipse (bx - r, by - r, r * 2.0f, r * 2.0f);
    }

    drawGrid (g);
    drawParticles (g);
    drawFigure (g);
    drawEqualizer (g);
    drawTitle (g);

    // Labels
    const float a = jlimit (0.0f, 1.0f, (introT - 0.25f) * 1.8f);
    g.setColour (Colours::white.withAlpha (0.45f * a));
    g.setFont (F (9.5f, Font::bold, 0.2f));
    g.drawText ("KEY",   370 + 295, 303, 58, 12, Justification::centred);
    g.drawText ("SCALE", 370 + 361, 303, 140, 12, Justification::centred);
    g.drawText ("RANGE", 370 + 509, 303, 81, 12, Justification::centred);
    g.drawText ("A4 REF", 370, 608, 60, 16, Justification::left);

    g.drawImageAt (overlay, 0, 0);
}
