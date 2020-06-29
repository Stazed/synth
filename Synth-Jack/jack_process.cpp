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

//jack_port_t *inputport_left, *inputport_right;
//jack_port_t *jack_midi_out;

jack_default_audio_sample_t ramp=0.0;
jack_default_audio_sample_t note_on;
jack_default_audio_sample_t note_frqs[128];

unsigned char note = 0;
void *dataout;


int jackprocess (jack_nframes_t nframes, void *arg);

int JACKstart (jack_client_t * jackclient_, NoiseMaker * a_noisy)
{
    jackclient = jackclient_;
    
    NoiseMaker *synth = a_noisy;

    jack_set_process_callback (jackclient, jackprocess, (void *) synth);

    jack_on_shutdown (jackclient, jackshutdown, (void *) synth);


//    inputport_left =
//        jack_port_register (jackclient, "in_1", JACK_DEFAULT_AUDIO_TYPE,
//                            JackPortIsInput, 0);
//    inputport_right =
//        jack_port_register (jackclient, "in_2", JACK_DEFAULT_AUDIO_TYPE,
//                            JackPortIsInput, 0);

    outport_left =  jack_port_register (jackclient, "out_1", JACK_DEFAULT_AUDIO_TYPE,
                            JackPortIsOutput, 0);
    outport_right = jack_port_register (jackclient, "out_2", JACK_DEFAULT_AUDIO_TYPE,
                            JackPortIsOutput, 0);

    jack_midi_in =  jack_port_register(jackclient, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

//    jack_midi_out =
//        jack_port_register(jackclient, "MC out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);


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

//    jack_default_audio_sample_t *inl = (jack_default_audio_sample_t *)
//                                       jack_port_get_buffer (inputport_left, nframes);
//    jack_default_audio_sample_t *inr = (jack_default_audio_sample_t *)
//                                       jack_port_get_buffer (inputport_right, nframes);

    
    float *midi_port_buffer = (float *)jack_port_get_buffer(jack_midi_in, nframes);
    event_count = jack_midi_get_event_count(midi_port_buffer);
    
    for (i = 0; i < event_count; i++)
    {
        jack_midi_event_get(&midievent, midi_port_buffer, i);
        synth->ProcessMidi(&midievent);
    }
    
    synth->MainThread(outl, outr);

#if 0
    if(event_count > 1)
    {
        printf(" midisine: have %d events\n", event_count);
        for(i=0; i<event_count; i++)
        {
            jack_midi_event_get(&midievent, midi_port_buffer, i);
            printf("    event %d time is %d. 1st byte is 0x%x\n", i, midievent.time, *(midievent.buffer));
        }
    }
    
    dataout = jack_port_get_buffer(jack_midi_out, nframes);
    jack_midi_clear_buffer(dataout);

    jack_midi_event_get(&midievent, midi_port_buffer, 0);
    for (i = 0; i < nframes; i++)
    {    
        if((midievent.time == i) && (event_index < event_count))
        {
            if( ((*(midievent.buffer) & 0xf0)) == 0x90 )
            {
                    /* note on */
                    note = *(midievent.buffer + 1);
                    note_on = 1.0;
            }
            
            else if( ((*(midievent.buffer)) & 0xf0) == 0x80 )
            {
                    /* note off */
                    note = *(midievent.buffer + 1);
                    note_on = 0.0;
            }
            
            event_index++;
            if(event_index < event_count)
            {
                jack_midi_event_get(&midievent, midi_port_buffer, event_index);
            }
        }
        
        ramp += note_frqs[note];
        ramp = (ramp > 1.0) ? ramp - 2.0 : ramp;
        
        calculate_note = note_on*sin(2*M_PI*ramp);
        
        outl[i] = calculate_note;
        outr[i] = calculate_note;
    }

    memcpy (JackOUT->efxoutl, inl,
            sizeof (jack_default_audio_sample_t) * nframes);
    memcpy (JackOUT->efxoutr, inr,
            sizeof (jack_default_audio_sample_t) * nframes);

    JackOUT->Alg (JackOUT->efxoutl, JackOUT->efxoutr, inl, inr ,0);

    memcpy (outl, JackOUT->efxoutl,
            sizeof (jack_default_audio_sample_t) * nframes);
    memcpy (outr, JackOUT->efxoutr,
            sizeof (jack_default_audio_sample_t) * nframes);
#endif // 0
    
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
#if 0
    if (gui == 0)
        printf ("Jack Shut Down, sorry.\n");
    else
        JackOUT->jshut=1;
#endif // 0
};
