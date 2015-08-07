#include <stdlib.h>
#include <stdio.h>

#include "../../types/job.h"
#include "../../types/m_str.h"
#include "../../types/scope.h"

#include "../../util/str.h"
#include "../../util/debug.h"

#include "../exec.h"
#include "../env.h"
#include "../var.h"

// self-include
#include "flow.h"

extern scope_j *cscope;

int func_if     (job_j *job);
int func_else   (job_j *job);
int func_while  (job_j *job);
int func_with   (job_j *job);
int func_for    (job_j *job);
int func_cfor   (job_j *job);

int flow_builtin (job_j *job)
{
        if (job->argc == 0) return 0;

        char *f_arg = ms_strip (job->argv[0]);
        if (olstrcmp (f_arg, "if")) {
                return func_if (job);
        } else if (olstrcmp (f_arg, "for")) {
                return func_for (job);
        } else if (olstrcmp (f_arg, "with")) {
                return func_with (job);
        } else if (olstrcmp (f_arg, "cfor")) {
                return func_cfor (job);
        } else if (olstrcmp (f_arg, "while")) {
                return func_while (job);
        } else if (olstrcmp (f_arg, "else")) {
                return func_else (job);
        } else {
                return 0;
        }
}

int test (m_str **argv)
{
        int retval = -1;
        char invert = 0;

        // var eval
        size_t argc;
        for (argc = 0; argv[argc] != NULL; argc++);

        if (ms_mstrcmp(argv[0], ms_mask("not"))) {
                invert = 1;
                argv++;
        }

        m_str *teststr = ms_combine ((const m_str **)argv, argc, ' ');

        // TODO: try to hint
        //  - create separante int_test and path_test fns, which
        //      get called if hinting succeeds

        // = 
        if (ms_strchr (teststr, '=')) {
                m_str **comparand = ms_split (teststr, '=');
                devar_arg (&comparand[0]);
                devar_arg (&comparand[1]);
                ms_trim (&comparand[0]);
                ms_trim (&comparand[1]);

                retval = ms_ustrcmp (comparand[0], comparand[1]);

                ms_free (comparand[0]);
                ms_free (comparand[1]);
                free (comparand);
                goto cleanup;
        }

        devar_arg (&teststr);

        // file existence/type (keyword-based)
        

        // truthiness/non-emptiness
        // falsy: "", "false" ("0" et. al. to be supported in int_test
        if (olstrcmp(ms_strip(teststr), "") || olstrcmp (ms_strip(teststr), "false")) {
                retval = 0;
        } else {
                retval = 1;
        }

        cleanup:
                ms_free (teststr);
                return invert ^ retval;
}

// Returns 2 on false, 1 on true
int func_if (job_j *job)
{
        m_str **argv = job->argv + 1;
        size_t argc = job->argc - 1;

        // empty test is false
        if (argc == 0) return 2;

        if (test (argv)) {
                if (job->block != NULL) {
                        cscope = new_scope (cscope);
                        exec (job->block);
                        cscope = leave_scope (cscope);
                }
                return 1;
        } else return 2;
}

// the rest return 3

int func_else (job_j *job)
{
        return 0;
}

int func_while (job_j *job)
{
        m_str **argv = job->argv + 1;
        size_t argc = job->argc - 1;

        // empty test is false
        if (argc == 0) return 3;

        while (test (argv)) {
                if (job->block != NULL) {
                        cscope = new_scope (cscope);
                        exec (job->block);
                        cscope = leave_scope (cscope);
                }
        }

        return 3;
}

int func_for (job_j *job)
{
        m_str **argv = job->argv + 1;
        size_t argc = job->argc - 1;

        if (argc == 0 || job->block == NULL) return 3;

        int i;
        for (i = 0; i < argc; i++) {
                if (olstrcmp (ms_strip(argv[i]), "in")) break;
        }
        if (i == 0 || i == argc) {
                print_err ("No variable given for for loop.");
                return 3;
        }
        char *varname = ms_strip(argv[0]);
        argv += i + 1;
        argc -= i + 1;

        argc = devar_argv (argv, argc);
        for (i = 0; i < argc; i++) {
                set_msvar (varname, argv[i]);
                cscope = new_scope (cscope);
                exec (job->block);
                cscope = leave_scope (cscope);
        }

        return 3;
}

int func_cfor (job_j *job)
{
        return 0;
}

int func_with (job_j *job)
{
        return 0;
}
