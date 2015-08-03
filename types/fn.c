#include <stdlib.h>

// local includes
#include "queue.h"
#include "m_str.h"
#include "arglist.h"
#include "job_queue.h"

// self-include
#include "fn.h"

fn_j *fn_form (queue *lines)
{
        m_str *fline;
        q_pop (lines, (void **)&fline);

        m_str *fident = ms_advance (fline, 3);
        char *dash = ms_strchr(fident, '-');
        char single = 0;
        *dash = '\0';
        ms_updatelen(fident);

        ms_trim (&fident);
        m_str *fname = ms_dup (fident);
        m_str *alist = ms_dup_at (fident, dash+1);
        ms_free (fident);

        char *sep = ms_strchr(alist, '{');
        if (sep == NULL) sep = ms_strchr(alist, ':');
        single = 0;
        if (*sep == ':') single = 1;
        *sep = '\0';

        m_str *remaining = ms_dup_at(alist, sep+1);
        q_push (lines, remaining);

        fn_j *nfn = malloc(sizeof(fn_j));
        nfn->name = ms_strip(fname);
        nfn->args = al_make (alist);
        ms_free (alist);

        if (!single) {
                nfn->jq = jq_make(lines);
        } else {
                nfn->jq = jq_singleton(lines);
        }

        return nfn;
}
