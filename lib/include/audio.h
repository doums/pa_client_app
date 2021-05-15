/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <pulse/context.h>
#include <pulse/proplist.h>
#include <pulse/mainloop.h>
#include <pulse/def.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>
#include <time.h>

#define PREFIX_ERROR "libaudio"
#define APPLICATION_NAME "baru"
#define NSEC_TO_SECOND(N) N / (long)1e9
#define MAX_NSEC 999999999

static bool alive = true;

typedef struct timespec
        t_timespec;

typedef struct main {
    uint32_t tick;
    bool connected;
    pa_context *context;
    pa_mainloop *mainloop;
    pa_mainloop_api *api;
    void *cb_context;
    t_timespec start;
} t_main;

void run(uint32_t tick,
         const char *sink_name,
         const char *source_name,
         void *cb_context);

#endif //AUDIO_H
