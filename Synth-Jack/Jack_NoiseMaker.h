/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Jack_NoiseMaker.h
 * Author: sspresto
 *
 * Created on June 27, 2020, 10:58 AM
 */

#ifndef JACK_NOISEMAKER_H
#define JACK_NOISEMAKER_H

#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <atomic>

#include <jack/midiport.h>


using namespace std;

struct sEnvelopeADSR
{
    double dAttackTime;
    double dDecayTime;
    double dReleaseTime;
    
    double dSustainAmplitude;
    double dStartAmplitude;
    
    double dTriggerOnTime;
    double dTriggerOffTime;
    
    bool bNoteOn;
    
    sEnvelopeADSR()
    {
        dAttackTime = 0.100;
        dDecayTime = 0.01;
        dStartAmplitude = 1.0;
        dSustainAmplitude = 0.8;
        dReleaseTime = 0.200;
        dTriggerOnTime = 0.0;
        dTriggerOffTime = 0.0;
        bNoteOn = false;
    }
    
    double GetAmplitude(double dTime) // dTime = wall time
    {
        double dAmplitude = 0.0;
        
        double dLifeTime = dTime - dTriggerOnTime;
        
        if (bNoteOn)
        {
            // ADS
            
            // Attack
            if(dLifeTime <= dAttackTime)
                dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;
            
            // Decay
            if(dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
                dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;
            
            // Sustain
            if(dLifeTime > (dAttackTime + dDecayTime))
            {
                dAmplitude = dSustainAmplitude;
            }
        }
        else
        {
            // Release
            dAmplitude = ((dTime - dTriggerOffTime) / dReleaseTime) * (0.0 - dSustainAmplitude) + dSustainAmplitude;
        }
        
        if (dAmplitude <= 0.0001)
        {
            dAmplitude = 0.0;
        }
        
        return dAmplitude;
    }
    
    void NoteOn(double dTimeOn)
    {
        dTriggerOnTime = dTimeOn;
        bNoteOn = true;
    }
    
    void NoteOff(double dTimeOff)
    {
        dTriggerOffTime = dTimeOff;
        bNoteOn = false;
    }
};


class NoiseMaker
{
public:
    NoiseMaker(double sample_rate, uint32_t buffer_size)
    {
        m_nSampleRate = sample_rate;
        m_nBlockSamples =  buffer_size;
        calc_note_frqs();
        SetTimeStep(sample_rate);
        m_dGlobalTime = 0.0;
    }

    ~NoiseMaker()
    {
        Destroy();
    }
    
    bool Destroy()
    {
        return false;
    }
    
    // Override to process current sample
    virtual double UserProcess(double dTime)
    {
        return 0.0;
    }
    
    void SetUserFunction(double(*func)(double))
    {
        m_userFunction = func;
    }
    
    void SetFreqVariable(double & freq)
    {
        m_dFrequency = &freq;
    }
    
    void SetEnvelope(sEnvelopeADSR & env)
    {
        m_structEnvelope = &env;
    }

    double GetTime()
    {
        return m_dGlobalTime;
    }
    
    void SetTimeStep(jack_nframes_t nSampleRate)
    {
        m_nSampleRate = nSampleRate;
        
        // Wall Time step per second
        m_dTimeStep = (1.0 / (double)m_nSampleRate);
    }
    
    int MainThread(float * efxoutl, float * efxoutr)
    {
        float nNewSample = 0.0;

        for (unsigned int n = 0; n < m_nBlockSamples; n++)
        {
            // User Process
            if (m_userFunction == nullptr)
                nNewSample = UserProcess(m_dGlobalTime);
            else
                nNewSample = m_userFunction(m_dGlobalTime); // function MakeNoise()
            
            efxoutl[n] = nNewSample;
            efxoutr[n] = nNewSample;
            
            m_dGlobalTime = m_dGlobalTime + m_dTimeStep;    // running wall time
        }
          
        return 0; // not used
    }
    
    int ProcessMidi(jack_midi_event_t *midievent)
    {
        if( ((*(midievent->buffer) & 0xf0)) == 0x90 )
        {
            /* note on */
            m_note = *(midievent->buffer + 1);          // get the MIDI note
            *m_dFrequency = m_note_hrz[m_note];         // set the synth frequency
            m_structEnvelope->NoteOn(m_dGlobalTime);    // set the time of note on
            //printf("Note ON = %f\n", *m_dFrequency);
        }

        else if( ((*(midievent->buffer)) & 0xf0) == 0x80 )
        {
            /* note off */
            m_note = *(midievent->buffer + 1);          // get MIDI note off - not used yet
            m_structEnvelope->NoteOff(m_dGlobalTime);   // set note off time
            //printf("Note OFF\n");
        }
        return 0; // not used
    }
    
    void jackshutdown (void *arg)
    {
        printf ("Jack Shut Down, sorry.\n");
    };
    
private:
    
    double (*m_userFunction)(double);
    double *m_dFrequency;
    sEnvelopeADSR *m_structEnvelope;
    
    jack_nframes_t m_nSampleRate;
    unsigned int m_nChannels;
    unsigned int m_nBlockCount;
    jack_nframes_t m_nBlockSamples;
    unsigned int m_nBlockCurrent;
    
    atomic<double> m_dGlobalTime;
    double m_dTimeStep;
    
    /* MIDI processing */
    unsigned char m_note = 0;
     
    jack_default_audio_sample_t m_note_hrz[128];    // note array
    
    /* MIDI to frequency (Hz) array */
    void calc_note_frqs()
    {
        for(int i=0; i<128; i++)
        {
            // this converts a MIDI note to audio frequency (Hz)
            m_note_hrz[i] = (440.0 / 32.0) * pow(2, (((jack_default_audio_sample_t)i - 9.0) / 12.0));
        }
    }
    
};


#endif /* JACK_NOISEMAKER_H */

