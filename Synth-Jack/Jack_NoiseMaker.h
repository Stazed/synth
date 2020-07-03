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
#include "Synth.h"


using namespace std;

const unsigned char  EVENT_STATUS_BIT       = 0x80;
const unsigned char  EVENT_NOTE_OFF         = 0x80;     // decimal 128
const unsigned char  EVENT_NOTE_ON          = 0x90;     // decimal 144
const unsigned char  EVENT_CONTROL_CHANGE   = 0xB0;     // decimal 176
const unsigned char  EVENT_PROGRAM_CHANGE   = 0xC0;
const unsigned char  EVENT_CLEAR_CHAN_MASK  = 0xF0;
const unsigned char  EVENT_CHANNEL          = 0x0F;

const int NOTE_ON   = 1;
const int NOTE_OFF  = 0;

class NoiseMaker
{
public:
    NoiseMaker(double sample_rate, uint32_t buffer_size)
    {
        m_nSampleRate = sample_rate;
        m_nBlockSamples =  buffer_size;
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
    virtual double UserProcess(int, double)
    {
        return 0.0;
    }
    
    void SetUserFunction(double(*func)(int, double))
    {
        m_userFunction = func;
    }
    
    void SetMidiAddNote(void(*func)(double, int, int, int, int, int))
    {
        m_MidiAddNote = func;
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
                nNewSample = UserProcess(1, m_dGlobalTime);
            else
                nNewSample = m_userFunction(1, m_dGlobalTime); // function MakeNoise()
            
            efxoutl[n] = nNewSample;
            efxoutr[n] = nNewSample;
            
            m_dGlobalTime = m_dGlobalTime + m_dTimeStep;    // running wall time
        }

        // return 0 if successful; otherwise jack_finish() will be called and
        // the client terminated immediately.
        return 0; // Continue
    }
    
    int ProcessMidi(jack_midi_event_t *midievent)
    {
        /* events */
        unsigned char channel = 0, note_key = 0, velocity = 0;
        
        if( ((*(midievent->buffer) & 0xf0)) == 0x90 )
        {
            /* note on */
            channel = 1 + (midievent->buffer[0] & EVENT_CHANNEL);
            note_key = midievent->buffer[1];
            velocity = midievent->buffer[2];
            
            // some hardware sends note-on with 0 velocity for note-off
            int nOn_Off = NOTE_ON;
            if(!velocity)
                nOn_Off = NOTE_OFF;

            m_MidiAddNote(m_dGlobalTime, note_key,nOn_Off, channel, velocity, synth::MIDI_NOTE);
        }

        else if( ((*(midievent->buffer)) & 0xf0) == 0x80 )
        {
            /* note off */
            channel = 1 + (midievent->buffer[0] & EVENT_CHANNEL);
            note_key = midievent->buffer[1];
            velocity = 0;
            
            m_MidiAddNote(m_dGlobalTime, note_key, NOTE_OFF, channel, velocity, synth::MIDI_NOTE);
        }
        
        // return 0 if successful; otherwise jack_finish() will be called and
        // the client terminated immediately.
        return 0; // Continue
    }
    
    void jackshutdown (void *)
    {
        printf ("Jack Shut Down, sorry.\n");
    };
    
private:
    
    double (*m_userFunction)(int, double);
    void (*m_MidiAddNote)(double, int, int, int, int, int);
    
    jack_nframes_t m_nSampleRate;
    unsigned int m_nChannels;
    unsigned int m_nBlockCount;
    jack_nframes_t m_nBlockSamples;
    unsigned int m_nBlockCurrent;
    
    atomic<double> m_dGlobalTime;
    double m_dTimeStep;
     
};


#endif /* JACK_NOISEMAKER_H */

