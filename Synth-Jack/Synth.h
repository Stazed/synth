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
#include <string>

namespace synth
{
    ////////////////////////////////////
    // Utilities

    // Converts frequency (Hz) to angular velocity
    double w (double dHertz);
    
    // MIDI velocity conversion (1.0 / 127)
    const double MIDI_VELOCITY_RATIO = 0.007874;
    
    // Drum note default
    const int DRUM_NOTE = 64;
    
    
    /* MIDI to frequency (Hz) array */
    void calc_note_frqs();
    
    // A basic note
    struct note
    {
        int id;         // Position in scale
        double on;      // Time note was activated
        double off;     // Time note was deactivated
        double volume;  // MIDI velocity
        bool active;
        int channel;    // Instrument type
        int scale;      // MIDI vs PC keyboard
        
        
        note()
        {
            id = 0;
            on = 0.0;
            off = 0.0;
            volume = 127.0; // MIDI range 0 to 127
            active = false;
            channel = 0;
            scale = 1;  // MIDI_NOTE
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
            const double dLFOHertz = 0.0, const double dLFOAmplitude = 0.1, double dCustom = 50.0);

    ////////////////////////////////////
    // scale to frequency conversion

    const int SCALE_DEFAULT = 0;
    const int MIDI_NOTE = 1;

    /* For PC Keyboard set to SCALE_DEFAULT, Jack MIDI set to MIDI_NOTE */
    double scale(const int nNoteID, const int nScaleID = MIDI_NOTE);


    ////////////////////////////////////
    // envelopes
    
    struct envelope
    {
        virtual double amplitude(const double dTime, const double dTimeOn, const double dTimeOff) = 0;
    };

    struct envelope_adsr : public envelope
    {
        double dAttackTime;
        double dDecayTime;
        double dSustainAmplitude;
        double dReleaseTime;
        double dStartAmplitude;

        envelope_adsr()
        {
            dAttackTime = 0.1;
            dDecayTime = 0.1;
            dSustainAmplitude = 1.0;
            dReleaseTime = 0.2;
            dStartAmplitude = 1.0;
        }

