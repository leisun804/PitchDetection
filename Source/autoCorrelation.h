/*
  ==============================================================================

    autoCorrelation.h
    Created: 24 Apr 2023 3:46:03pm
    Author:  孫新磊

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class AutoCorrelation
{
public:
    AutoCorrelation()
    {
        noteOn = false;
        noteLife = 0;
        noteNum = 1;
    }
    void process(const juce::dsp::ProcessContextReplacing<float> &context ,juce::MidiBuffer& midiMessages)
    {
        auto &&inBlock = context.getInputBlock();
        
        auto len = inBlock.getNumSamples();

        auto *src = inBlock.getChannelPointer(0);
        
        int T;
        float ACF_PREV, ACF;
        int pdState = 0;
        float thres;
        double freq = 440;
        
        for (int k = 0; k < len; ++k)
        {
            ACF_PREV = ACF;
            ACF = 0;
            
            for (int n = 0; n < len - k; ++n)
                ACF += src[n] * src[n + k];
            
            if (pdState == 2 && (ACF-ACF_PREV) <= 0) {
                T = k;
                pdState = 3;
            }
                            
            if (pdState == 1 && (ACF > thres) && (ACF-ACF_PREV) > 0) {
                pdState = 2;
            }
                            
            if (!k) {
                thres = ACF * 0.5;
                pdState = 1;
            }
            
        }
        
        if (thres > 1)
        {
            freq = sampleRate / T;
            int note = round(log(freq / 440.0) / log(2) * 12 + 69);
            if (note > 127 || note < 0)
                return;
            if (noteLife <= 0)
            {
                auto message = juce::MidiMessage::noteOn(1, note, (juce::uint8) 100);
                midiMessages.addEvent(message, 0);
                noteNum = note;
                noteLife = 3;
            }
            else if (note == noteNum)
                noteLife = 3;
        }
        
        if(noteLife > 0)
            noteLife--;
        if (noteLife == 0)
        {
            auto message = juce::MidiMessage::noteOff(1, noteNum);
            midiMessages.addEvent(message, sampleSize - 1);
        }
        
         
        /*
        if (thres > 1)
        {
            freq = sampleRate / T;
            int note = round(log(freq / 440.0) / log(2) * 12 + 69);
            if (note > 127 || note < 0)
                return;
            auto message = juce::MidiMessage::noteOn(1, note, (juce::uint8) 100);
            midiMessages.addEvent(message, 0);
        }
        */
        
        
        
            
        DBG(thres);
        
    }
    void prepare(double SampleRate, int SampleSize)
    {
        sampleRate = SampleRate;
        //ACF = new float(SampleSize);
        sampleSize = SampleSize;
    }
private:
    double sampleRate;
    int sampleSize;
    
    bool noteOn;
    int noteLife;
    int noteNum;
    
};
