#include "PluginEditor.h"

using namespace gparams;

static constexpr int kRows[3][10] = {
    { P_IN, P_HPF, P_BODY, P_BOXY, P_PRES, P_AIR, -1, -1, -1, -1 },
    { P_DSTHR, P_DSFREQ, P_RIDER, P_RIDERTGT, P_CTHR, P_CRATIO, P_CATK, P_CREL, P_CKNEE, P_CMK },
    { P_SATD, P_SATM, P_WIDTH, P_BASSMONO, P_CEIL, P_LIMREL, P_OUT, -1, -1, -1 } };
static const char* kRowTitles[3] = { "ENTRÉE  &  ÉGALISEUR",
                                     "DE-ESSER  ·  NIVEAU AUTO  ·  COMPRESSEUR",
                                     "COULEUR  ·  LARGEUR  ·  LIMITEUR  ·  SORTIE" };

static juce::Font mkFont (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
NeonLnF::NeonLnF()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffcfeeff));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colour (0xff8fb8d6));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0b1a2e));
    setColour (juce::ComboBox::outlineColourId, neon().withAlpha (0.6f));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffe6f7ff));
    setColour (juce::ComboBox::arrowColourId, neon());
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0b1a2e));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe6f7ff));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, neon().withAlpha (0.3f));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0b1a2e));
    setColour (juce::TextButton::textColourOffId, neon());
}

void NeonLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                float startAngle, float endAngle, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (7.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) / 2.f;
    const auto c = b.getCentre();
    const float ang = startAngle + pos * (endAngle - startAngle);

    juce::Path track; track.addCentredArc (c.x, c.y, r, r, 0.f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff10233a));
    g.strokePath (track, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val; val.addCentredArc (c.x, c.y, r, r, 0.f, startAngle, ang, true);
    g.setColour (neon().withAlpha (0.22f));
    g.strokePath (val, juce::PathStrokeType (9.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (neon());
    g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xff0b1a2e));
    g.fillEllipse (b.withSizeKeepingCentre (r * 1.2f, r * 1.2f));
    const juce::Point<float> tip (c.x + (r - 3.f) * std::sin (ang), c.y - (r - 3.f) * std::cos (ang));
    const juce::Point<float> base (c.x + (r * 0.35f) * std::sin (ang), c.y - (r * 0.35f) * std::cos (ang));
    g.setColour (juce::Colours::white);
    g.drawLine (juce::Line<float> (base, tip), 2.f);
}

void NeonLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const bool on = b.getToggleState();
    g.setColour (on ? neon().withAlpha (0.25f) : juce::Colour (0xff0b1a2e));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (on ? neon() : neon().withAlpha (0.35f));
    g.drawRoundedRectangle (r, 6.f, 1.5f);
    g.setColour (on ? juce::Colours::white : juce::Colour (0xff8fb8d6));
    g.setFont (mkFont (12.f, true));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

//==============================================================================
GorylEditor::GorylEditor (GorylProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    setSize (910, 490);

    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 10; ++col)
        {
            const int idx = kRows[row][col];
            if (idx < 0 || kParams[idx].isBool) continue;
            auto k = std::make_unique<Knob>();
            k->paramIndex = idx;
            k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
            k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                           juce::MathConstants<float>::pi * 2.75f, true);
            k->slider.setDoubleClickReturnValue (true, kParams[idx].def);
            k->label.setText (kParams[idx].name, juce::dontSendNotification);
            k->label.setJustificationType (juce::Justification::centred);
            k->label.setFont (mkFont (11.f));
            k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kParams[idx].id, k->slider);
            addAndMakeVisible (k->slider);
            addAndMakeVisible (k->label);
            knobs.push_back (std::move (k));
        }

    addAndMakeVisible (bassMonoBtn);
    addAndMakeVisible (bypassBtn);
    bassMonoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "bassMono", bassMonoBtn);
    bypassAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "bypass", bypassBtn);

    modeBox.addItemList ({ "Voix", "Bus Instrumental", "Bus Master" }, 1);
    addAndMakeVisible (modeBox);
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "mode", modeBox);
    modeBox.onChange = [this] { populatePresets(); };

    addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("Choisir un préréglage...");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1 && id <= (int) shown.size()) proc.applyPreset (shown[(size_t) id - 1]);
    };
    populatePresets();

    addAndMakeVisible (resetBtn);
    resetBtn.onClick = [this] { proc.meter.requestReset(); };

    startTimerHz (30);
}

GorylEditor::~GorylEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void GorylEditor::populatePresets()
{
    const int mode = juce::jlimit (0, 2, modeBox.getSelectedItemIndex());
    const int current = (int) proc.apvts.state.getProperty ("preset", -1);
    shown.clear();
    presetBox.clear (juce::dontSendNotification);
    int selectId = 0;
    for (int i = 0; i < kNumPresets; ++i)
        if (kPresets[i].mode == mode)
        {
            shown.push_back (i);
            presetBox.addItem (kPresets[i].name, (int) shown.size());
            if (i == current) selectId = (int) shown.size();
        }
    if (selectId > 0) presetBox.setSelectedId (selectId, juce::dontSendNotification);
}

