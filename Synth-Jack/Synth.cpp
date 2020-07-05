/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <jack/jack.h>
#include "Synth.h"

double d_Note_Hz[128];

namespace synth
{
    double w (double dHertz)
    {
        return dHertz * 2.0 * M_PI;
    }
    
    void calc_note_frqs()
    {
        for(int i=0; i<128; i++)
        {
            // this converts a MIDI note to audio frequency (Hz)
            d_Note_Hz[i] = (440.0 / 32.0) * pow(2, (((double)i - 9.0) / 12.0));
        }
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
            return (2.0 / M_PI) * (dHertz * M_PI * fmod(dTime, 1.0 / dHertz) - (M_PI / 2.0));

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
                return d_Note_Hz[nNoteID];
            
            case SCALE_DEFAULT: default:
                return 8 * pow(1.0594630943592952645618252949463, nNoteID);
        }
    }
    
    double env(const double dTime, envelope &env, const double dTimeOn, const double dTimeOff)
    {
        return env.amplitude(dTime, dTimeOn, dTimeOff);
    }
}       // namespace synth
