// local includes
// HERE'S WHERE THE TYPE HELL BEGINS
#include "../types/queue.h"
#include "../types/job_queue.h"

// self-include
#include "jq_form.h"

extern queue *elines;
extern job_queue *jobs;

void jq_form (void)
{
        jobs = jq_make(elines);
}
