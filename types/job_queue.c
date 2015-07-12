// local includes
#include "block.h"

// self-include
#include "job_queue.h"

typedef struct jq_elt_str {
        char is_block;
        union dat {
                block_j bl;
                job_j job;
        };
} jq_elt;
