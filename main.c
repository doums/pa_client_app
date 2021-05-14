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

    run(100000000, NULL, NULL, ctx, cb_sink, cb_source);
    return 0;
}