        virtual double amplitude(const double dTime, const double dTimeOn, const double dTimeOff) // dTime = wall time
        {
            double dAmplitude = 0.0;

            double dReleaseAmplitude = 0.0;

            if (dTimeOn > dTimeOff) // note is on
            {
                double dLifeTime = dTime - dTimeOn;

                // Attack
                if(dLifeTime <= dAttackTime)
                    dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

                // Decay
                if(dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
                    dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * 
                            (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

                // Sustain
                if(dLifeTime > (dAttackTime + dDecayTime))
                {
                    dAmplitude = dSustainAmplitude;
                }
            }
            else    // note is off
            {
                // Release
                
                double dLifeTime = dTimeOff - dTimeOn;
                
                if (dLifeTime <= dAttackTime)
                {
                    dReleaseAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;
                }
                
                if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
                {
                    dReleaseAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) *
                            (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;
                }
                
                if (dLifeTime > (dAttackTime + dDecayTime))
                    dReleaseAmplitude = dSustainAmplitude;
                    
                dAmplitude = ((dTime - dTimeOff) / dReleaseTime) * (0.0 - dReleaseAmplitude) + dReleaseAmplitude;
            }

            // amplitude should not be negative
            if (dAmplitude <= 0.0)
            {
                dAmplitude = 0.0;
            }

            return dAmplitude;
        }
    };
    
    double env(const double dTime, envelope &env, const double dTimeOn, const double dTimeOff);
    
    struct instrument_base
    {
        double dVolume;
        synth::envelope_adsr env;
        double dMaxLifeTime;
        std::string name;
        virtual double sound(const double dTime, synth::note n, bool &bNoteFinished) = 0;
    };
    
    struct instrument_bell : public instrument_base
    {
        instrument_bell()
        {
            env.dAttackTime = 0.01;
            env.dDecayTime = 1.0;
            env.dSustainAmplitude = 0.0;
            env.dReleaseTime = 1.0;
            dMaxLifeTime = 3.0;
            dVolume = 1.0;
            name = "Bell";
        }

        virtual double sound(const double dTime, synth::note n, bool &bNoteFinished)
        {
            dVolume = n.volume * MIDI_VELOCITY_RATIO;
            double dAmplitude = synth::env(dTime, env, n.on, n.off);
            if (dMaxLifeTime > 0.0 && dTime - n.on >= dMaxLifeTime)
                bNoteFinished = true;
            
            double dSound =
                + 1.00 * synth::osc(dTime - n.on, synth::scale(n.id + 12, n.scale), synth::OSC_SINE, 5.0, 0.001)
                + 0.50 * synth::osc(dTime - n.on, synth::scale(n.id + 24, n.scale))
                + 0.25 * synth::osc(dTime - n.on, synth::scale(n.id + 36, n.scale));
            
            return dAmplitude * dSound * dVolume;
        }  
    };
    
    struct instrument_bell8 : public instrument_base
    {
        instrument_bell8()
        {
            env.dAttackTime = 0.01;
            env.dDecayTime = 0.5;
            env.dSustainAmplitude = 0.8;
            env.dReleaseTime = 1.0;
            dMaxLifeTime = 3.0;
            dVolume = 1.0;
            name = "8-Bit Bell";
            
        }

        virtual double sound(const double dTime, synth::note n, bool &bNoteFinished)
        {
            dVolume = n.volume * MIDI_VELOCITY_RATIO;
            double dAmplitude = synth::env(dTime, env, n.on, n.off);
            if (dMaxLifeTime > 0.0 && dTime - n.on >= dMaxLifeTime)
                bNoteFinished = true;

            double dSound =
                +1.00 * synth::osc(dTime - n.on, synth::scale(n.id, n.scale), synth::OSC_SQUARE, 5.0, 0.001)
                + 0.50 * synth::osc(dTime - n.on, synth::scale(n.id + 12, n.scale))
                + 0.25 * synth::osc(dTime - n.on, synth::scale(n.id + 24, n.scale));

            return dAmplitude * dSound * dVolume;
        }
    };

    struct instrument_harmonica : public instrument_base
    {
        instrument_harmonica()
        {
            env.dAttackTime = 0.05;
            env.dDecayTime = 1.0;
            env.dSustainAmplitude = 0.95;
            env.dReleaseTime = 0.1;
            dMaxLifeTime = -1.0;    // Not used
            dVolume = 1.0;
            name = "Harmonica";
        }

        virtual double sound(const double dTime, synth::note n, bool &bNoteFinished)
        {
            dVolume = n.volume * MIDI_VELOCITY_RATIO;
            double dAmplitude = synth::env(dTime, env, n.on, n.off);
            if (dAmplitude <= 0.0 && n.off >= n.on)
                bNoteFinished = true;

            double dSound =
                + 1.0  * synth::osc(dTime - n.on, synth::scale(n.id-12), synth::OSC_SAW_DIG, 5.0, 0.001, 20)
                + 1.00 * synth::osc(dTime - n.on, synth::scale(n.id, n.scale), synth::OSC_SQUARE, 5.0, 0.001)
                + 0.50 * synth::osc(dTime - n.on, synth::scale(n.id + 12, n.scale), synth::OSC_SQUARE)
                + 0.05  * synth::osc(dTime - n.on, synth::scale(n.id + 24, n.scale), synth::OSC_NOISE);

            return dAmplitude * dSound * dVolume;
        }
    };
    
    struct instrument_drumkick : public instrument_base
    {
        instrument_drumkick()
        {
            env.dAttackTime = 0.01;
            env.dDecayTime = 0.15;
            env.dSustainAmplitude = 0.0;
            env.dReleaseTime = 0.01;
            dMaxLifeTime = 1.5;
            dVolume = 1.0;
            name = "Drum Kick";
        }

        virtual double sound(const double dTime, synth::note n, bool &bNoteFinished)
        {
            double dAmplitude = synth::env(dTime, env, n.on, n.off);
            if(dMaxLifeTime > 0.0 && dTime - n.on >= dMaxLifeTime)
                bNoteFinished = true;

            double dSound =
                + 0.99 * synth::osc(dTime - n.on, synth::scale(DRUM_NOTE - 36, SCALE_DEFAULT), synth::OSC_SINE, 1.0, 1.0)
                + 0.01 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

            return dAmplitude * dSound * dVolume;
        }
    };

    struct instrument_drumsnare : public instrument_base
    {
        instrument_drumsnare()
        {
            env.dAttackTime = 0.01;
            env.dDecayTime = 0.2;
            env.dSustainAmplitude = 0.0;
            env.dReleaseTime = 0.01;
            dMaxLifeTime = 1.0;
            name = "Drum Snare";
            dVolume = 1.0;
        }

        virtual double sound(const double dTime, synth::note n, bool &bNoteFinished)
        {
            double dAmplitude = synth::env(dTime, env, n.on, n.off);
            if (dMaxLifeTime > 0.0 && dTime - n.on >= dMaxLifeTime)
                bNoteFinished = true;

            double dSound =
                + 0.5 * synth::osc(dTime - n.on, synth::scale(DRUM_NOTE - 24, SCALE_DEFAULT), synth::OSC_SINE, 0.5, 1.0)
                + 0.5 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

                return dAmplitude * dSound * dVolume;
        }
    };


    struct instrument_drumhihat : public instrument_base
    {
        instrument_drumhihat()
        {
            env.dAttackTime = 0.01;
            env.dDecayTime = 0.05;
            env.dSustainAmplitude = 0.0;
            env.dReleaseTime = 0.01;
            dMaxLifeTime = 1.0;
            name = "Drum HiHat";
            dVolume = 0.5;
        }

        virtual double sound(const double dTime, synth::note n, bool &bNoteFinished)
        {
            double dAmplitude = synth::env(dTime, env, n.on, n.off);
            if (dMaxLifeTime > 0.0 && dTime - n.on >= dMaxLifeTime)
                bNoteFinished = true;

            double dSound =
                + 0.1 * synth::osc(dTime - n.on, synth::scale(DRUM_NOTE -12, SCALE_DEFAULT), synth::OSC_SQUARE, 1.5, 1)
                + 0.9 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

            return dAmplitude * dSound * dVolume;
        }
    };

}   // namespace synth

#endif /* SYNTH_H */

