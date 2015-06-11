#include <stdio.h>
#include "doc.h"
#include "cmd_help.h"

char sampler_doc[] = CLI_CMD_HELPTEXT_calibrate;

void doc_print()
{
    printf("%s\n", sampler_doc);
}
