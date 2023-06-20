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
    
    int LNL; //least note length
    bool SIMD;
    AutoCorrelation()
    {
        lastNote = -1;
        lastNotePos = 0;
        LNL = 3000;
    }
    int findNote(int windowSize){
        int T;
        float ACF_PREV, ACF;
        int pdState = 0;
        float thres;
        double freq = 440;
        
        
        for (int k = 0; k < windowSize; ++k)
        {
            ACF_PREV = ACF;
            ACF = 0;
            
            for (int n = 0; n < windowSize - k; ++n)
                ACF += windowSamples[n] * windowSamples[n + k];

            if (pdState == 2 && (ACF-ACF_PREV) <= 0) {
                T = k - 1;
                pdState = 3;
            }
                            
            if (pdState == 1 && (ACF > thres) && (ACF-ACF_PREV) > 0) {
                pdState = 2;
            }
                            
            if (!k) {
                thres = ACF * 0.6;
                pdState = 1;
            }
        }
        freq = sampleRate / T;
        int note = round(log(freq / 440.0) / log(2) * 12 + 69);
        
        if (note > 127 || note < 0 || thres <= 0.05 )
            return -1;
        return note;
    }
    
    int SIMDfindNote(int windowSize){
        for(int k = 0; k < windowSize; k++)
            sums[k] = 0;
        for(int k = 0; k < windowSize; k++){
            juce::FloatVectorOperationsBase<float, int>::addWithMultiply(sums, &windowSamples[k], windowSamples[k], windowSize - k);
        }
        
        int T = 100;
        bool flag = false;
        float thres = 0.6 * sums[0];
        
        for (int k = 1; k < windowSize; ++k){
            if (flag && sums[k] <= sums[k-1]){
                T = k - 1;
                break;
            }
            if (sums[k] > sums[k-1] && sums[k] > thres){
                flag = true;
            }
        }
        
        double freq = sampleRate / T;
        int note = round(log(freq / 440.0) / log(2) * 12 + 69);
        if (note > 127 || note < 0 || thres <= 0.05 )
            return -1;
        return note;
    }
    
    void buildingMidiMessage(int note, int notePos, juce::MidiBuffer& midiMessages){
        if (note == -1)
            return;
        
        if (lastNote == -1){
            auto message = juce::MidiMessage::noteOn(1, note, (juce::uint8) 100);
            midiMessages.addEvent(message, notePos);
            lastNote = note;
            lastNotePos = notePos;
            return;
        }
        
        if (note == lastNote){
            lastNotePos = notePos;
            return;
        }
        
    }
    
    void process(int windowSize, int hoppingSize, const juce::dsp::ProcessContextReplacing<float> &context ,juce::MidiBuffer& midiMessages)
    {
        auto &&inBlock = context.getInputBlock();
        auto *src = inBlock.getChannelPointer(0);
        
        lastNotePos -= sampleSize;
        
        while(true){
            //noteOff the note which sustained more than LNL time
            if ( (lastNote != -1) && (curSample-1  - lastNotePos > LNL) ){
                auto message = juce::MidiMessage::allNotesOff(1);
                midiMessages.addEvent(message, curSample - 1);
                lastNote = -1;
            }
            
            //window samples filled done ---> find note
            if(windowNextFill >= windowSize){
                int note = -1;
                if (SIMD)
                    note = SIMDfindNote(windowSize);
                else
                    note = findNote(windowSize);
                buildingMidiMessage(note, curSample - 1, midiMessages);
                
                //windowSamples left shift
                for (int i = 0; i < windowSize - hoppingSize; i++)
                    windowSamples[i] = windowSamples[i + hoppingSize];
                windowNextFill -= hoppingSize;
                continue;
            }
            //window sample need next sample block to continue filling up
            if(curSample >= sampleSize){
                curSample = 0;
                break;
            }
            windowSamples[windowNextFill] = src[curSample];
            curSample++;
            windowNextFill++;
        }
        
    }
    void prepare(double SampleRate, int SampleSize)
    {
        sampleRate = SampleRate;
        sampleSize = SampleSize;
        windowNextFill = 0;
        curSample = 0;
    }
private:
    double sampleRate;
    int sampleSize;
    
    float windowSamples[4096] = {0};
    int windowNextFill;
    int curSample;
    
    float sums[4096] = { 0 };
    
    int lastNote;   //lastNote == -1 means there's no note sustaining
    int lastNotePos;
};

