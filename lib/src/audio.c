/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "../include/audio.h"

void printe(char *err) {
    fprintf(stderr, "%s: %s, %s\n", PREFIX_ERROR, err, strerror(errno));
    exit(EXIT_FAILURE);
}

void init_data(t_data *data, const char *name, send_cb cb) {
    data->name = name;
    data->cb = cb;
    data->op = NULL;
    data->use_default = name == NULL ? true : false;
    data->volume.volume = -1;
    data->volume.mute = false;
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

void try_free_op(pa_operation **operation) {
    if (*operation != NULL) {
        pa_operation_unref(*operation);
        *operation = NULL;
    }
}

void sink_info_cb(pa_context *context, const pa_sink_info *info, int eol, void *main) {
    t_main *m;
    uint32_t info_volume;

    printf("sink_info_cb\n");
    (void) context;
    m = main;
    if (info != NULL && eol == 0) {
        info_volume = VOLUME(pa_cvolume_avg(&info->volume));
        if (m->sink->volume.mute != info->mute || m->sink->volume.volume != info_volume) {
            (*m->sink->cb)(m->cb_context, info_volume, info->mute);
        }
        m->sink->volume.mute = info->mute;
        m->sink->volume.volume = info_volume;
    }
    if (eol != 0) {
        try_free_op(&m->sink->op);
    }
}

void source_info_cb(pa_context *context, const pa_source_info *info, int eol,
                    void *main) {
    t_main *m;
    uint32_t info_volume;

    (void) context;
    m = main;
    if (info != NULL && eol == 0) {
        info_volume = VOLUME(pa_cvolume_avg(&info->volume));
        if (m->source->volume.mute != info->mute || m->source->volume.volume != info_volume) {
            (*m->source->cb)(m->cb_context, info_volume, info->mute);
        }
        m->source->volume.mute = info->mute;
        m->source->volume.volume = info_volume;
    }
    if (eol != 0) {
        try_free_op(&m->source->op);
    }
}

const char *name_switch(const char *old_name, const char *new_name) {
    if (old_name != NULL) {
        free((char *) old_name);
    }
    if ((old_name = malloc(sizeof(char) * (strlen(new_name) + 1))) == NULL) {
        printe("malloc failed");
    }
    return strcpy((char *) old_name, new_name);
}

void
server_info_cb(pa_context *context, const pa_server_info *info, void *main) {
    t_main *m;

    (void) context;
    m = main;
    if (info != NULL) {
        if (m->sink->use_default && (m->sink->name == NULL || strcmp(info->default_sink_name, m->sink->name) != 0)) {
            m->sink->name = name_switch(m->sink->name, info->default_sink_name);
            try_free_op(&m->sink->op);
            m->sink->op = pa_context_get_sink_info_by_name(m->context, m->sink->name, sink_info_cb, main);
        }
        if (m->source->use_default &&
            (m->source->name == NULL || strcmp(info->default_source_name, m->source->name) != 0)) {
            m->source->name = name_switch(m->source->name, info->default_source_name);
            try_free_op(&m->source->op);
            m->source->op = pa_context_get_source_info_by_name(m->context, m->source->name, source_info_cb, main);
        }
    }
    try_free_op(&m->server_op);
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

    // free pa_operation objects
//    try_free_op(&main->sink->op);
//    try_free_op(&main->source->op);
//    try_free_op(&main->server_op);

    // wait for the remaining time of the tick value
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tick, NULL);
}

void run(uint32_t tick, const char *sink_name, const char *source_name, void *cb_context, send_cb sink_cb,
         send_cb source_cb) {
    pa_proplist *proplist;
    t_main main;
    t_data sink;
    t_data source;
//    pa_operation *context_subscription;

    init_data(&sink, sink_name, sink_cb);
    init_data(&source, source_name, source_cb);

    main.tick = tick;
    main.connected = false;
    main.cb_context = cb_context;
    main.mainloop = pa_mainloop_new();
    main.api = pa_mainloop_get_api(main.mainloop);
    main.server_op = NULL;
    main.sink = &sink;
    main.source = &source;

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
        if (sink.use_default || source.use_default) {
            main.server_op = pa_context_get_server_info(main.context, server_info_cb, &main);
        }
        if (sink.name != NULL) {
            main.sink->op = pa_context_get_sink_info_by_name(main.context, sink.name, sink_info_cb, &main);
        }
        if (source.name != NULL) {
            main.source->op = pa_context_get_source_info_by_name(main.context, source.name, source_info_cb, &main);
        }
        iterate(&main);
    }

    // close connection and free
    pa_context_disconnect(main.context);
    pa_mainloop_free(main.mainloop);
}
