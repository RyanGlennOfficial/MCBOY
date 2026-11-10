#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include <memory>
#include <array>
#include <functional>

class McBoyAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                      private juce::Timer {
public:
    explicit McBoyAudioProcessorEditor(McBoyAudioProcessor&);
    ~McBoyAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    struct LockSlider final : juce::Slider {
        std::function<void()> onLock;
        void mouseDown(const juce::MouseEvent& e) override {
            if(e.mods.isPopupMenu() || e.mods.isAltDown()) {if(onLock)onLock();return;}
            Slider::mouseDown(e);
        }
        void mouseDoubleClick(const juce::MouseEvent&) override {showTextBox();}
    };
    struct MiniKnob final : juce::Component {
        LockSlider slider;
        juce::Label text;
        int index=0;
        bool locked=false,available=false;
        MiniKnob(int number,bool isAvailable);
        void resized() override;
        void paintOverChildren(juce::Graphics&) override;
        void setLabels(bool show);
    };
    struct BigButton final : juce::TextButton {
        BigButton();
        void paintButton(juce::Graphics&,bool hovered,bool down) override;
    };
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
    void timerCallback() override;
    void updateDescriptions();
    void copyState();
    void pasteState();
    static juce::Colour bankColour(int bank);

    McBoyAudioProcessor& processor;
    std::array<std::unique_ptr<MiniKnob>,100> knobs;
    std::array<std::unique_ptr<SliderAttachment>,100> knobAttach;
    BigButton shuffle;
    juce::Slider degree,mix,output;
    juce::ToggleButton labels,guard,bypass;
    juce::TextButton saveCompare{"STORE A/B"}, swapCompare{"SWAP A/B"};
    juce::TextButton undo{"UNDO"},redo{"REDO"},clear{"RESET"},
                     lockAll{"LOCK ALL"},unlockAll{"UNLOCK ALL"},
                     copy{"COPY"},paste{"PASTE"};
    juce::ComboBox preset;
    juce::Label degreeLabel,mixLabel,outputLabel,info;
    std::unique_ptr<SliderAttachment> degreeAttach,mixAttach,outputAttach;
    std::unique_ptr<ButtonAttachment> labelsAttach,guardAttach,bypassAttach;
    bool lastLabels=false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(McBoyAudioProcessorEditor)
};
