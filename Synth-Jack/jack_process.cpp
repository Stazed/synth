/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   jack_process.cpp
 * Author: sspresto
 * 
 * Created on June 26, 2020, 9:53 AM
 */

#include "jack_process.h"
#include <cstdio>
#include <unistd.h>     /* usleep() */
#include <math.h>


jack_client_t *jackclient;
jack_port_t *outport_left, *outport_right;
jack_port_t *jack_midi_in;

unsigned char note = 0;
void *dataout;


int jackprocess (jack_nframes_t nframes, void *arg);

int JACKstart (jack_client_t * jackclient_, NoiseMaker * a_noisy)
{
    jackclient = jackclient_;
    
    NoiseMaker *synth = a_noisy;

    jack_set_process_callback (jackclient, jackprocess, (void *) synth);

    jack_on_shutdown (jackclient, jackshutdown, (void *) synth);

    outport_left =  jack_port_register (jackclient, "out_1", JACK_DEFAULT_AUDIO_TYPE,
                            JackPortIsOutput, 0);
    outport_right = jack_port_register (jackclient, "out_2", JACK_DEFAULT_AUDIO_TYPE,
                            JackPortIsOutput, 0);

    jack_midi_in =  jack_port_register(jackclient, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);


    if (jack_activate (jackclient)) {
        fprintf (stderr, "Cannot activate jack client.\n");
        return (2); //  not used
    };

    return 3;   // not used
};



int jackprocess (jack_nframes_t nframes, void *arg)
{
    NoiseMaker *synth = (NoiseMaker *) arg;
    
    int i, event_count;
    jack_midi_event_t midievent;
    jack_nframes_t event_index = 0;
    
    jack_default_audio_sample_t calculate_note = 0.0;

    jack_default_audio_sample_t *outl = (jack_default_audio_sample_t *)
                                        jack_port_get_buffer (outport_left, nframes);
    jack_default_audio_sample_t *outr = (jack_default_audio_sample_t *)
                                        jack_port_get_buffer (outport_right, nframes);

    
    float *midi_port_buffer = (float *)jack_port_get_buffer(jack_midi_in, nframes);
    event_count = jack_midi_get_event_count(midi_port_buffer);
    
    for (i = 0; i < event_count; i++)
    {
        jack_midi_event_get(&midievent, midi_port_buffer, i);
        synth->ProcessMidi(&midievent);
    }
    
    synth->MainThread(outl, outr);

    return 0;
};

void JACKfinish ()
{
    printf("JACKfinish\n");
    jack_client_close (jackclient);
    usleep (1000);
};

void jackshutdown (void *arg)
{
    NoiseMaker *synth = (NoiseMaker *) arg;
    
    synth->jackshutdown(0);
};
