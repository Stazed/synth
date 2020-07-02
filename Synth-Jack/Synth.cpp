/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Synth.h"

jack_default_audio_sample_t *note_hz;

namespace synth
{
    double w (double dHertz)
    {
        return dHertz * 2.0 * M_PI;
    }
    
    double osc(const double dTime, const double dHertz, const int nType,
            const double dLFOHertz, const double dLFOAmplitude, double dCustom)
    {
        double dFreq = w(dHertz) * dTime + dLFOAmplitude * dHertz * (sin(w(dLFOHertz) * dTime));
        
        switch(nType)
        {
        case OSC_SINE: // sine wave between -1 and +1
            return sin(dFreq);

        case OSC_SQUARE: // square wave between -1 and +1
            return sin(dFreq) > 0.0 ? 1.0 : -1.0;

        case OSC_TRIANGLE: // triangle wave between -1 and +1
            return asin(sin(dFreq)) * (2.0 * M_PI);

        case OSC_SAW_ANA: // saw wave (analog / warm / slow)
        {
            double dOutput = 0.0;

            for(double n = 1.0; n < dCustom; n++)
            {
                dOutput += (sin(n * dFreq)) / n;
            }

            return dOutput * (2.0 / M_PI); 
        }

        case OSC_SAW_DIG: // saw wave (optimized / harsh / fast)
            return (2.0 * M_PI) * (dHertz * M_PI * fmod(dTime, 1.0 / dHertz) - (M_PI / 2.0));

        case OSC_NOISE: // pseudo random noise
            return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

        default:
            return 0.0;
        }
    }
    
    double scale(const int nNoteID, const int nScaleID)
    {
        switch (nScaleID)
        {
            case MIDI_NOTE:
                return note_hz[nNoteID];
            
            case SCALE_DEFAULT: default:
                return 256 * pow(1.0594630943592952645618252949463, nNoteID);
        }
    }
    
    double env(const double dTime, envelope &env, const double dTimeOn, const double dTimeOff)
    {
        return env.amplitude(dTime, dTimeOn, dTimeOff);
    }
}       // namespace synth