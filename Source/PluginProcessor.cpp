#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace gparams;

juce::AudioProcessorValueTreeState::ParameterLayout GorylProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Mode",
                juce::StringArray { "Voix", "Bus Instrumental", "Bus Master" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));

    for (int i = 0; i < P_COUNT; ++i)
    {
        const auto& d = kParams[i];
        if (d.isBool)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { d.id, 1 }, d.name, d.def > 0.5f));
        }
        else
        {
            const int dec = d.dec;
            auto attrs = juce::AudioParameterFloatAttributes()
                             .withLabel (d.unit)
                             .withStringFromValueFunction ([dec] (float v, int) { return juce::String (v, dec); });
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { d.id, 1 }, d.name,
                juce::NormalisableRange<float> (d.mn, d.mx, 0.01f, d.skew), d.def, attrs));
        }
    }
    return layout;
}

GorylProcessor::GorylProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GORYL_STATE", createLayout())
{
    for (int i = 0; i < P_COUNT; ++i) p[i] = apvts.getRawParameterValue (kParams[i].id);
    pBypass = apvts.getRawParameterValue ("bypass");
}

bool GorylProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void GorylProcessor::prepareToPlay (double sampleRate, int)
{
    fs = sampleRate;
    for (auto& e : eq) { e.hp1.reset(); e.hp2.reset(); e.body.reset(); e.boxy.reset(); e.pres.reset(); e.air.reset(); }
    deesser.prepare (fs);
    rider.prepare (fs);
    comp.prepare (fs);
    limiter.prepare (fs);
    for (auto& s : sideHp) s.reset();
    meter.prepare (fs);
    inG = gdsp::dbToLin (*p[P_IN]);
    outG = gdsp::dbToLin (*p[P_OUT]);
    setLatencySamples (limiter.latency());
}

void GorylProcessor::updateCoefficients()
{
    auto v = [this] (int i) { return (double) p[i]->load(); };
    for (auto& e : eq)
    {
        e.hp1.highPass (fs, v (P_HPF), 0.5412);   // Butterworth 4e ordre = 2 sections
        e.hp2.highPass (fs, v (P_HPF), 1.3066);
        e.body.lowShelf (fs, 120.0, v (P_BODY));
        e.boxy.peak (fs, 320.0, 1.2, v (P_BOXY));
        e.pres.peak (fs, 3200.0, 0.9, v (P_PRES));
        e.air.highShelf (fs, 11000.0, v (P_AIR));
    }
    deesser.setFreq (v (P_DSFREQ));
    comp.setParams (v (P_CTHR), v (P_CRATIO), v (P_CATK), v (P_CREL), v (P_CKNEE), v (P_CMK));
    for (auto& s : sideHp) { s.highPass (fs, 120.0, 0.5412); }
}

void GorylProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    if (pBypass->load() < 0.5f)
    {
        updateCoefficients();
        const double inT = gdsp::dbToLin (p[P_IN]->load()), outT = gdsp::dbToLin (p[P_OUT]->load());
        const double dsThr = p[P_DSTHR]->load();
        const double riderAmt = p[P_RIDER]->load() / 100.0, riderTgt = p[P_RIDERTGT]->load();
        const double satD = p[P_SATD]->load() / 100.0, satM = p[P_SATM]->load() / 100.0;
        const double drive = 1.0 + satD * 4.0;
        const double width = p[P_WIDTH]->load() / 100.0;
        const bool bassMono = p[P_BASSMONO]->load() > 0.5f;
        const double ceilLin = gdsp::dbToLin (p[P_CEIL]->load());
        const double limRel = std::exp (-1.0 / (0.001 * std::max (5.0f, p[P_LIMREL]->load()) * fs));
        const double smooth = 1.0 - std::exp (-1.0 / (0.010 * fs));
        float gr = 0.f;
        limiter.lastMin = 1.0;

        for (int i = 0; i < n; ++i)
        {
            inG  += (inT  - inG)  * smooth;
            outG += (outT - outG) * smooth;

            double l = (double) L[i] * inG, r = (double) R[i] * inG;

            l = eq[0].process (l);
            r = eq[1].process (r);
            deesser.process (l, r, dsThr);
            rider.process (l, r, riderAmt, riderTgt);
            comp.process (l, r);

            if (satD > 0.001)
            {
                const double wl = std::tanh (drive * l) / drive, wr = std::tanh (drive * r) / drive;
                l = l * (1.0 - satM) + wl * satM;
                r = r * (1.0 - satM) + wr * satM;
            }

            {   // largeur stéréo + basses en mono
                const double m = 0.5 * (l + r);
                double s = 0.5 * (l - r) * width;
                if (bassMono) s = sideHp[1].process (sideHp[0].process (s));
                l = m + s; r = m - s;
            }

            l *= outG; r *= outG;
            limiter.process (l, r, ceilLin, limRel);
            L[i] = (float) l; R[i] = (float) r;
            gr = std::min (gr, (float) comp.grDb);
        }
        compGrDb.store (gr);
        limGrDb.store ((float) gdsp::linToDb (limiter.lastMin));
    }
    meter.process (L, R, n);
}

void GorylProcessor::applyPreset (int idx)
{
    if (idx < 0 || idx >= kNumPresets) return;
    const auto& pr = kPresets[idx];
    auto setParam = [this] (const char* id, float value)
    {
        if (auto* prm = apvts.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
            prm->endChangeGesture();
        }
    };
    setParam ("mode", (float) pr.mode);
    for (int i = 0; i < P_COUNT; ++i) setParam (kParams[i].id, pr.v[(size_t) i]);
    apvts.state.setProperty ("preset", idx, nullptr);
}

juce::AudioProcessorEditor* GorylProcessor::createEditor() { return new GorylEditor (*this); }

void GorylProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void GorylProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GorylProcessor(); }
