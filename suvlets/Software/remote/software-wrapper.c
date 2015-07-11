#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <grp.h>
#include <string.h>

int main(int argc, char *argv[]) {
	char cmd[200];
	regex_t regex;
	char *pattern = "^([-A-Za-z0-9._]+)$";
	int inx;

	// Two arguments only
	if (argc != 3) {
		printf("Incorrect number of arguments\n");
		return -1;
	}

	// No funny punctuation
	regcomp(&regex, pattern, REG_EXTENDED);
	inx = regexec(&regex, argv[1], (size_t) 0, NULL, 0);
	if (inx != 0) {
		printf("Invalid characters\n");
		return -1;
	}

	inx = regexec(&regex, argv[2], (size_t) 0, NULL, 0);
	if (inx != 0) {
		printf("Invalid characters\n");
		return -1;
	}

	if (strncmp(argv[1], "-e", 2) == 0) {
		sprintf(cmd, "/var/lib/cobalt/uninstallers/%s", argv[2]);
	} else if (strncmp(argv[1], "-i", 2) == 0) {
		sprintf(cmd, "/usr/local/sbin/cobalt_upgrade /home/packages/%s", argv[2]);
	} else if (strncmp(argv[1], "-r", 2) == 0) {
		sprintf(cmd, "/bin/chown admin.suvlet /home/packages;/bin/chmod 775 /home/packages");
	} else {
		printf("Invalid flag\n");
		return 1;
	}

	printf("Running: %s\n", cmd);
	setgroups(0, NULL);
	setgid(0); setuid(0);
	system(cmd);

	return 0;
}
