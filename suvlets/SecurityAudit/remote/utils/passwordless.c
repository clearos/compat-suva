#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>
#include <shadow.h>
#include <sys/types.h>

int main()
{
	struct spwd *spwd_data;
	setuid(0);
	setspent();
	while ((spwd_data = getspent()) != NULL) {
		if (strncmp(spwd_data->sp_pwdp, "", 1) == 0)
			printf("%s\n", spwd_data->sp_namp);
	}
	endspent();
	return 0;
}
