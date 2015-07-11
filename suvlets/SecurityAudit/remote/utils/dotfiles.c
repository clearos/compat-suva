#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstab.h>

#define TORRES_BASE "/var/lib/suvlets/SecurityAudit/"
#define LINE_LENGTH 4096

// Prototypes (note: safe_exec only takes up to ten arguments)
int safe_exec_pipe(int *out, char *path, ...);

// Main :: main
int main()
{
	int ret, in, pid, ret_wait, ret_val;
	FILE *exec_pipe;
	struct fstab *ftab;
	char line[LINE_LENGTH];

	// Be root
	setuid(0);

	// Open fstab
	if (setfsent() == 0)
		exit(1);

	// Loop through mounted filesystems
	while ((ftab = getfsent()) != NULL) {
		if (
			(strncmp(ftab->fs_vfstype, "ext2", 4) == 0) ||
			(strncmp(ftab->fs_vfstype, "ext3", 4) == 0) ||
			(strncmp(ftab->fs_vfstype, "ext4", 4) == 0) ||
			(strncmp(ftab->fs_vfstype, "resierfs", 8) == 0) ||
			(strncmp(ftab->fs_vfstype, "minixfs", 7) == 0) ||
			(strncmp(ftab->fs_vfstype, "xfs", 3) == 0)) {
				// Have a look at it
				pid = safe_exec_pipe(	&in, TORRES_BASE "find", "find", 
										ftab->fs_file, "-xdev", "-type", "d", "-name", ".*", 0);
				if (pid < 0)
					exit(2);

				// Wrap pipe in stream
				exec_pipe = fdopen(in, "r");
				if (exec_pipe == NULL)
					exit(3);

				while (fgets(line, LINE_LENGTH, exec_pipe)) {
					printf(line);
				}

				// Reap process
				ret_wait = wait(&ret_val);
				if (ret_wait == -1 || WEXITSTATUS(ret_val) != 0)
					exit(4);
		}
	}

	endfsent();
	return 0;
}



/*---------------------------
  Util Functions
---------------------------*/

#define LOCAL 0
#define REMOTE 1

// safe_exec :: Forks and executes a program, returns a pipe connected to it's stdout
int safe_exec_pipe(int *out, char *path, ...)
{
   	int pid;
	va_list ap;
	int num_args = 0;
	char *args[11];
	int exec_pipe[2];

	// Collect arguments
	va_start(ap, path);
	while(num_args < 10 && (args[num_args] = va_arg(ap, char *)))
		num_args++;
	va_end(ap);

	// Make certain it's null terminated even if we exceed the arg limit
	args[11] = 0;

	// Create pipes
	if (pipe(exec_pipe) < 0) {
		return -1;
	}

	// Fork
	pid = fork();
	if (pid < 0) { // Failed
		close(exec_pipe[LOCAL]);
		close(exec_pipe[REMOTE]);
		return(-1);
	}
	else if (pid > 0) { // Parent
		// Close unused end
		close(exec_pipe[REMOTE]);
		// Return file desc of pipe to child
		*out = exec_pipe[LOCAL];
	}
	else { // Child
		// Close unused end
		close(exec_pipe[LOCAL]);

		// Set stdout to caller:
		close(1);
		dup(exec_pipe[REMOTE]);

		// Exec command
		if (execv(path, args)) {
			exit(1);
		}
	}
	return pid;
}
#undef LOCAL
#undef REMOTE

