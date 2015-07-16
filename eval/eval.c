

// local includes
#include "../types/m_str.h"
#include "../types/queue.h"
#include "../types/block.h"

#include "../util/debug.h"

#include "mask.h"
#include "comment.h"
#include "spl_line.h"
#include "spl_pipe.h"
#include "job_form.h"
#include "block_form.h"

// self-include
#include "eval.h"

queue *ejobs;
queue *elines;

// essentially the global block;
// what is to be returned.
job_queue *jobs;

job_queue *eval (char **lines)
{
        // initialize eval data
        ejobs = q_make();
        elines = q_make();

        if (ejobs == NULL || elines == NULL) {
                if (ejobs != NULL) free (ejobs);
                if (elines != NULL) free (elines);
                return NULL;
        }

        size_t i;
        char *cline;
        for (i = 0; (cline = lines[i]) != NULL; i++) {
                q_push (elines, lines[i]);
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
