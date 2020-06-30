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


class NoiseMaker
{
public:
    NoiseMaker(double sample_rate, uint32_t buffer_size)
    {
        //printf("sample_rate = %lf: bufsize = %d\n", sample_rate, buffer_size);
        m_nSampleRate = sample_rate;
        m_nBlockSamples =  buffer_size;
        calc_note_frqs(sample_rate);
    }

    ~NoiseMaker()
    {
        Destroy();
    }
    
    bool Destroy()
    {
        return false;
    }
    
    void Stop()
    {
        m_bReady = false;
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
    
    void SetNote(float note)
    {
        m_note_on = note;
    }
    
    double GetTime()
    {
        return m_dGlobalTime;
    }
    
    int MainThread(float * efxoutl, float * efxoutr)
    {
        m_dGlobalTime = 0.0;
#if 1
        double dTimeStep = (1.0 / (double)m_nSampleRate);
        
        float nNewSample = 0.0;

        for (unsigned int n = 0; n < m_nBlockSamples; n++)
        {
            m_ramp_time += dTimeStep;
            m_ramp_time = (m_ramp_time > 1.0) ? m_ramp_time - 2.0 : m_ramp_time;
            
            // User Process
            if (m_userFunction == nullptr)
                nNewSample = m_note_on * UserProcess(m_ramp_time);
            else
                nNewSample = m_note_on * m_userFunction(m_ramp_time);
            
            efxoutl[n] = nNewSample;
            efxoutr[n] = nNewSample;
            
            m_dGlobalTime = m_dGlobalTime + dTimeStep;
        }
        
#endif // 0
            
#if 0
        jack_default_audio_sample_t calculate_note = 0.0;
        
        for (unsigned i = 0; i < m_nBlockSamples; i++)
        { 
            //printf("MainThread\n");
            m_ramp += m_note_frqs[m_note];
            m_ramp = (m_ramp > 1.0) ? m_ramp - 2.0 : m_ramp;

            calculate_note = m_note_on*sin(2*M_PI*m_ramp);
            
            efxoutl[i] = calculate_note;
            efxoutr[i] = calculate_note;
        }
#endif // 0
    }
    
    int ProcessMidi(jack_midi_event_t *midievent)
    {
        //printf("ProcessMidi\n");

        if( ((*(midievent->buffer) & 0xf0)) == 0x90 )
        {
            /* note on */
            m_note = *(midievent->buffer + 1);
            *m_dFrequency = m_note_hrz[m_note];
            m_note_on = 1.0;
            //printf("Note ON = %f\n", *m_dFrequency);
        }

        else if( ((*(midievent->buffer)) & 0xf0) == 0x80 )
        {
            /* note off */
            m_note = *(midievent->buffer + 1);
            m_note_on = 0.0;
            //printf("Note OFF\n");
        }
    }
    
    void jackshutdown (void *arg)
    {
        printf ("Jack Shut Down, sorry.\n");
    };
    
private:
    
    double (*m_userFunction)(double);
    double *m_dFrequency;
    
    uint32_t m_nSampleRate;
    unsigned int m_nChannels;
    unsigned int m_nBlockCount;
    uint32_t m_nBlockSamples;
    unsigned int m_nBlockCurrent;
    
    atomic<bool> m_bReady;
    
    atomic<double> m_dGlobalTime;
    
    /* MIDI processing */
    unsigned char m_note = 0;
    jack_default_audio_sample_t m_note_on = 0.0;
    
    
    jack_default_audio_sample_t m_ramp=0.0;
    jack_default_audio_sample_t m_note_frqs[128];
    
    jack_default_audio_sample_t m_ramp_time=0.0;
    
    jack_default_audio_sample_t m_note_hrz[128];
    
    void calc_note_frqs(jack_default_audio_sample_t srate)
    {
        int i;

        for(i=0; i<128; i++)
        {
            m_note_frqs[i] = (2.0 * 440.0 / 32.0) * pow(2, (((jack_default_audio_sample_t)i - 9.0) / 12.0)) / srate;
            
            m_note_hrz[i] = (440.0 / 32.0) * pow(2, (((jack_default_audio_sample_t)i - 9.0) / 12.0));
        }

        /* Debug */
    #if 0
        int note = 100;
        jack_default_audio_sample_t calculate_note = 0.0;
        jack_default_audio_sample_t a_ramp=0.0;

        for (int ii = 0; ii < 20; ii++)
        {
            a_ramp += note_frqs[note];
            a_ramp = (a_ramp > 1.0) ? a_ramp - 2.0 : a_ramp;

            calculate_note = 1*sin(2*M_PI*a_ramp);

            printf("out value = %f: note_freqs = %f: a_ramp = %f\n", calculate_note, note_frqs[note], a_ramp);
        }
    #endif // 0
    }
    
};


#endif /* JACK_NOISEMAKER_H */

