/* Intrusive conversions between pending inputs and their owning jobs. */
#include "cl.h"
#include <recon.h>

/* 00403b5d..00403b8e: DeferredInput is embedded twelve bytes into Job. */
Job *jobs_from_deferred(DeferredInput *inputs)
{
    Job *jobs = NULL, *last = NULL, *job;
    for (; inputs; inputs = inputs->next) {
        job = CONTAINING_RECORD(inputs, Job, deferred);
        if (last)
            last->next = job;
        else
            jobs = job;
        last = job;
    }
    if (last)
        last->next = NULL;
    return jobs;
}
