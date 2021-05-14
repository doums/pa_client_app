#include <stdio.h>
#include "lib/include/audio.h"

typedef struct context
        t_context;

void cb_sink(void *context, uint32_t volume, bool mute) {
    printf("sink cb: v %d, m %d\n", volume, mute);
}

void cb_source(void *context, uint32_t volume, bool mute) {
    printf("source cb: v %d, m %d\n", volume, mute);
}

int main() {
    t_context *ctx = NULL;

//    run(500, NULL, NULL, ctx, cb_sink, cb_source);
//    run(500, "alsa_output.usb-Kingston_HyperX_Virtual_Surround_Sound_00000000-00.analog-stereo", "alsa_output.usb-Kingston_HyperX_Virtual_Surround_Sound_00000000-00.analog-stereo.monitor", ctx, cb_sink, cb_source);
    run(50000, NULL, "alsa_output.usb-Kingston_HyperX_Virtual_Surround_Sound_00000000-00.analog-stereo.monitor", ctx, cb_sink, cb_source);
    return 0;
}
