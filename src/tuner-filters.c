#include <glib.h>
#include <stdlib.h>
#include "tuner.h"

static const gint filters[] =
{
    15,
    14,
    13,
    12,
    11,
    10,
    9,
    8,
    7,
    6,
    5,
    4,
    3,
    2,
    1,
    0,
    -1
};

#define FILTERS_COUNT (sizeof(filters) / sizeof(gint))
static const gint filters_bw[][FILTERS_COUNT] =
{
    { /* FM */
        311000,
        287000,
        254000,
        236000,
        217000,
        200000,
        184000,
        168000,
        151000,
        133000,
        114000,
         97000,
         84000,
         72000,
         64000,
         56000,
            -1
    },
    { /* AM */
         8000,
         6000,
         4000,
         3000,
            0
    }
};

gint
tuner_filter_from_index(gint index)
{
    if(index >= 0 && index < FILTERS_COUNT)
        return filters[index];
    return -1; /* unknown -> adaptive */
}

gint
tuner_filter_index(gint filter)
{
    gint i;
    for(i=0; i<FILTERS_COUNT; i++)
        if(filters[i] == filter)
            return i;
    return -1; /* unknown -> none set */
}

gint
tuner_filter_bw(gint filter)
{
    gint position = tuner_filter_index(filter);
    if(position >= 0 && (tuner.mode == MODE_FM || tuner.mode == MODE_AM))
        return filters_bw[tuner.mode][position];
    return 0; /* unknown bandwidth */
}

gint
tuner_filter_bw_from_index(gint index)
{
    if(index >= 0 && index < FILTERS_COUNT && (tuner.mode == MODE_FM || tuner.mode == MODE_AM))
        return filters_bw[tuner.mode][index];
    return 0; /* unknown bandwidth */
}

gint
tuner_filter_index_from_bw(gint bw)
{
    gint index = FILTERS_COUNT-1;
    gint min = G_MAXINT;
    gint diff, i;

    if(bw >= 0)
    {
        for(i=0; i<FILTERS_COUNT-1; i++)
        {
            diff = abs(tuner_filter_bw_from_index(i)-bw);
            if(min > diff)
            {
                min = diff;
                index = i;
            }
        }
    }
    return index;
}

gint
tuner_filter_count()
{
    return FILTERS_COUNT;
}
