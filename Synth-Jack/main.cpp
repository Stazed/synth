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

double dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 220.0; // A2		// frequency of octave represented by keyboard
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);		// assuming western 12 notes per ocatve


static void signal_handler(int sig)
{
    jack_client_close(jackclient);
    fprintf(stderr, "signal received, exiting ...\n");
    exit(0);
}

double MakeNoise(double dTime)
{	
    double dOutput = sin(dFrequencyOutput * 2.0 * M_PI * dTime);
    
//    double dOutput = 1.0 * (sin(dFrequencyOutput * 2.0 * M_PI * dTime) + sin((dFrequencyOutput + 20.0) * 2.0 * M_PI * dTime));
    
    return dOutput * 0.5;
//    if(dOutput > 0.0)
//        return 0.2;
//    else
//        return -0.2;
    
}

#if 0

extern jack_default_audio_sample_t note_frqs[128];
jack_default_audio_sample_t note_time[128];
jack_default_audio_sample_t note_wave[128];

void calc_note_frqs(jack_default_audio_sample_t srate)
{
    int i;
    
    for(i=0; i<128; i++)
    {
        note_frqs[i] = (2.0 * 440.0 / 32.0) * pow(2, (((jack_default_audio_sample_t)i - 9.0) / 12.0)) / srate;
        
        note_time[i] = (2.0 / 32.0) * pow(2.0, (((jack_default_audio_sample_t)i - 9.0) / 12.0)) / srate;
        
        note_wave[i] = (440.0 / 32.0) * pow(2, (((jack_default_audio_sample_t)i - 9.0) / 12.0));
        
    }
    
    
/*
    // MIDI note to wave frequency
    float noteToFreq(int note)
    {
        int a = 440; //frequency of A (coomon value is 440Hz)
        return (a / 32) * pow(2, ((note - 9.0) / 12.0));
    }
 
    m_ramp += m_note_frqs[m_note];
    m_ramp = (m_ramp > 1.0) ? m_ramp - 2.0 : m_ramp;

    calculate_note = m_note_on*sin(2*M_PI*m_ramp); 
 */
    
    /* Debug */
#if 1
    int note = 69;  // 440 HZ = 69
    jack_default_audio_sample_t calculate_note = 0.0;
    jack_default_audio_sample_t a_ramp=0.0;
    
    jack_default_audio_sample_t a_ramp_time=0.0;
    jack_default_audio_sample_t a_ramp_wave=0.0;
    
    double dOutput = 0.0;
    double dTime = 0.0;
    
    double dTimeStep = 2.0 * (1.0 / (double)srate) ;
    double m_dGlobalTime = 0.0;
    
    double freq_time = 32.0 * pow(2.0, (((jack_default_audio_sample_t)i - 9.0) / 12.0)) / srate;
    
    for (int ii = 0; ii < 20; ii++)
    {
        a_ramp += note_frqs[note];
        a_ramp = (a_ramp > 1.0) ? a_ramp - 2.0 : a_ramp;

        calculate_note = 1 * sin(2 * M_PI * a_ramp);
        
/*        
        a_ramp_time += note_time[note];
        a_ramp_time = (a_ramp_time > 1.0) ? a_ramp_time - 2.0 : a_ramp_time;
        
        dTime = a_ramp_time;
        dOutput = sin(440 * 2.0 * M_PI * dTime);
*/    
        
        m_dGlobalTime = m_dGlobalTime + dTimeStep;
        
        a_ramp_wave += (note_wave[note] * 2) / srate ;
        a_ramp_wave = (a_ramp_wave > 1.0) ? a_ramp_wave - 2.0 : a_ramp_wave;
        
        dTime = m_dGlobalTime;
        dOutput = sin(note_wave[note] * 2.0 * M_PI * dTime);
        
        printf("out value = %f: note_freqs = %f: a_ramp = %f\n", calculate_note, note_frqs[note], a_ramp);
        
        printf("dOutput = %f: note_time = %f: note_wave = %f \n", dOutput, note_time[note], note_wave[note] );
        
        printf("Global time %f: dTimeStep = %f: freq_time = %f\n", m_dGlobalTime, dTimeStep, freq_time);
    }
#endif // 0
}

#endif // 0

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
                    nCurrentKey = -1;
                }

                dFrequencyOutput = 0.0;
                sound.SetNote(0.0); // turn note off
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

