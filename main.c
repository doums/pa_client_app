#include <stdio.h>
#include "lib/include/audio.h"

typedef struct context
        t_context;

int main() {
    t_context *ctx = NULL;

    // tick in nanosecond, change the sink and source name by the ones you want to use
    run(100000000, "alsa_output.usb-Kingston_HyperX_Virtual_Surround_Sound_00000000-00.analog-stereo",
        "alsa_output.usb-Kingston_HyperX_Virtual_Surround_Sound_00000000-00.analog-stereo.monitor",
        ctx);
    return 0;
}
