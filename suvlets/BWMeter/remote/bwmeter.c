#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "bwmeter.h"
#include "suva_receiver.h"

extern int scl_crc(char *data, int len);

int main(int argc, char *argv[])
{
	constructor();

	fprintf(stderr, "Init receiver.\n");

	suva_init_receiver();

	for(;;)
		suva_get_request();

	fprintf(stderr, "Shutdown.\n");
}

unsigned long xfer_chunk(unsigned char *data, long length)
{
	return scl_crc((char *)data, length);
}

void constructor()
{
	fprintf(stderr, "Constructor.\n");
}

void deconstructor()
{
	fprintf(stderr, "Deconstructor.\n");
}
