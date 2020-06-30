/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: sspresto
 *
 * Created on June 26, 2020, 8:51 AM
 */

#include <cstdlib>
#include <cstdio>
#include <unistd.h>     /* sleep() */
#include <signal.h>
#include <math.h>
#include <ncurses.h>

#include "jack_process.h"
#include "Jack_NoiseMaker.h"
#include "LinuxInput.h"     // https://github.com/Barracuda72/synth.git

using namespace std;

/* Switch between MIDI and PC note control */
bool MIDI_ON = false;

extern jack_client_t *jackclient;
jack_status_t status;
char jackcliname[64];

static void signal_handler(int sig)
{
    jack_client_close(jackclient);
    fprintf(stderr, "signal received, exiting ...\n");
    exit(0);
}

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

double dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 110.0; // A2		// frequency of octave represented by keyboard
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);		// assuming western 12 notes per ocatve
sEnvelopeADSR envelope;

double MakeNoise(double dTime)
{	
    double dOutput = envelope.GetAmplitude(dTime) *
    (
        + osc(dFrequencyOutput * 0.5, dTime, 3)
        + osc(dFrequencyOutput * 1.0, dTime, 1)
    );
    
    return dOutput * 0.4;   // master volume
}

int srate(jack_nframes_t nframes, void *arg)
{
    printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
//    calc_note_frqs((jack_default_audio_sample_t)nframes);
    return 0;
}

/*
 * 
 */
int main(int argc, char** argv)
{
    WINDOW *w = initscr();
    cbreak();
    nodelay(w, TRUE);
    noecho();
    
    char temp[256];
    sprintf (temp, "synth-jack");
    
    jackclient = jack_client_open (temp, JackNoStartServer, &status, NULL);

    if (jackclient == NULL)
    {
        fprintf (stderr, "Cannot make a jack client, is jackd running?\n");
        int nojack = 1;
        return nojack;
    }
    
    NoiseMaker sound((double)jack_get_sample_rate (jackclient), jack_get_buffer_size (jackclient));
    
    // Link noise function with sound machine
    sound.SetUserFunction(MakeNoise);
    sound.SetFreqVariable(dFrequencyOutput);
    sound.SetEnvelope(envelope);
    
    jack_set_sample_rate_callback (jackclient, srate, 0);
    
    JACKstart(jackclient, &sound);

    /* To quit program ctrl-c */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    /* Ncurses Keyboard */
    auto draw = [&w](int x, int y, std::string ws)
    {
        mvwaddstr(w, y, x, ws.c_str());
    };


    int keys[16] = {
        KEY_Z,
        KEY_S,
        KEY_X,
        KEY_C,
        KEY_F,
        KEY_V,
        KEY_G,
        KEY_B,
        KEY_N,
        KEY_J,
        KEY_M,
        KEY_K,
        KEY_COMMA,
        KEY_L,
        KEY_DOT,
        KEY_SLASH,
    };
    
    FILE* input = start_input();
    char* all_keys = get_all_keys(input);
    
    // Sit in loop, capturing keyboard state changes and modify
    // synthesizer output accordingly
    int nCurrentKey = -1;	
    bool bKeyPressed = false;
    
    while(1)
    {
        if(!MIDI_ON)
        {
            bKeyPressed = false;
            read_keys(input, all_keys);
            for (int k = 0; k < 16; k++)
            {
                int nKeyState = check_key_state(all_keys, keys[k]);

                if(nKeyState)
                {
                    if (nCurrentKey != k)
                    {					
                        dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
                        envelope.NoteOn(sound.GetTime());
                        mvprintw(2, 15, "\rNote On : %fs %fHz", sound.GetTime(), dFrequencyOutput);
                        wrefresh(w);				
                        nCurrentKey = k;
                    }

                    bKeyPressed = true;
                    sound.SetNote(1.0); // turn note on
                }
            }

            if (!bKeyPressed)
            {	
                if (nCurrentKey != -1)
                {
                    mvprintw(2, 15,"\rNote Off: %fs                   ", sound.GetTime());
                    wrefresh(w);
                    envelope.NoteOff(sound.GetTime());
                    nCurrentKey = -1;
                }

                //dFrequencyOutput = 0.0;
                //sound.SetNote(0.0); // turn note off
            }

            draw(2, 8,  "|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |  ");
            draw(2, 9,  "|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |  ");
            draw(2, 10, "|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__");
            draw(2, 11, "|     |     |     |     |     |     |     |     |     |     |");
            draw(2, 12, "|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |");
            draw(2, 13, "|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|");

            wrefresh(w);
            // /VISUAL
        }
        if (get_key_state(input, KEY_Q))
            break;
        
        sleep(.1);
    }


//    J_SAMPLE_RATE = jack_get_sample_rate (jackclient);
//    J_PERIOD = jack_get_buffer_size (jackclient);

    JACKfinish ();
    
    return 0;
}

