/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "../include/audio.h"

void printe(char *err) {
    fprintf(stderr, "%s: %s, %s\n", PREFIX_ERROR, err, strerror(errno));
    exit(EXIT_FAILURE);
}

void context_state_cb(pa_context *context, void *main) {
    pa_context_state_t state;

    state = pa_context_get_state(context);
    if (state == PA_CONTEXT_READY) {
        ((t_main *) main)->connected = true;
    } else if (state == PA_CONTEXT_FAILED) {
        printe("context connection failed");
    }
}

void sink_info_cb(pa_context *context, const pa_sink_info *info, int eol, void *main) {
    printf("sink cb\n");
}

void source_info_cb(pa_context *context, const pa_source_info *info, int eol,
                    void *main) {
    printf("source cb\n");
}

void abs_time_tick(t_timespec *start, t_timespec *end, uint32_t tick) {
    long int sec;
    long int nsec;

    sec = start->tv_sec + (long int) NSEC_TO_SECOND(tick);
    nsec = start->tv_nsec + (long int) tick;
    if (nsec > MAX_NSEC) {
        end->tv_sec = sec + 1;
        end->tv_nsec = nsec - MAX_NSEC;
    } else {
        end->tv_sec = sec;
        end->tv_nsec = nsec;
    }
}

void iterate(t_main *main) {
    t_timespec tick;

    // get the time at the start of an iteration
    if (clock_gettime(CLOCK_REALTIME, &main->start) == -1) {
        printe("clock_gettime failed");
    }
    // get the absolute time of the next tick (start time + tick value)
    abs_time_tick(&main->start, &tick, main->tick);

    // iterate the main loop
    if (pa_mainloop_iterate(main->mainloop, 0, NULL) < 0) {
        printe("pa_mainloop_iterate failed");
    }

    // wait for the remaining time of the tick value
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tick, NULL);
}

void run(uint32_t tick, const char *sink_name, const char *source_name, void *cb_context) {
    pa_proplist *proplist;
    t_main main;

    main.tick = tick;
    main.connected = false;
    main.cb_context = cb_context;
    main.mainloop = pa_mainloop_new();
    main.api = pa_mainloop_get_api(main.mainloop);

    proplist = pa_proplist_new();

    // context creation
    if (pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, APPLICATION_NAME) != 0) {
        printe("pa_proplist_sets failed");
    }
    main.context = pa_context_new_with_proplist(main.api, APPLICATION_NAME, proplist);

    // context connection to the sever
    pa_context_set_state_callback(main.context, context_state_cb, &main);
    if (pa_context_connect(main.context, NULL, PA_CONTEXT_NOFAIL, NULL) < 0) {
        printe("pa_context_connect failed");
    }
    while (main.connected == false) {
        if (pa_mainloop_iterate(main.mainloop, 0, NULL) < 0) {
            printe("pa_mainloop_iterate failed");
        }
    }

    // iterate main loop
    while (alive) {
        pa_context_get_sink_info_by_name(main.context, sink_name, sink_info_cb, &main);
        pa_context_get_source_info_by_name(main.context, source_name, source_info_cb, &main);
        iterate(&main);
    }

    // close connection and free
    pa_context_disconnect(main.context);
    pa_mainloop_free(main.mainloop);
}
