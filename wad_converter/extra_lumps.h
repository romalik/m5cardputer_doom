#ifndef EXTRA_LUMPS_H__
#define EXTRA_LUMPS_H__

#include <stdint.h>
#include "extra_lumps_gen.h"


struct extra_lump {
    char name[9];
    uint32_t length;
    char * data;
};

extern struct extra_lump extra_lumps[EXTRA_LUMPS_N];

#endif