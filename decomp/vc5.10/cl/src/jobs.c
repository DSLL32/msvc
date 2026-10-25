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

/* 00403b8e..00403bb7: retain Job links and chain their embedded inputs. */
DeferredInput *deferred_from_jobs(Job *jobs)
{
    DeferredInput *inputs = NULL, *last = NULL, *input;
    for (; jobs; jobs = jobs->next) {
        input = &jobs->deferred;
        if (last)
            last->next = input;
        else
            inputs = input;
        last = input;
    }
    if (last)
        last->next = NULL;
    return inputs;
}
