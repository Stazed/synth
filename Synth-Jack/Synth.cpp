/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Synth.h"

namespace synth
{
    double w (double dHertz)
    {
        return dHertz * 2.0 * M_PI;
    }
    
    double osc(double dHertz, double dTime, int nType)
    {
        switch(nType)
        {

        case 0: // sine wave
            return sin(w(dHertz) * dTime);

        case 1: // square wave
            return sin(w(dHertz) * dTime) > 0.0 ? 1.0 : -1.0;

        case 2: // triangle wave
            return asin(sin(w(dHertz) * dTime)) * 2.0 * M_PI;

        case 3: // saw wave (analog / warm / slow)
        {
            double dOutput = 0.0;

            for(double n = 1.0; n < 100.0; n++)
            {
                dOutput += (sin(n * w(dHertz) * dTime)) / n;
            }

            return dOutput * (2.0 / M_PI); 
        }

        case 4: // saw wave (optimized / harsh / fast)
            return (2.0 * M_PI) * (dHertz * M_PI * fmod(dTime, 1.0 / dHertz) - (M_PI / 2.0));

        case 5: // pseudo random noise
            return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

        default:
            return 0.0;
        }
    }
}       // namespace synth