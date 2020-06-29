/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   jack_process.h
 * Author: sspresto
 *
 * Created on June 26, 2020, 9:53 AM
 */

#ifndef JACK_PROCESS_H
#define JACK_PROCESS_H

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>

#include "Jack_NoiseMaker.h"

int JACKstart (jack_client_t *jackclient_, NoiseMaker *a_noisy);
void JACKfinish ();
void jackshutdown (void *arg);

#endif /* JACK_PROCESS_H */
