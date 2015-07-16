#include <stdlib.h>

// local includes
#include "queue.h"

// self-include
#include "fn.h"

fn_j *fn_form (queue *lines)
{
        m_str *fline;
        q_pop (lines, (void **)&fline);

        m_str *fident = ms_advance (fline, 3);
        char *dash = ms_strchr(fident, '-');
        *dash = '\0';
        ms_updatelen(fident);

        m_str *fname = ms_dup (ms_trim (&fident));
        m_str *alist = ms_dup_at (fident, dash+1);
        ms_free (fident);

        char *sep = ms_strchr(alist, '{');
        if (sep == NULL) sep = ms_strchr(alist, ':');
        char single = 0;
        if (*sep == ':') single = 1;
        *sep = '\0';

        m_str *remaining = ms_dup_at(alist, sep+1);
        q_push (lines, remaining);

        fn_j *nfn = malloc(sizeof(fn_j));
        nfn->name = fname;
        nfn->arglist = al_make (alist);
        ms_free (alist);

        nfn->block = block_form (lines);

        return nfn;
}
