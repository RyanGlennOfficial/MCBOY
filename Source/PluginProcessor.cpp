#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

juce::AudioProcessorValueTreeState::ParameterLayout McBoyAudioProcessor::createParameterLayout(){
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for(int i=0;i<100;++i){
        const auto id=juce::String::formatted("fx%03d",i+1);
        const auto name=juce::String(mcboy::effectNames[size_t(i)].data());
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id,1}, name,juce::NormalisableRange<float>{0.f,1.f,.0001f},0.f));
    }
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"degree",1},"Degree",juce::NormalisableRange<float>{0.f,1.f,.0001f},.72f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix",1},"Master Mix",juce::NormalisableRange<float>{0.f,1.f,.0001f},1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"output",1},"Output (dB)",juce::NormalisableRange<float>{-18.f,6.f,.01f},0.f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"guard",1},"Digital Peak Guard",true));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"labels",1},"Show Labels",false));
    return layout;
}

McBoyAudioProcessor::McBoyAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                                    .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
   state(*this,nullptr,"MCBOY_STATE",createParameterLayout()) {
    for(size_t i=0;i<100;++i)rawPointers[i]=state.getRawParameterValue(juce::String::formatted("fx%03d",int(i+1)));
    degreePtr=state.getRawParameterValue("degree");mixPtr=state.getRawParameterValue("mix");
    outputPtr=state.getRawParameterValue("output");guardPtr=state.getRawParameterValue("guard");
    bypassPtr=state.getRawParameterValue("bypass");
    for(auto& l:locks)l.store(false,std::memory_order_relaxed);
    undoHistory.reserve(maxHistory);redoHistory.reserve(maxHistory);
}
void McBoyAudioProcessor::prepareToPlay(double sampleRate,int){engine.prepare(sampleRate);setLatencySamples(0);}
void McBoyAudioProcessor::releaseResources(){}
bool McBoyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto input=layouts.getMainInputChannelSet(),output=layouts.getMainOutputChannelSet();
    return (input==juce::AudioChannelSet::mono() || input==juce::AudioChannelSet::stereo()) && input==output;
}
void McBoyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
    juce::ScopedNoDenormals noDenormals;
    mcboy::CoreControls p;
    for(size_t i=0;i<100;++i)p.raw[i]=rawPointers[i]->load(std::memory_order_relaxed);
    p.degree=degreePtr->load(std::memory_order_relaxed);
    p.mix=mixPtr->load(std::memory_order_relaxed);
    p.outputDb=outputPtr->load(std::memory_order_relaxed);
    p.guard=guardPtr->load(std::memory_order_relaxed)>=.5f;
    p.bypass=bypassPtr->load(std::memory_order_relaxed)>=.5f;
    p.seed=patchSeed.load(std::memory_order_relaxed);
    engine.setControls(p);
    double bpm=120.0;
    if(auto* head=getPlayHead())if(auto pos=head->getPosition())if(auto tempo=pos->getBpm())bpm=*tempo;
    auto* left=buffer.getWritePointer(0);
    auto* right=buffer.getNumChannels()>1 ? buffer.getWritePointer(1) : nullptr;
    engine.process(left,right,buffer.getNumSamples(),bpm);
}

void McBoyAudioProcessor::setParam(const juce::String& id,float raw){
    if(auto* p=dynamic_cast<juce::RangedAudioParameter*>(state.getParameter(id))){
        p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(raw));p->endChangeGesture();
    }
}
McBoyAudioProcessor::Snapshot McBoyAudioProcessor::capture() const {
    Snapshot s;
    for(size_t i=0;i<100;++i){s.raw[i]=rawPointers[i]->load();s.locked[i]=isLocked(i);}
    s.degree=degreePtr->load();s.mix=mixPtr->load();s.output=outputPtr->load();
    s.guard=guardPtr->load()>=.5f;s.bypass=bypassPtr->load()>=.5f;
    s.labels=state.getRawParameterValue("labels")->load()>=.5f;
    s.seed=patchSeed.load();return s;
}
void McBoyAudioProcessor::applySnapshot(const Snapshot& s){
    for(size_t i=0;i<100;++i){
        locks[i].store(s.locked[i]);setParam(juce::String::formatted("fx%03d",int(i+1)),s.raw[i]);
    }
    setParam("degree",s.degree);setParam("mix",s.mix);setParam("output",s.output);
    setParam("guard",s.guard?1.f:0.f);setParam("bypass",s.bypass?1.f:0.f);
    setParam("labels",s.labels?1.f:0.f);patchSeed.store(s.seed);
}
void McBoyAudioProcessor::pushUndo(){
    if(undoHistory.size()>=maxHistory)undoHistory.erase(undoHistory.begin());
    undoHistory.push_back(capture());redoHistory.clear();
}
void McBoyAudioProcessor::randomize(){
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    pushUndo();const auto before=capture();
    const auto newSeed=mcboy::nextPatchSeed(before.seed);
    const auto newRaw=mcboy::generatePatch(newSeed,before.degree,before.raw,before.locked,mcboy::implemented);
    patchSeed.store(newSeed);
    for(size_t i=0;i<100;++i) if(!before.locked[i] && mcboy::implemented[i])
        setParam(juce::String::formatted("fx%03d",int(i+1)),newRaw[i]);
}
void McBoyAudioProcessor::undoShuffle(){
    if(undoHistory.empty())return;
    if(redoHistory.size()>=maxHistory)redoHistory.erase(redoHistory.begin());
    redoHistory.push_back(capture());auto s=undoHistory.back();undoHistory.pop_back();applySnapshot(s);
}
void McBoyAudioProcessor::redoShuffle(){
    if(redoHistory.empty())return;
    if(undoHistory.size()>=maxHistory)undoHistory.erase(undoHistory.begin());
    undoHistory.push_back(capture());auto s=redoHistory.back();redoHistory.pop_back();applySnapshot(s);
}
void McBoyAudioProcessor::resetNeutral(){
    pushUndo();
    applySnapshot(Snapshot{});
}
void McBoyAudioProcessor::storeCompare(){
    compareSnapshot=capture();
}
void McBoyAudioProcessor::swapCompare(){
    pushUndo();
    const auto current=capture();
    applySnapshot(compareSnapshot);
    compareSnapshot=current;
}
void McBoyAudioProcessor::applyFactoryPreset(int index){
    const auto presets=mcboy::factoryPresets();
    if(index<0 || index>=int(presets.size()))return;
    pushUndo();Snapshot s;const auto& p=presets[size_t(index)];
    s.raw=p.raw;s.locked=p.locked;s.degree=p.degree;s.mix=p.mix;
    s.output=p.outputDb;s.guard=p.guard;s.bypass=p.bypass;s.labels=p.labels;s.seed=p.seed;
    applySnapshot(s);
}
void McBoyAudioProcessor::lockAll(bool v) noexcept {for(auto& l:locks)l.store(v);}

