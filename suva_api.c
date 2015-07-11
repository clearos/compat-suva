#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>

#include "suva_conf.h"
#include "mem.h"

static int init = 0;
struct SuvaConfig_t SuvaConfig;

void default_err_handler(int err_no, char *err_text);

void suva_api_die(void)
{
	mem_free_all(0);
}

void suva_api_init(void)
{
	struct sigaction signal_conf;

	if(init)
		return;
	
	init = 1;

	atexit(suva_api_die);

	memset(&signal_conf, 0, sizeof(struct sigaction));
	signal_conf.sa_handler = SIG_IGN;

	sigaction(SIGPIPE, &signal_conf, 0);

	mem_init();
	mem_add_type(1024, "Suva Connection");
}
