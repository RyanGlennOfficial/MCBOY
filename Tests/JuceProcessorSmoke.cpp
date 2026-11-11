// Compile/run on macOS with JUCE. Does not substitute for actual VST3 host tests.
#include "../Source/PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool test, const char* message) {
    if (!test) throw std::runtime_error(message);
}
void automate(McBoyAudioProcessor& processor, const juce::String& id, float value) {
    auto* p = dynamic_cast<juce::RangedAudioParameter*>(processor.state.getParameter(id));
    check(p != nullptr, "Missing automatable JUCE parameter");
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(value));
    p->endChangeGesture();
}
void fill(juce::AudioBuffer<float>& buffer) {
    for (int c=0; c<buffer.getNumChannels(); ++c)
        for (int i=0; i<buffer.getNumSamples(); ++i)
            buffer.setSample(c,i,.25f*std::sin(.031f*float(i)));
}
}
int main() {
    try {
        juce::ScopedJuceInitialiser_GUI initialiseJuce;
        McBoyAudioProcessor instance;
        check(instance.getParameters().size()==106, "Expected 100 fx + 6 utility host parameters");
        check(instance.state.getRawParameterValue("labels")->load() < .5f, "Labels should default OFF");
        check(std::abs(instance.state.getRawParameterValue("degree")->load()-.72f)<.001f,
              "Degree should default to 0.72");
        for(int i=1;i<=100;++i){
            auto id=juce::String::formatted("fx%03d",i);
            check(instance.state.getParameter(id)!=nullptr, "Missing FX automation parameter");
        }
        instance.prepareToPlay(48000.,512);
        automate(instance,"fx001",.78f);
        automate(instance,"degree",.95f);
        automate(instance,"mix",1.f);
        instance.toggleLock(0);
        check(instance.isLocked(0), "Lock failed");
        juce::AudioBuffer<float> input(2,512);
        juce::MidiBuffer midi;
        for(int n=0;n<10;++n){fill(input);instance.processBlock(input,midi);}
        for(int c=0;c<2;++c)for(int i=0;i<input.getNumSamples();++i)
            check(std::isfinite(input.getSample(c,i)), "Nonfinite JUCE wrapped output");
        const auto originalSeed=instance.getSeed();
        instance.randomize();
        check(instance.getSeed()!=originalSeed, "Randomize should move seed forward");
        check(std::abs(instance.state.getRawParameterValue("fx001")->load()-.78f)<.002f,
              "Locked parameter changed during randomization");
        const auto seedAfterRandomize=instance.getSeed();
        instance.undoShuffle();
        check(instance.getSeed()==originalSeed, "Undo did not restore seed");
        instance.redoShuffle();
        check(instance.getSeed()==seedAfterRandomize, "Redo did not restore seed");
        instance.storeCompare();
        const float storedFX49=instance.state.getRawParameterValue("fx049")->load();
        const float alternateFX49=storedFX49>.50f ? .10f : .71f;
        automate(instance,"fx049",alternateFX49);
        instance.swapCompare();
        check(std::abs(instance.state.getRawParameterValue("fx049")->load()-storedFX49)<.002f,
              "A/B swap did not recall stored state");
        instance.swapCompare();
        check(std::abs(instance.state.getRawParameterValue("fx049")->load()-alternateFX49)<.002f,
              "A/B swap did not restore alternate state");
        automate(instance,"labels",1.f);
        juce::MemoryBlock bytes;
        instance.getStateInformation(bytes);
        check(bytes.getSize()>0, "State did not serialize");
        McBoyAudioProcessor reload;
        reload.prepareToPlay(48000.,512);
        reload.setStateInformation(bytes.getData(),int(bytes.getSize()));
        check(reload.getSeed()==instance.getSeed(), "State reload lost PRNG seed");
        check(reload.isLocked(0), "State reload lost lock status");
        check(reload.state.getRawParameterValue("labels")->load()>.5f,
              "State reload lost label mode");
        for(int i=1;i<=100;++i){
            const auto id=juce::String::formatted("fx%03d",i);
            const float expected=instance.state.getRawParameterValue(id)->load();
            const float actual=reload.state.getRawParameterValue(id)->load();
            check(std::abs(expected-actual)<.0002f,"State reload lost a raw effect value");
        }
        reload.swapCompare();
        check(std::abs(reload.state.getRawParameterValue("fx049")->load()-storedFX49)<.002f,
              "State reload lost A/B comparison slot");
        reload.applyFactoryPreset(1);
        check(std::abs(reload.state.getRawParameterValue("fx011")->load()-.85f)<.002f,
              "Factory preset did not restore Plate Glow to its defined 0.85 value");
        reload.resetNeutral();
        check(reload.state.getRawParameterValue("fx011")->load()<.0001f,
              "Neutral reset failed");
        std::cout<<"PASS JUCE parameter automation, process, state, lock, randomization, "
                    "undo/redo, preset, neutral reset\n";
        return 0;
    } catch(const std::exception& e){
        std::cerr<<"FAIL JUCE test: "<<e.what()<<'\n';
        return 1;
    }
}
