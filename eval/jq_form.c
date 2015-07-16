

// local includes
// HERE'S WHERE THE TYPE HELL BEGINS
#include "../types/queue.h"
#include "../types/stack.h"
#include "../types/m_str.h"
#include "../types/job_queue.h"
#include "../types/job.h"
#include "../types/block.h"
#include "../types/fn.h"
#include "../types/linkedlist.h"
#include "../types/hashtable.h"

// to insert functions into the fn table
#include "../exec/env.h"

// self-include
#include "jq_form.h"

extern queue *elines;
extern job_queue *jobs;

void jq_form (void)
{
        jobs = jq_make(elines);
}

// To do: job_form
//      block_form
//      fn_form
//      jq_add_block
//      jq_add_job
//

