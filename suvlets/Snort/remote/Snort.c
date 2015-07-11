#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "Snort.h"
#include "suva_receiver.h"

int main(int argc, char *argv[])
{
    constructor();
    fprintf(stderr, "Init receiver.\n");
    suva_init_receiver();

    for(;;)
        suva_get_request();

    fprintf(stderr, "Shutdown.\n");
}


void constructor()
{}

void deconstructor()
{}
