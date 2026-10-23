#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace ui
{
static const juce::Colour kAcid (0xffb6ff00), kPink (0xffff2d95), kCyan (0xff00e5ff),
                          kPurple (0xff8a3cff), kRed (0xffff3b30), kBg (0xff07070d);

inline juce::Font F (float h, int style = juce::Font::plain, float kern = 0.0f)
{
    return juce::Font (juce::FontOptions (h, style).withKerningFactor (kern));
}

//==============================================================================
class NeonLook : public juce::LookAndFeel_V4
{
public:
    NeonLook()
    {
        using namespace juce;
        setColour (ComboBox::backgroundColourId, Colour (0xff10101c));
        setColour (ComboBox::outlineColourId, kPurple.withAlpha (0.7f));
        setColour (ComboBox::textColourId, Colours::white);
        setColour (ComboBox::arrowColourId, kAcid);
        setColour (PopupMenu::backgroundColourId, Colour (0xff0d0d18));
        setColour (PopupMenu::textColourId, Colours::white);
        setColour (PopupMenu::highlightedBackgroundColourId, kPink);
        setColour (PopupMenu::highlightedTextColourId, Colours::black);
        setColour (Slider::thumbColourId, kAcid);
        setColour (Slider::trackColourId, kPink);
        setColour (Slider::backgroundColourId, Colour (0xff1c1c2b));
        setColour (Slider::textBoxTextColourId, Colours::white.withAlpha (0.85f));
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        using namespace juce;
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (7.0f);
        const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const bool hot = s.isMouseOverOrDragging();
        const float ang = startA + pos * (endA - startA);

        Path track; track.addCentredArc (c.x, c.y, r, r, 0.0f, startA, endA, true);
        g.setColour (Colour (0xff1c1c2b));
        g.strokePath (track, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));

        Path val; val.addCentredArc (c.x, c.y, r, r, 0.0f, startA, jmax (startA + 0.001f, ang), true);
        const Colour col = kAcid.interpolatedWith (kPink, pos);
        const float pulse = 0.75f + 0.25f * std::sin (t * 4.0f);
        for (int i = 3; i > 0; --i)
        {
            g.setColour (col.withAlpha (0.07f * (float) i * (hot ? 1.8f : pulse)));
            g.strokePath (val, PathStrokeType (5.0f + (float) i * 4.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (col);
        g.strokePath (val, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

        const float rb = r * 0.68f;
        g.setGradientFill (ColourGradient (Colour (0xff42425f), c.x - rb * 0.5f, c.y - rb * 0.6f,
                                           Colour (0xff0c0c15), c.x + rb * 0.6f, c.y + rb * 0.8f, false));
        g.fillEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f);
        g.setColour (col.withAlpha (hot ? 0.95f : 0.55f));
        g.drawEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f, hot ? 2.2f : 1.4f);

        Path p; p.addRoundedRectangle (-1.8f, -rb + 3.0f, 3.6f, rb * 0.5f, 1.5f);
        g.setColour (Colours::white);
        g.fillPath (p, AffineTransform::rotation (ang).translated (c.x, c.y));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        using namespace juce;
        auto r = b.getLocalBounds().toFloat().reduced (1.5f);
        const bool on = b.getToggleState();
        const Colour c = on ? kRed : Colour (0xff4a4a66);
        g.setColour (c.withAlpha (on ? 0.28f : 0.10f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (c);
        g.drawRoundedRectangle (r, 8.0f, hl ? 2.4f : 1.4f);
        g.setColour (on ? Colours::white : Colours::grey);
        g.setFont (F (12.0f, Font::bold, 0.15f));
        g.drawText (b.getButtonText(), r, Justification::centred);
    }

    float t = 0.0f;
};

//==============================================================================
class KnobComp : public juce::Component
{
public:
    KnobComp (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& name)
        : title (name.toUpperCase())
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (slider);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, slider);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (kAcid.withAlpha (0.9f));
        g.setFont (F (11.5f, juce::Font::bold, 0.18f));
        g.drawText (title, 0, 0, getWidth(), 15, juce::Justification::centred);
    }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedTop (14)); }

    juce::Slider slider;
private:
    juce::String title;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

//==============================================================================
class ModeSelector : public juce::Component
{
public:
    explicit ModeSelector (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

    void tick()
    {
        const float m = apvts.getRawParameterValue ("mode")->load();
        pos += (m - pos) * 0.2f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        static const char* names[] = { "CLEAN", "CLASSIC", "ROBOT", "EXTREME" };
        const Colour cols[] = { kCyan, kAcid, kPink, kRed };
        auto b = getLocalBounds().toFloat();
        g.setColour (Colour (0xff10101c));
        g.fillRoundedRectangle (b, 10.0f);
        g.setColour (kPurple.withAlpha (0.6f));
        g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.2f);

        const float w = b.getWidth() / 4.0f;
        const int i0 = jlimit (0, 3, (int) std::floor (pos));
        const int i1 = jmin (3, i0 + 1);
        const Colour c = cols[i0].interpolatedWith (cols[i1], pos - (float) i0);
        auto hi = Rectangle<float> (b.getX() + pos * w, b.getY(), w, b.getHeight()).reduced (3.0f);
        for (int i = 4; i > 0; --i)
        {
            g.setColour (c.withAlpha (0.08f));
            g.fillRoundedRectangle (hi.expanded ((float) i * 2.0f), 9.0f);
        }
        g.setColour (c);
        g.fillRoundedRectangle (hi, 8.0f);

        g.setFont (F (13.0f, Font::bold, 0.2f));
        for (int i = 0; i < 4; ++i)
        {
            const float d = jlimit (0.0f, 1.0f, std::fabs (pos - (float) i));
            g.setColour (Colours::white.interpolatedWith (Colours::black, 1.0f - d).withAlpha (0.55f + 0.45f * (1.0f - d)));
            g.drawText (names[i], Rectangle<float> (b.getX() + (float) i * w, b.getY(), w, b.getHeight()), Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int idx = juce::jlimit (0, 3, (int) (e.position.x / ((float) getWidth() / 4.0f)));
        if (auto* p = apvts.getParameter ("mode"))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));
            p->endChangeGesture();
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    float pos = 2.0f;
};

//==============================================================================
class NoteBar : public juce::Component
{
public:
    NoteBar (juce::AudioProcessorValueTreeState& s, EuroTuneAudioProcessor& p) : apvts (s), proc (p)
    {
        for (int i = 0; i < 12; ++i) { flash[i] = 0.0f; ptr[i] = apvts.getRawParameterValue ("note_" + juce::String (i)); }
    }

    void tick()
    {
        const float in = proc.tInMidi.load(), tg = proc.tTargetMidi.load();
        targetPc = tg >= 0 ? (((int) std::lround (tg) % 12) + 12) % 12 : -1;
        for (int i = 0; i < 12; ++i)
            flash[i] += ((i == targetPc ? 1.0f : 0.0f) - flash[i]) * (i == targetPc ? 0.4f : 0.12f);
        if (in >= 0)
        {
            const float pc = std::fmod (in, 12.0f);
            float d = pc - marker;
            while (d > 6.0f) d -= 12.0f;
            while (d < -6.0f) d += 12.0f;
            marker += d * 0.35f;
            if (marker < 0) marker += 12.0f;
            if (marker >= 12.0f) marker -= 12.0f;
            markerA += (1.0f - markerA) * 0.2f;
        }
        else markerA *= 0.85f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        static const bool blk[] = { 0,1,0,1,0,0,1,0,1,0,1,0 };
        auto b = getLocalBounds().toFloat();
        const float gap = 4.0f, kw = (b.getWidth() - gap * 11.0f) / 12.0f;

        for (int i = 0; i < 12; ++i)
        {
            auto r = Rectangle<float> (b.getX() + (float) i * (kw + gap), b.getY(), kw, b.getHeight());
            const bool on = ptr[i]->load() > 0.5f;
            const float f = flash[i];
            const Colour base = blk[i] ? kPurple : kCyan;
            if (f > 0.05f)
                for (int k = 3; k > 0; --k) { g.setColour (kAcid.withAlpha (0.12f * f)); g.fillRoundedRectangle (r.expanded ((float) k * 2.5f), 8.0f); }
            g.setColour (on ? base.withAlpha (0.22f + 0.4f * f) : Colour (0xff11111b));
            g.fillRoundedRectangle (r, 7.0f);
            if (f > 0.05f) { g.setColour (kAcid.withAlpha (0.55f * f)); g.fillRoundedRectangle (r, 7.0f); }
            g.setColour (on ? base.withAlpha (0.9f) : Colour (0x40ffffff));
            g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.3f);
            g.setColour (f > 0.4f ? Colours::black : (on ? Colours::white : Colours::grey));
            g.setFont (F (12.0f, Font::bold));
            g.drawText (names[i], r, Justification::centred);
        }

        if (markerA > 0.05f)
        {
            const float x = b.getX() + (marker + 0.5f) / 12.0f * b.getWidth();
            for (int k = 4; k > 0; --k) { g.setColour (kPink.withAlpha (0.10f * markerA)); g.fillRect (x - (float) k * 1.5f, b.getY() - 2.0f, (float) k * 3.0f, b.getHeight() + 4.0f); }
            g.setColour (Colours::white.withAlpha (0.9f * markerA));
            g.fillRect (x - 1.0f, b.getY() - 2.0f, 2.0f, b.getHeight() + 4.0f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, 11, (int) (e.position.x / ((float) getWidth() / 12.0f)));
        if (auto* p = apvts.getParameter ("note_" + juce::String (i)))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
            p->endChangeGesture();
        }
        // manuelles Editieren -> Scale auf "Custom"
        if (auto* sp = apvts.getParameter ("scale"))
        {
            sp->beginChangeGesture();
            sp->setValueNotifyingHost (sp->convertTo0to1 ((float) EuroTuneAudioProcessor::customScaleIndex()));
            sp->endChangeGesture();
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    EuroTuneAudioProcessor& proc;
    std::atomic<float>* ptr[12] {};
    float flash[12];
    int targetPc = -1;
    float marker = 0.0f, markerA = 0.0f;
};

//==============================================================================
class PitchGraph : public juce::Component
{
public:
    PitchGraph (juce::AudioProcessorValueTreeState& s, EuroTuneAudioProcessor& p) : apvts (s), proc (p)
    {
        for (int i = 0; i < 12; ++i) ptr[i] = apvts.getRawParameterValue ("note_" + juce::String (i));
        pts.reserve (N + 2);
    }

    void tick (float t_)
    {
        t = t_;
        const Pt p { proc.tInMidi.load(), proc.tOutMidi.load(), proc.tTargetMidi.load() };
        pts.push_back (p);
        if ((int) pts.size() > N) pts.erase (pts.begin());
        if (p.in >= 0) { centre += (p.in - centre) * (hasCentre ? 0.04f : 1.0f); hasCentre = true; }
        if (p.tgt >= 0) { noteFade = 1.0f; lastTgt = p.tgt; } else noteFade *= 0.96f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto b = getLocalBounds().toFloat();
        g.setColour (Colour (0xcc08080f));
        g.fillRoundedRectangle (b, 12.0f);
        g.setColour (kPurple.withAlpha (0.55f + 0.2f * std::sin (t * 2.0f)));
        g.drawRoundedRectangle (b.reduced (0.5f), 12.0f, 1.4f);

        auto inner = b.reduced (6.0f, 8.0f);
        const float span = 14.0f;
        auto toY = [&] (float m) { return inner.getCentreY() - (m - centre) / span * inner.getHeight(); };

        g.saveState();
        g.reduceClipRegion (inner.toNearestInt());
        static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        for (int n = (int) std::floor (centre - span * 0.5f) - 1; n <= (int) std::ceil (centre + span * 0.5f) + 1; ++n)
        {
            const int pc = ((n % 12) + 12) % 12;
            const bool on = ptr[pc]->load() > 0.5f;
            const float y = toY ((float) n);
            g.setColour ((on ? kCyan : Colours::white).withAlpha (on ? 0.22f : 0.04f));
            g.drawHorizontalLine ((int) y, inner.getX(), inner.getRight());
            if (on)
            {
                g.setColour (kCyan.withAlpha (0.6f));
                g.setFont (F (10.0f, Font::bold));
                g.drawText (String (names[pc]) + String (n / 12 - 1), (int) inner.getX() + 4, (int) y - 11, 40, 11, Justification::left);
            }
        }

        auto build = [&] (auto getter)
        {
            Path p; bool pen = false;
            const int n = (int) pts.size();
            for (int i = 0; i < n; ++i)
            {
                const float v = getter (pts[(size_t) i]);
                if (v < 0) { pen = false; continue; }
                const float x = inner.getX() + (float) i / (float) (N - 1) * inner.getWidth();
                const float y = toY (v);
                if (! pen) { p.startNewSubPath (x, y); pen = true; } else p.lineTo (x, y);
            }
            return p;
        };
        auto stroke = [&] (const Path& p, Colour c, float w, bool glow)
        {
            if (glow) for (int k = 3; k > 0; --k) { g.setColour (c.withAlpha (0.09f * (float) k)); g.strokePath (p, PathStrokeType (w + (float) k * 4.0f, PathStrokeType::curved, PathStrokeType::rounded)); }
            g.setColour (c);
            g.strokePath (p, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));
        };
        stroke (build ([] (const Pt& q) { return q.tgt; }), kCyan.withAlpha (0.8f), 1.5f, false);
        stroke (build ([] (const Pt& q) { return q.in; }), Colours::white.withAlpha (0.55f), 1.4f, false);
        stroke (build ([] (const Pt& q) { return q.out; }), kAcid, 2.6f, true);
        g.restoreState();

        // Aktuelle Zielnote gross
        if (noteFade > 0.05f && lastTgt >= 0)
        {
            const int n = (int) std::lround (lastTgt);
            g.setColour (kPink.withAlpha (noteFade));
            g.setFont (F (30.0f, Font::bold | Font::italic));
            g.drawText (String (names[((n % 12) + 12) % 12]) + String (n / 12 - 1), b.reduced (14.0f, 6.0f), Justification::topRight);
        }
        g.setColour (Colours::white.withAlpha (0.35f));
        g.setFont (F (9.5f, Font::bold, 0.15f));
        g.drawText ("IN", (int) b.getRight() - 150, (int) b.getBottom() - 16, 20, 12, Justification::left);
        g.setColour (kAcid);
        g.drawText ("OUT", (int) b.getRight() - 120, (int) b.getBottom() - 16, 30, 12, Justification::left);
        g.setColour (kCyan);
        g.drawText ("TARGET", (int) b.getRight() - 80, (int) b.getBottom() - 16, 50, 12, Justification::left);
    }

private:
    struct Pt { float in, out, tgt; };
    static constexpr int N = 260;
    juce::AudioProcessorValueTreeState& apvts;
    EuroTuneAudioProcessor& proc;
    std::atomic<float>* ptr[12] {};
    std::vector<Pt> pts;
    float centre = 60.0f, noteFade = 0.0f, lastTgt = -1.0f, t = 0.0f;
    bool hasCentre = false;
};
} // namespace ui
