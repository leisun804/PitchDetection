/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PitchDetectionAudioProcessor::PitchDetectionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    addParameter (windowSizePower2 = new juce::AudioParameterInt("windowSizePower2", "windowSizePower2", 11, 13, 11) );
    addParameter (hoppingSize = new juce::AudioParameterInt("hoppingSize", "hoppingSize", 1, 1024, 1024) );
    addParameter (leastNoteLength = new juce::AudioParameterInt("least note length", "least note length", 500, 2500, 2000) );
    addParameter(correlationThres = new juce::AudioParameterFloat("correlation threshold", "correlation threshold", juce::NormalisableRange<float>(0.2f, 0.8f, 0.01f, 1.f), 0.6f));
    addParameter(noiseThres = new juce::AudioParameterFloat("noise threshold", "noise threshold", juce::NormalisableRange<float>(0.f, 0.1f, 0.005f, 1.f), 0.05f));
    juce::StringArray str;
    str.add("Origin");
    str.add("SIMD");
    str.add("FFT");
    addParameter (function = new juce::AudioParameterChoice("function", "function", str, 2));
}

PitchDetectionAudioProcessor::~PitchDetectionAudioProcessor()
{
}

//==============================================================================
const juce::String PitchDetectionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PitchDetectionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PitchDetectionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PitchDetectionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PitchDetectionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PitchDetectionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PitchDetectionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PitchDetectionAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PitchDetectionAudioProcessor::getProgramName (int index)
{
    return {};
}

void PitchDetectionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PitchDetectionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    autoCorrelation.prepare(sampleRate, samplesPerBlock);
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void PitchDetectionAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PitchDetectionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PitchDetectionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    buffer.getWritePointer(1);
    juce::dsp::AudioBlock<float> block(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);
    
    updateCoef();
    autoCorrelation.process(context, midiMessages);
    
}

void PitchDetectionAudioProcessor::updateCoef()
{
    autoCorrelation.LNL = leastNoteLength -> get();
    autoCorrelation.function = function -> getIndex();
    autoCorrelation.windowSizePower2 = windowSizePower2 -> get();
    autoCorrelation.hoppingSize = hoppingSize -> get();
    autoCorrelation.correlationThres = correlationThres -> get();
    autoCorrelation.noiseThres = noiseThres -> get();
}

//==============================================================================
bool PitchDetectionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PitchDetectionAudioProcessor::createEditor()
{
    //return new PitchDetectionAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void PitchDetectionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PitchDetectionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchDetectionAudioProcessor();
}