void GorylEditor::resized()
{
    modeBox.setBounds (300, 22, 170, 28);
    presetBox.setBounds (480, 22, 270, 28);
    bypassBtn.setBounds (765, 22, 125, 28);

    const int x0 = 16, colW = 72, y0 = 84, rowH = 132;
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 10; ++col)
        {
            const int idx = kRows[row][col];
            if (idx < 0) continue;
            const int x = x0 + col * colW, y = y0 + row * rowH + 20;
            if (kParams[idx].isBool) { bassMonoBtn.setBounds (x + 2, y + 30, colW - 4, 30); continue; }
            for (auto& k : knobs)
                if (k->paramIndex == idx)
                {
                    k->label.setBounds (x, y, colW, 14);
                    k->slider.setBounds (x, y + 14, colW, 94);
                }
        }
    meterArea = { 750, 84, 150, 396 };
    resetBtn.setBounds (770, 452, 110, 24);
}

void GorylEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff040a14), 0.f, 0.f,
                                             juce::Colour (0xff0a1a30), 0.f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (NeonLnF::neon().withAlpha (0.18f));
    g.setFont (mkFont (23.f, true));
    g.drawText ("GORYL  VOCAL LEVELER", 17, 11, 300, 30, juce::Justification::centredLeft);
    g.setColour (NeonLnF::neon());
    g.drawText ("GORYL  VOCAL LEVELER", 16, 10, 300, 30, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff6f93b0));
    g.setFont (mkFont (10.f));
    g.drawText ("voix · bus instrumental · bus master", 16, 40, 300, 14, juce::Justification::centredLeft);

    g.setColour (NeonLnF::neon().withAlpha (0.5f));
    g.drawLine (16.f, 64.f, 894.f, 64.f, 1.f);

    for (int row = 0; row < 3; ++row)
    {
        const int y = 84 + row * 132;
        g.setColour (juce::Colour (0xff10233a));
        g.fillRoundedRectangle (12.f, (float) y - 2.f, 728.f, 128.f, 8.f);
        g.setColour (NeonLnF::neon().withAlpha (0.8f));
        g.setFont (mkFont (11.f, true));
        g.drawText (kRowTitles[row], 20, y + 2, 600, 14, juce::Justification::centredLeft);
    }
    drawMeter (g);
}

void GorylEditor::drawMeter (juce::Graphics& g)
{
    auto panel = meterArea.toFloat();
    g.setColour (juce::Colour (0xff0b1a2e));
    g.fillRoundedRectangle (panel, 8.f);
    g.setColour (NeonLnF::neon().withAlpha (0.4f));
    g.drawRoundedRectangle (panel, 8.f, 1.f);

    const float st  = proc.meter.shortTerm();
    const float mom = proc.meter.momentary();
    const float integ = proc.meter.integrated();
    smoothSt  += (st  - smoothSt)  * 0.35f;
    smoothMom += (mom - smoothMom) * 0.6f;

    g.setColour (juce::Colours::white);
    g.setFont (mkFont (13.f, true));
    g.drawText ("LUFS", 750, 88, 150, 16, juce::Justification::centred);

    const float top = 112.f, bottom = 372.f, segs = 41.f;      // 0 à -40 LUFS
    const float segH = (bottom - top) / segs;
    for (int j = 0; j < 41; ++j)
    {
        const float y = top + (float) j * segH;
        const float level = -(float) j;
        const bool is14 = (j == 14);
        const bool lit = smoothSt >= level - 0.5f;
        juce::Colour c = is14 ? NeonLnF::amber() : (j < 6 ? juce::Colour (0xffff4d6d) : NeonLnF::neon());
        g.setColour (lit ? c : c.withAlpha (is14 ? 0.35f : 0.12f));
        g.fillRect (772.f, y, 36.f, segH - 1.5f);
        const bool litM = smoothMom >= level - 0.5f;
        g.setColour (litM ? juce::Colours::white.withAlpha (0.9f) : juce::Colours::white.withAlpha (0.06f));
        g.fillRect (812.f, y, 5.f, segH - 1.5f);
    }
    g.setFont (mkFont (10.f));
    const int ticks[] = { 0, -6, -9, -14, -20, -30, -40 };
    for (int t : ticks)
    {
        const float y = top + (float) (-t) * segH;
        g.setColour (t == -14 ? NeonLnF::amber() : juce::Colour (0xff8fb8d6));
        g.drawText (juce::String (t), 822, (int) y - 6, 32, 12, juce::Justification::centredLeft);
    }
    g.setColour (NeonLnF::amber());
    g.drawLine (766.f, top + 14.f * segH + segH * 0.5f, 770.f, top + 14.f * segH + segH * 0.5f, 2.f);

    auto fmt = [] (float v) { return v <= -99.f ? juce::String ("--") : juce::String (v, 1); };
    const bool near14 = st > -99.f && std::abs (smoothSt + 14.f) <= 1.0f;
    g.setFont (mkFont (11.f));
    g.setColour (juce::Colour (0xff8fb8d6));
    g.drawText ("Court terme", 760, 376, 130, 12, juce::Justification::centredLeft);
    g.setColour (near14 ? NeonLnF::amber() : juce::Colours::white);
    g.setFont (mkFont (15.f, true));
    g.drawText (fmt (st), 760, 387, 130, 18, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff8fb8d6));
    g.setFont (mkFont (11.f));
    g.drawText ("Intégré", 760, 406, 130, 12, juce::Justification::centredLeft);
    g.setColour (std::abs (integ + 14.f) <= 1.0f ? NeonLnF::amber() : juce::Colours::white);
    g.setFont (mkFont (15.f, true));
    g.drawText (fmt (integ), 760, 417, 130, 18, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff6f93b0));
    g.setFont (mkFont (10.f));
    g.drawText ("Comp " + juce::String (proc.compGrDb.load(), 1) + " dB   Lim " + juce::String (proc.limGrDb.load(), 1) + " dB",
                754, 436, 142, 12, juce::Justification::centredLeft);
}
