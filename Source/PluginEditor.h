#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// Look néon bleu
struct NeonLnF : public juce::LookAndFeel_V4
{
    static juce::Colour neon()  { return juce::Colour (0xff00d0ff); }
    static juce::Colour amber() { return juce::Colour (0xffffb300); }

    NeonLnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class GorylEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GorylEditor (GorylProcessor&);
    ~GorylEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint (meterArea); }
    void populatePresets();
    void drawMeter (juce::Graphics&);

    GorylProcessor& proc;
    NeonLnF lnf;

    struct Knob
    {
        int paramIndex = 0;
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    std::vector<std::unique_ptr<Knob>> knobs;

    juce::ToggleButton bassMonoBtn { "Basses mono" }, bypassBtn { "Bypass" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bassMonoAtt, bypassAtt;
    juce::ComboBox modeBox, presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt;
    juce::TextButton resetBtn { "Reset LUFS" };
    std::vector<int> shown;
    juce::Rectangle<int> meterArea;
    float smoothSt = -100.f, smoothMom = -100.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GorylEditor)
};
