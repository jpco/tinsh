#include <stdlib.h>

// local includes
#include "../types/m_str.h"
#include "../types/queue.h"
#include "../types/linkedlist.h"
#include "../types/block.h"

#include "../util/debug.h"

#include "mask.h"
#include "comment.h"
#include "spl_line.h"
#include "spl_pipe.h"

// self-include
#include "eval.h"

queue *ejobs;
queue *elines;

// essentially the global block;
// what is to be returned.
job_queue *jobs;

job_queue *eval (queue *lines)
{
        // initialize eval data
        ejobs = q_make();
        elines = lines;

        if (ejobs == NULL || elines == NULL) {
                if (ejobs != NULL) free (ejobs);
                if (elines != NULL) free (elines);
                return NULL;
        }

        q_push (ejobs, mask_eval);

        while (q_len (ejobs) > 0) {
                void (*ejob)(void);
                q_pop (ejobs, (void **)&ejob);
                ejob();
        }

        // free eval data
        ll_destroy (ejobs);
        ll_destroy (elines);

        // return
        return jobs;
}
