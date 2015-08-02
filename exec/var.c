#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// local includes
#include "../types/m_str.h"
#include "../types/job.h"
#include "../types/var.h"

#include "../util/str.h"
#include "../util/debug.h"
#include "../util/vector.h"

#include "env.h"

// self-include
#include "var.h"

void var_eval (job_j *job)
{
        m_str **argv = job->argv;

        int i;
        for (i = 0; i < job->argc; i++) {
                // check for var
                if (!ms_strchr (argv[i], '(')) continue;
                m_str *arg = ms_dup(argv[i]);
                mbuf_strchr(arg, '(');
                ms_advance(arg, 1);
                if (!ms_strchr (arg, ')')) {
                        free (arg);
                        continue;
                }
                *(ms_strchr (arg, ')')) = 0;
                ms_updatelen (arg);
                m_str *val = devar (arg);

                // insert value

        }
}

m_str *devar (m_str *name)
{
        // eval/exec
}
