#ifndef SCHED_H
#define SCHED_H

#include <types.h>

#define SCHED_TIMESLICE 14915
#define SCHED_TIMESLICE_MS SCHED_TIMESLICE / (3579545 / 3) * 1000

void sched_init();

#endif