/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Synth.h
 * Author: sspresto
 *
 * Created on July 1, 2020, 12:38 PM
 */

#ifndef SYNTH_H
#define SYNTH_H

#include <math.h>

namespace synth
{
    ////////////////////////////////////
    // Utilities

    // Converts frequency (Hz) to angular velocity
    double w (double dHertz);
    
    // A basic note
    struct note
    {
        int id;     // Position in scale
        double on;  // Time note was activated
        double off; // Time note was deactivated
        bool active;
        int channel;
        
        note()
        {
            id = 0;
            on = 0.0;
            off = 0.0;
            active = false;
            channel = 0;
        }
    };
    
    ////////////////////////////////////
    // Multi-Function Oscillator
    const int OSC_SINE = 0;
    const int OSC_SQUARE = 1;
    const int OSC_TRIANGLE = 2;
    const int OSC_SAW_ANA = 3;
    const int OSC_SAW_DIG = 4;
    const int OSC_NOISE = 5;
    

    double osc(const double dTime, const double dHertz, const int nType = OSC_SINE,
            const double dLFOHertz = 5.0, const double dLFOAmplitude = 0.1, double dCustom = 50.0);

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

}   // namespace synth

#endif /* SYNTH_H */

