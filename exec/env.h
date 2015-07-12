#ifndef JPSH_EXEC_ENV_H
#define JPSH_EXEC_ENV_H

hashtable *alias_tab;
hashtable *fn_tab;
stack *gstack;
stack *cstack;  // defines full scope stack within stack type as well

#endif