juce::ValueTree McBoyAudioProcessor::snapshotToTree(const Snapshot& s){
    juce::ValueTree v("SNAPSHOT");
    for(size_t i=0;i<100;++i)v.setProperty("fx"+juce::String::formatted("%03d",int(i+1)),s.raw[i],nullptr);
    juce::String bits;for(bool b:s.locked)bits+=(b?"1":"0");
    v.setProperty("locks",bits,nullptr);
    v.setProperty("degree",s.degree,nullptr);v.setProperty("mix",s.mix,nullptr);v.setProperty("output",s.output,nullptr);
    v.setProperty("guard",s.guard,nullptr);v.setProperty("bypass",s.bypass,nullptr);v.setProperty("labels",s.labels,nullptr);
    v.setProperty("seed",juce::String::toHexString((juce::int64)s.seed),nullptr);
    return v;
}
McBoyAudioProcessor::Snapshot McBoyAudioProcessor::snapshotFromTree(const juce::ValueTree& v){
    Snapshot s;
    for(size_t i=0;i<100;++i)s.raw[i]=juce::jlimit(0.f,1.f,float(v.getProperty("fx"+juce::String::formatted("%03d",int(i+1)),0.f)));
    const auto bits=v.getProperty("locks").toString();
    for(size_t i=0;i<100;++i)s.locked[i]=int(i)<bits.length() && bits[int(i)]=='1';
    s.degree=juce::jlimit(0.f,1.f,float(v.getProperty("degree",.3f)));
    s.mix=juce::jlimit(0.f,1.f,float(v.getProperty("mix",.75f)));
    s.output=juce::jlimit(-18.f,6.f,float(v.getProperty("output",0.f)));
    s.guard=bool(v.getProperty("guard",true));s.bypass=bool(v.getProperty("bypass",false));
    s.labels=bool(v.getProperty("labels",false));
    const auto txt=v.getProperty("seed").toString();
    s.seed=txt.isNotEmpty()?uint64_t(txt.getHexValue64()):s.seed;
    return s;
}
void McBoyAudioProcessor::getStateInformation(juce::MemoryBlock& output){
    auto tree=state.copyState();
    tree.setProperty("schema",1,nullptr);
    tree.addChild(snapshotToTree(capture()),-1,nullptr);
    auto undo=juce::ValueTree("UNDO");for(const auto& h:undoHistory)undo.addChild(snapshotToTree(h),-1,nullptr);
    auto redo=juce::ValueTree("REDO");for(const auto& h:redoHistory)redo.addChild(snapshotToTree(h),-1,nullptr);
    tree.addChild(undo,-1,nullptr);tree.addChild(redo,-1,nullptr);
    auto compare=juce::ValueTree("COMPARE");
    compare.addChild(snapshotToTree(compareSnapshot),-1,nullptr);
    tree.addChild(compare,-1,nullptr);
    if(auto xml=tree.createXml())copyXmlToBinary(*xml,output);
}
void McBoyAudioProcessor::setStateInformation(const void* data,int size){
    if(auto xml=getXmlFromBinary(data,size)){
        auto tree=juce::ValueTree::fromXml(*xml);
        if(!tree.isValid() || !tree.hasType("MCBOY_STATE"))return;
        auto current=tree.getChildWithName("SNAPSHOT");
        auto undo=tree.getChildWithName("UNDO"),redo=tree.getChildWithName("REDO");
        auto compare=tree.getChildWithName("COMPARE");
        compareSnapshot=(compare.isValid() && compare.getNumChildren()>0)
                     ? snapshotFromTree(compare.getChild(0)) : Snapshot{};
        undoHistory.clear();redoHistory.clear();
        if(undo.isValid())for(int i=std::max(0,undo.getNumChildren()-int(maxHistory));i<undo.getNumChildren();++i)undoHistory.push_back(snapshotFromTree(undo.getChild(i)));
        if(redo.isValid())for(int i=std::max(0,redo.getNumChildren()-int(maxHistory));i<redo.getNumChildren();++i)redoHistory.push_back(snapshotFromTree(redo.getChild(i)));
        tree.removeChild(current,nullptr);tree.removeChild(undo,nullptr);tree.removeChild(redo,nullptr);tree.removeChild(compare,nullptr);
        state.replaceState(tree);
        if(current.isValid()){
            const auto s=snapshotFromTree(current);
            patchSeed.store(s.seed);
            for(size_t i=0;i<100;++i)locks[i].store(s.locked[i]);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new McBoyAudioProcessor();}
