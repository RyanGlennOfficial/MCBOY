#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>
#include "Core/Engine.h"
#include "Presets.h"

class McBoyAudioProcessor final : public juce::AudioProcessor {
public:
    McBoyAudioProcessor();
    ~McBoyAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    const juce::String getName() const override {return "McBoy";}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override {return 3.0;}
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {return "State";}
    void changeProgramName(int,const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void* data,int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;
    bool isLocked(size_t index) const noexcept {return locks[index].load(std::memory_order_relaxed);}
    void toggleLock(size_t index) noexcept {locks[index].store(!isLocked(index),std::memory_order_relaxed);}
    void lockAll(bool locked) noexcept;
    uint64_t getSeed() const noexcept {return patchSeed.load(std::memory_order_relaxed);}
    void randomize();
    void undoShuffle();
    void redoShuffle();
    void resetNeutral();
    void storeCompare();
    void swapCompare();
    void applyFactoryPreset(int index);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
private:
    struct Snapshot {
        std::array<float,100> raw{};
        std::array<bool,100> locked{};
        float degree=.72f,mix=1.f,output=0.f;
        bool guard=true,bypass=false,labels=false;
        uint64_t seed=0xA74E12B50B0C0101ULL;
    };
    Snapshot capture() const;
    void applySnapshot(const Snapshot&);
    void pushUndo();
    static juce::ValueTree snapshotToTree(const Snapshot&);
    static Snapshot snapshotFromTree(const juce::ValueTree&);
    void setParam(const juce::String& id, float value);

    mcboy::Engine engine;
    std::array<std::atomic<float>*,100> rawPointers{};
    std::atomic<float>* degreePtr=nullptr;
    std::atomic<float>* mixPtr=nullptr;
    std::atomic<float>* outputPtr=nullptr;
    std::atomic<float>* guardPtr=nullptr;
    std::atomic<float>* bypassPtr=nullptr;
    std::array<std::atomic<bool>,100> locks{};
    std::atomic<uint64_t> patchSeed{0xA74E12B50B0C0101ULL};
    Snapshot compareSnapshot{};
    std::vector<Snapshot> undoHistory, redoHistory;
    static constexpr size_t maxHistory=20;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(McBoyAudioProcessor)
};
