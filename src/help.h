/* Help Window */

#include "help.hxx"

Help* help_create(void);

void help_destroy(Help** help);

void help_show(Help* help, const V2Df pos);

void help_hide(Help* help);

bool_t help_is_open(void);
