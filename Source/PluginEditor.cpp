#include "PluginEditor.h"
#include "EffectDescriptions.h"
#include <algorithm>

McBoyAudioProcessorEditor::MiniKnob::MiniKnob(int number,bool isAvailable)
: index(number),available(isAvailable){
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,45,15);
    slider.setRange(0.,1.,.0001);
    slider.setDoubleClickReturnValue(true,0.);
    slider.setColour(juce::Slider::rotarySliderFillColourId,
                     available?juce::Colour(0xffe89a65):juce::Colour(0xff687077));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff303541));
    slider.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffeee7df));
    slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff121820));
    slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    text.setJustificationType(juce::Justification::centred);
    text.setFont(juce::Font(juce::FontOptions(9.0f)));
    text.setColour(juce::Label::textColourId,juce::Colour(0xffd7d7e0));
    addAndMakeVisible(slider);addAndMakeVisible(text);
    setLabels(false);
}
void McBoyAudioProcessorEditor::MiniKnob::setLabels(bool show){
    const auto name=juce::String(mcboy::effectNames[size_t(index)].data());
    const auto display=show ? (name.length()>12 ? name.substring(0,11)+"…" : name)
                            : juce::String::formatted("%03d",index+1);
    text.setText(display,juce::dontSendNotification);
}
void McBoyAudioProcessorEditor::MiniKnob::resized(){
    auto b=getLocalBounds();
    slider.setBounds(b.withTrimmedBottom(12));text.setBounds(b.removeFromBottom(12));
}
void McBoyAudioProcessorEditor::MiniKnob::paintOverChildren(juce::Graphics& g){
    if(locked){
        g.setColour(juce::Colour(0xffefce73));
        auto corner=getLocalBounds().removeFromTop(12).removeFromRight(15);
        g.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::bold)));
        g.drawText("L",corner,juce::Justification::centred);
    }
    if(!available){
        g.setColour(juce::Colours::white.withAlpha(.36f));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.drawText("DEV",getLocalBounds().removeFromTop(13).removeFromLeft(30),juce::Justification::centred);
    }
}
McBoyAudioProcessorEditor::BigButton::BigButton():juce::TextButton("MCBOY!"){
    setTooltip("Randomize all unlocked effects (R or Space)");
    setTitle("Randomize unlocked effects");
}
void McBoyAudioProcessorEditor::BigButton::paintButton(juce::Graphics& g,bool hovered,bool down){
    const auto b=getLocalBounds().toFloat().reduced(6.f);
    auto circle=b.withSizeKeepingCentre(std::min(b.getWidth(),b.getHeight()),std::min(b.getWidth(),b.getHeight()));
    g.setColour(juce::Colour(0xff10151c));g.fillEllipse(circle.translated(2.f,5.f));
    juce::ColourGradient grad(down?juce::Colour(0xffd94760):(hovered?juce::Colour(0xffff7888):juce::Colour(0xffeb506b)),
                             circle.getTopLeft(),juce::Colour(0xff7f2d5b),circle.getBottomRight(),false);
    g.setGradientFill(grad);g.fillEllipse(circle);
    g.setColour(juce::Colour(0xffffc0ad));g.drawEllipse(circle.reduced(4.f),3.f);
    g.setColour(juce::Colour(0xffffefe0));
    g.setFont(juce::Font(juce::FontOptions(25.f,juce::Font::bold)));
    g.drawText("MCBOY!",circle,juce::Justification::centred);
    g.setFont(juce::Font(juce::FontOptions(11.f)));
    g.drawText("RANDOMIZE",circle.removeFromBottom(33),juce::Justification::centred);
}
juce::Colour McBoyAudioProcessorEditor::bankColour(int bank){
    static const std::array<juce::uint32,10> colors={0xff6d8cbb,0xff8a7ab0,0xff75adac,0xffc68eb3,0xffbb8c76,
                                                     0xff88ae88,0xffc0a26d,0xff9b9abd,0xff98b4c3,0xffc47e88};
    return juce::Colour(colors[size_t(bank)]);
}
McBoyAudioProcessorEditor::McBoyAudioProcessorEditor(McBoyAudioProcessor& proc)
 : juce::AudioProcessorEditor(proc),processor(proc) {
    setWantsKeyboardFocus(true);
    for(int i=0;i<100;++i){
        auto k=std::make_unique<MiniKnob>(i,mcboy::implemented[size_t(i)]);
        k->slider.onLock=[this,i](){processor.toggleLock(size_t(i));knobs[size_t(i)]->locked=processor.isLocked(size_t(i));knobs[size_t(i)]->repaint();};
        k->locked=processor.isLocked(size_t(i));
        const auto id=juce::String::formatted("fx%03d",i+1);
        k->slider.setTitle(juce::String(mcboy::effectNames[size_t(i)].data()));
        addAndMakeVisible(*k);
        knobAttach[size_t(i)]=std::make_unique<SliderAttachment>(processor.state,id,k->slider);
        knobs[size_t(i)]=std::move(k);
    }
    shuffle.onClick=[this](){processor.randomize();updateDescriptions();};
    addAndMakeVisible(shuffle);
    for(auto* s:{&degree,&mix,&output}){
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,21);
        s->setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffffbb85));
        s->setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff343b48));
        addAndMakeVisible(*s);
    }
    degreeAttach=std::make_unique<SliderAttachment>(processor.state,"degree",degree);
    mixAttach=std::make_unique<SliderAttachment>(processor.state,"mix",mix);
    outputAttach=std::make_unique<SliderAttachment>(processor.state,"output",output);
    for(auto* t:{&labels,&guard,&bypass}){addAndMakeVisible(*t);t->setColour(juce::ToggleButton::textColourId,juce::Colour(0xfff0e8e3));}
    labels.setButtonText("LABELS");guard.setButtonText("GUARD");bypass.setButtonText("BYPASS");
    labelsAttach=std::make_unique<ButtonAttachment>(processor.state,"labels",labels);
    guardAttach=std::make_unique<ButtonAttachment>(processor.state,"guard",guard);
    bypassAttach=std::make_unique<ButtonAttachment>(processor.state,"bypass",bypass);
    for(auto* b:{&saveCompare,&swapCompare,&undo,&redo,&clear,&lockAll,&unlockAll,&copy,&paste}){
        addAndMakeVisible(*b);b->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff333a46));
        b->setColour(juce::TextButton::textColourOffId,juce::Colour(0xffefebe4));
    }
    saveCompare.onClick=[this]{processor.storeCompare();updateDescriptions();};
    swapCompare.onClick=[this]{processor.swapCompare();updateDescriptions();};
    undo.onClick=[this]{processor.undoShuffle();updateDescriptions();};
    redo.onClick=[this]{processor.redoShuffle();updateDescriptions();};
    clear.onClick=[this]{processor.resetNeutral();updateDescriptions();};
    lockAll.onClick=[this]{processor.lockAll(true);updateDescriptions();};
    unlockAll.onClick=[this]{processor.lockAll(false);updateDescriptions();};
    copy.onClick=[this]{copyState();};paste.onClick=[this]{pasteState();};
    addAndMakeVisible(preset);
    const auto factory=mcboy::factoryPresets();
    for(int i=0;i<int(factory.size());++i)preset.addItem(juce::String(factory[size_t(i)].name),i+1);
    preset.setTextWhenNothingSelected("FACTORY PRESETS");
    preset.onChange=[this]{const auto index=preset.getSelectedId()-1;if(index>=0)processor.applyFactoryPreset(index);updateDescriptions();};
    for(auto* label:{&degreeLabel,&mixLabel,&outputLabel,&info}){
        addAndMakeVisible(*label);label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId,juce::Colour(0xffe3dcda));
    }
    degreeLabel.setText("DEGREE",juce::dontSendNotification);
    mixLabel.setText("MIX",juce::dontSendNotification);
    outputLabel.setText("OUTPUT dB",juce::dontSendNotification);
    info.setFont(juce::Font(juce::FontOptions(11.f)));
    info.setText("Right/Option click = lock  •  A/B compare available",juce::dontSendNotification);
    setResizable(true,true);setResizeLimits(1130,750,1920,1200);setSize(1440,900);
    startTimerHz(5);updateDescriptions();
}
McBoyAudioProcessorEditor::~McBoyAudioProcessorEditor(){stopTimer();}
void McBoyAudioProcessorEditor::updateDescriptions(){
    const float degree=processor.state.getRawParameterValue("degree")->load();
    for(int i=0;i<100;++i){
        auto& k=*knobs[size_t(i)];k.locked=processor.isLocked(size_t(i));
        const float raw=processor.state.getRawParameterValue(juce::String::formatted("fx%03d",i+1))->load();
        const auto tooltip=juce::String::formatted("%03d %s | raw %.3f effective %.3f | %s%s | %s",i+1,
                  mcboy::effectNames[size_t(i)].data(),double(raw),double(raw*degree),
                  mcboy::implemented[size_t(i)]?"ACTIVE":"UNAVAILABLE",k.locked?" | LOCKED":"",
                  mcboy::effectDescriptions[size_t(i)].data());
        k.slider.setTooltip(tooltip);k.slider.setDescription(tooltip);k.repaint();
    }
    info.setText("Seed " + juce::String::toHexString((juce::int64)processor.getSeed()) + "  •  right-click knobs to lock",juce::dontSendNotification);
}
void McBoyAudioProcessorEditor::timerCallback(){
    const auto display=processor.state.getRawParameterValue("labels")->load()>=.5f;
    if(display!=lastLabels){lastLabels=display;for(auto& k:knobs)k->setLabels(display);}
    updateDescriptions();
}
void McBoyAudioProcessorEditor::paint(juce::Graphics& g){
    auto all=getLocalBounds();
    g.fillAll(juce::Colour(0xff111720));
    const auto frame=all.reduced(14);
    g.setColour(juce::Colour(0xff1d2531));g.fillRoundedRectangle(frame.toFloat(),17.f);
    auto title=juce::Rectangle<int>(26,13,getWidth()-52,60);
    g.setColour(juce::Colour(0xffff91ad));g.setFont(juce::Font(juce::FontOptions(30.f,juce::Font::bold)));
    g.drawText("McBOY",title.removeFromLeft(250),juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffb6bbc6));g.setFont(juce::Font(juce::FontOptions(14.f)));
    g.drawText("100 KNOBS  •  1 VERY BIG IDEA",title,juce::Justification::centredRight);
    const int outer=24,centerWidth=std::max(234,getWidth()/5),half=(getWidth()-outer*2-centerWidth-30)/2;
    const int banksTop=85,banksBottom=getHeight()-50;
    const float bankHeight=float(banksBottom-banksTop)/5.f;
    for(int bank=0;bank<10;++bank){
        const int side=bank<5?0:1,row=bank%5;
        const int x=side==0?outer:getWidth()-outer-half;
        const int y=int(float(banksTop)+bankHeight*row);
        const auto rect=juce::Rectangle<float>(float(x),float(y),float(half),bankHeight-7.f);
        g.setColour(bankColour(bank).withAlpha(.17f));g.fillRoundedRectangle(rect,8.f);
        g.setColour(bankColour(bank).withAlpha(.62f));g.drawRoundedRectangle(rect,8.f,1.f);
        g.setColour(bankColour(bank).brighter(.44f));g.setFont(juce::Font(juce::FontOptions(12.f,juce::Font::bold)));
        g.drawText(juce::String::formatted("%c",'A'+bank)+"  "+juce::String(mcboy::bankNames[size_t(bank)].data()),
                   rect.toNearestInt().removeFromTop(20).reduced(11,0),juce::Justification::centredLeft);
    }
    g.setColour(juce::Colour(0xff4c5160));
    g.drawLine(float(getWidth()/2),96.f,float(getWidth()/2),135.f,1.f);
    g.setColour(juce::Colour(0xffefb7c4));
    g.setFont(juce::Font(juce::FontOptions(13.f)));
    g.drawText("PUSH THE BIG BUTTON",juce::Rectangle<int>(getWidth()/2-125,102,250,25),juce::Justification::centred);
}
void McBoyAudioProcessorEditor::resized(){
    const int outer=24,centerWidth=std::max(234,getWidth()/5),half=(getWidth()-outer*2-centerWidth-30)/2;
    saveCompare.setBounds(getWidth()/2-106,29,102,30);
    swapCompare.setBounds(getWidth()/2+4,29,102,30);
    const int bankTop=85,bankBottom=getHeight()-50;
    const float bankHeight=float(bankBottom-bankTop)/5.f;
    for(int bank=0;bank<10;++bank){
        const int side=bank<5?0:1,row=bank%5;
        const int x=side==0?outer:getWidth()-outer-half;
        const int y=int(float(bankTop)+bankHeight*row);
        const int usableHeight=int(bankHeight)-26;
        const int columnWidth=(half-12)/5;
        for(int sub=0;sub<10;++sub){
            const int fx=bank*10+sub,c=sub%5,r=sub/5;
            const int xx=x+6+c*columnWidth,yy=y+21+r*(usableHeight/2);
            knobs[size_t(fx)]->setBounds(xx,yy,columnWidth,usableHeight/2);
        }
    }
    const int middleX=outer+half+15,middleW=centerWidth;
    const float sy=float(getHeight()-110)/790.f;
    const auto Y=[&](int base){return 90+int(float(base-90)*sy);};
    const auto H=[&](int size){return std::max(18,int(size*sy));};
    shuffle.setBounds(middleX+middleW/2-107,Y(142),214,H(214));
    degreeLabel.setBounds(middleX+middleW/2-90,Y(356),180,H(23));
    degree.setBounds(middleX+middleW/2-81,Y(378),162,H(134));
    mixLabel.setBounds(middleX+10,Y(511),100,H(19));
    outputLabel.setBounds(middleX+middleW-115,Y(511),105,H(19));
    mix.setBounds(middleX+10,Y(529),100,H(95));
    output.setBounds(middleX+middleW-110,Y(529),100,H(95));
    const int left=middleX+10,right=middleX+middleW/2+5;
    labels.setBounds(left,Y(631),middleW/2-10,H(25));
    guard.setBounds(right,Y(631),middleW/2-10,H(25));
    bypass.setBounds(left,Y(655),middleW/2-10,H(25));
    preset.setBounds(middleX+12,Y(690),middleW-24,H(27));
    undo.setBounds(middleX+10,Y(728),(middleW-30)/2,H(29));
    redo.setBounds(right,Y(728),(middleW-30)/2,H(29));
    clear.setBounds(middleX+10,Y(765),middleW-20,H(27));
    lockAll.setBounds(middleX+10,Y(800),(middleW-30)/2,H(26));
    unlockAll.setBounds(right,Y(800),(middleW-30)/2,H(26));
    copy.setBounds(middleX+10,Y(834),(middleW-30)/2,H(26));
    paste.setBounds(right,Y(834),(middleW-30)/2,H(26));
    info.setBounds(outer,getHeight()-40,getWidth()-outer*2,24);
}
bool McBoyAudioProcessorEditor::keyPressed(const juce::KeyPress& key){
    if(key.getTextCharacter()=='r'||key.getTextCharacter()=='R'||key.getTextCharacter()==' '){processor.randomize();return true;}
    return false;
}
void McBoyAudioProcessorEditor::copyState(){
    juce::MemoryBlock bytes;processor.getStateInformation(bytes);
    juce::SystemClipboard::copyTextToClipboard("MCBOY1:"+bytes.toBase64Encoding());
}
void McBoyAudioProcessorEditor::pasteState(){
    const auto copied=juce::SystemClipboard::getTextFromClipboard();
    if(!copied.startsWith("MCBOY1:"))return;
    juce::MemoryBlock bytes;
    if(bytes.fromBase64Encoding(copied.substring(7)))processor.setStateInformation(bytes.getData(),int(bytes.getSize()));
    updateDescriptions();
}
juce::AudioProcessorEditor* McBoyAudioProcessor::createEditor(){return new McBoyAudioProcessorEditor(*this);}
