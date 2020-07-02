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
#include "Synth.h"

using namespace std;

extern jack_client_t *jackclient;
jack_status_t status;
char jackcliname[64];

static void signal_handler(int sig)
{
    jack_client_close(jackclient);
    fprintf(stderr, "signal received, exiting ...\n");
    exit(0);
}

double dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 110.0; // A2		// frequency of octave represented by keyboard
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);		// assuming western 12 notes per ocatve
synth::sEnvelopeADSR envelope;

vector<synth::note> vecNotes;
mutex muxNotes;
synth::instrument_bell instBell;
synth::instrument_harmonica instHarm;

typedef bool(*lambda)(synth::note const& item);
template<class T>
void safe_remove(T &v, lambda f)
{
    auto n = v.begin();
    while (n != v.end())
    {
        if (!f(*n))
            n = v.erase(n);
        else
            ++n;
    }
}

double MakeNoise(int nChannel, double dTime)
{	
    unique_lock<mutex> lm(muxNotes);
    double dMixedOutput = 0.0;
    
    for (auto &n : vecNotes)
    {
        bool bNoteFinished = false;
        double dSound = 0;
        if(n.channel == 2)
            dSound = instBell.sound(dTime, n, bNoteFinished);
        if(n.channel == 1)
            dSound = instHarm.sound(dTime, n, bNoteFinished) * 0.5;
        
        dMixedOutput += dSound;
        
        if(bNoteFinished && n.off > n.on)
            n.active = false;
    }
    
    safe_remove<vector<synth::note>>(vecNotes, [](synth::note const& item) {return item.active; });
    
    return dMixedOutput * 0.2;   // master volume
}

int srate(jack_nframes_t nframes, void *arg)
{
    printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
    
    NoiseMaker *sound = (NoiseMaker *) arg;
    
    sound->SetTimeStep(nframes);

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
    
    jack_set_sample_rate_callback (jackclient, srate, &sound);
    
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
    
    if(input == nullptr)
    {
        mvprintw(2, 2, "NULL input file - check if user is in 'input' group");
        mvprintw(3, 2, "Also check for correct symlink in LinuxInput.h start_input()");
        wrefresh(w);
        endwin();
        JACKfinish ();
        return 0;
    }
    
    char* all_keys = get_all_keys(input);
    
    // Sit in loop, capturing keyboard state changes and modify
    // synthesizer output accordingly
    int nCurrentKey = -1;	
    bool bKeyPressed = false;
    
    while(1)
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
        }

        draw(2, 8,  "|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |  ");
        draw(2, 9,  "|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |  ");
        draw(2, 10, "|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__");
        draw(2, 11, "|     |     |     |     |     |     |     |     |     |     |");
        draw(2, 12, "|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |");
        draw(2, 13, "|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|");

        draw(2, 6, "Press Q to quit...");

        wrefresh(w);
        // /VISUAL
        
        if (get_key_state(input, KEY_Q))
            break;
        
        sleep(.1);
    }
    
    stop_input(input);
    free(all_keys);

    endwin();

    JACKfinish ();
    
    return 0;
}

