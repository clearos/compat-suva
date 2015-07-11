#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "SecurityAudit.h"
#include "suva_receiver.h"

#define SUVLET_VERSION 6
#define LINE_LENGTH 4096
#define COPY_BUFFER_SIZE 4096
#define TORRES_BASE "/var/lib/suvlets/SecurityAudit/"

// Data types
struct buffer_desc
{
    char *buffer;
	int buffer_size;
	int buffer_pos;
	char superseded;
};

// Prototypes (note: safe_exec etc. only take up to ten arguments)
int safe_exec(char *path, ...);
int safe_exec_pipe(int *out, char *path, ...);
void add_line(struct buffer_desc* buf, char *line);
void clear_buffer(struct buffer_desc* buf);
int get_match(char *needle, FILE *haystack);
char *db_diff(char *prog, char *prog_name, char *db_file);
int file_copy(char *from, char *to);
void exit_clean_up();

// Global vars
char line[LINE_LENGTH];
extern int suvlet_version;
struct buffer_desc buf;
struct buffer_desc buf2;
char *tmpDir;

// De/Constructor & Main
void constructor()
{
	// Init buffers
    memset(&buf, 0, sizeof(struct buffer_desc));
    memset(&buf2, 0, sizeof(struct buffer_desc));
	add_line(&buf, "null");
	add_line(&buf2, "null");
}

void deconstructor()
{
	if (tmpDir != NULL) {
		safe_exec("/bin/rm", "rm", "-r", tmpDir, 0);
		free(tmpDir);
	}

	free(buf.buffer);
	free(buf2.buffer);
}

int main(int argc, char *argv[])
{
	suvlet_version = SUVLET_VERSION;
	tmpDir = NULL;
	atexit(exit_clean_up);
    constructor();
    fprintf(stderr, "Init receiver.\n");
    suva_init_receiver();

    for(;;)
        suva_get_request();
}

// exit_clean_up :: Just being paranoid
void exit_clean_up()
{
	if (tmpDir != NULL)
		safe_exec("/bin/rm", "rm", "-r", tmpDir, 0);
}

// SecurityAudit_mk_working_dir :: Makes the random working dir (usually in tmp)
char SecurityAudit_mk_working_dir(char* dir_name)
{
	int ret = mkdir(dir_name, 0700);
	if (ret == 0) {
		tmpDir = strdup(dir_name);
		return 1;
	}
	return 0;
}

// SecurityAudit_rmdir :: Removes a directory, presumably the one created above
char SecurityAudit_rmdir(char* dir_name)
{
	if (tmpDir != NULL) {
		int ret;
		ret = safe_exec("/bin/rm", "rm", "-r", tmpDir, 0);
		free(tmpDir);
		tmpDir = NULL;
		return (ret == 0) ? 1 : 0;
	}
	return 1;
}

// SecurityAudit_get_md5 :: Returns the md5hash of an arbitrary file, using an ext. program
char* SecurityAudit_get_md5(char* prog, char* file)
{
	int in, pid;
	FILE *exec_pipe;
	
	// Execute program, getting a pipe to the process
	pid = safe_exec_pipe(&in, prog, "md5sum", file, 0);
	if (pid < 0) {
		fprintf(stderr, "%s:%d:Failed to execute md5sum\n", __FILE__, __LINE__);
		return "<error>1\nFailed to execute md5sum";
	}
	
	// Wrap the pipe in a stream
	exec_pipe = fdopen(in, "r");
	if (exec_pipe == NULL) {
		fprintf(stderr, "%s:%d:Failed to open pipe as stream\n", __FILE__, __LINE__);
		return "<error>2\nFailed to open pipe as stream";
	}

	// Grab a line
	if (fgets(line, LINE_LENGTH, exec_pipe)) {
		strtok(line, " "); // All I want is the hash, not the whole line
		wait(NULL);
		return line;
	}
	return "<error>3\nFailed to get md5sum";
}

// SecurityAudit_run_aide :: Runs aide, returns raw output
char *SecurityAudit_run_aide(char *prog, char *aide_conf, char *working_dir)
{
	int in, pid;
	FILE *exec_pipe;

	clear_buffer(&buf);

	// The aide program outputs a file into the current directory, so move to 
	// where we are expected to be
	if (chdir(working_dir) == -1) {
		fprintf(stderr, "%s:%d:Failed to change to requested directory: %s\n", 
						__FILE__, __LINE__, working_dir);
		return "<error>6\nFailed to change to requested directory";
	}

	// The aide program also expects to find the old db file in the current directory, 
	// so we symlink it in.
	if (symlink(TORRES_BASE "db/aide.db", "./aide.db") == -1) {
		fprintf(stderr, "%s:%d:Failed to create symlink to aide db\n", 
						__FILE__, __LINE__);
		return "<error>7\nFailed to create symlink to aide db";
	}
	
	// Execute program, getting a pipe to the process
	pid = safe_exec_pipe(&in, prog, "aide", "-c", aide_conf, 0);
	if (pid < 0) {
		fprintf(stderr, "%s:%d:Failed to execute aide\n", __FILE__, __LINE__);
		return "<error>1\nFailed to execute aide";
	}
	
	// Wrap the pipe in a stream
	exec_pipe = fdopen(in, "r");
	if (exec_pipe == NULL) {
		fprintf(stderr, "%s:%d:Failed to open pipe as stream\n", __FILE__, __LINE__);
		return "<error>2\nFailed to open pipe as stream";
	}

	// Grab data then reap process
	while (fgets(line, LINE_LENGTH, exec_pipe)) {
		add_line(&buf, line);
	}
	wait(NULL);

	// Generate new db
	if (safe_exec(prog, "aide", "-c", aide_conf, "--init", 0) != 0) {
		fprintf(stderr, "%s:%d:Failed while creating new db\n", __FILE__, __LINE__);
		add_line(&buf,"\n\n<error>8\nFailed while creating new db");
		return buf.buffer;
	}

	// Copy new db over old one
	if (file_copy(TORRES_BASE "db/aide.db.new", TORRES_BASE "db/aide.db") != 0) {
		fprintf(stderr, "%s:%d:Failed to move new db to proper location\n", __FILE__, __LINE__);
		add_line(&buf, "\n\n<error>9\nFailed to move new db to proper location");
		return buf.buffer;
	}
	
	return buf.buffer;
}

// SecurityAudit_get_new_xuids :: Runs 'xuids', returns any that weren't there last time
char *SecurityAudit_get_new_xuids(char *prog)
{
	return db_diff(prog, "xuids", TORRES_BASE "db/xuids.db");
}

// SecurityAudit_get_new_dot_files :: Runs 'dotfiles', returns any that weren't there last time
char *SecurityAudit_get_new_dot_files(char *prog)
{
	return db_diff(prog, "dotfiles", TORRES_BASE "db/dotfiles.db");
}

// SecurityAudit_get_passwordless :: Runs 'passwordless', returns any passwordless unix accounts
char *SecurityAudit_get_passwordless(char *prog)
{
	int in, pid;
	FILE *exec_pipe;
	int ret_wait, ret_val;

	clear_buffer(&buf);

	// Execute program, getting a pipe to the process
	pid = safe_exec_pipe(&in, prog, "passwordless", 0);
	if (pid < 0) {
		fprintf(stderr, "%s:%d:Failed to execute 'passwordless'\n", __FILE__, __LINE__);
		return "<error>1\nFailed to execute command";
	}

	// Wrap the pipe in a stream 
	exec_pipe = fdopen(in, "r");
	if (exec_pipe == NULL) {
		fprintf(stderr, "%s:%d:Failed to open pipe as stream\n", __FILE__, __LINE__);
		return "<error>2\nFailed to open pipe as stream";
	}
		
	// Grab data into a buffer then reap process
	while (fgets(line, LINE_LENGTH, exec_pipe)) {
		add_line(&buf, line);
	}
	ret_wait = wait(&ret_val);
	if (ret_wait == -1 || WEXITSTATUS(ret_val) != 0) {
		fprintf(stderr, "%s:%d:Failed to execute command\n", __FILE__, __LINE__);
		return "<error>1\nFailed to execute command";
	}

	return buf.buffer;
}

// SecurityAudit_get_uid_zeros :: Returns number of zero uid unix accounts
int SecurityAudit_get_uid_zeros()
{
	FILE *passwd;
	int num_ids = 0;

	// Open the passwd file for reading
	if (!(passwd = fopen("/etc/passwd", "r"))) {
		fprintf(stderr, "%s:%d:Failed opening passwd file\n", __FILE__, __LINE__);
		return -1;
	}

	// Loop
	while (fgets(line, LINE_LENGTH, passwd) != NULL) {
		if (strtok(line, ":") && strtok(NULL, ":") && 
			strcmp((char *)strtok(NULL, ":"), "0") == 0)
			num_ids++;
	}

	fclose(passwd);
	return num_ids;
}



/*------------------------------
  Util Functions
------------------------------*/

// safe_exec :: Forks and executes a program
int safe_exec(char *path, ...)
{
   	int pid;
	va_list ap;
	int num_args = 0;
	char *a[11];

	// Collect arguments
	va_start(ap, path);
	while(num_args < 10 && (a[num_args] = va_arg(ap, char *)))
		num_args++;
	va_end(ap);
	
	// Make certain it's null terminated even if we exceed the arg limit
	a[11] = 0;
	
	// Fork
	pid = fork();
	if (pid < 0) { // Failed
		fprintf(stderr, "%s:%s: Failed to fork. %s\n", 
				"__FILE__", "__LINE__", strerror(errno));
		return -1;
	}
	else if (pid > 0) { // Parent
		int ret_val;
		int ret_wait = wait(&ret_val);
		return (ret_wait != -1) ? WEXITSTATUS(ret_val) : -1;
	}
	else { // Child
		close(1); // close stdout (we are a suvlet)
		if (execv(path, a)) {
			fprintf(stderr, "%s:%s: Failed to exec %s. %s\n", 
					"__FILE__", "__LINE__", path, strerror(errno));
			exit(1);
		}
	}
	return -1; // We should never get here, makes the compiler happy
}


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
		fprintf(stderr, "%s:%s: Failed to create pipe. %s\n", 
				"__FILE__", "__LINE__", strerror(errno));
		return -1;
	}

	// Fork
	pid = fork();
	if (pid < 0) { // Failed
		close(exec_pipe[LOCAL]);
		close(exec_pipe[REMOTE]);
		fprintf(stderr, "%s:%s: Failed to fork. %s\n", 
				"__FILE__", "__LINE__", strerror(errno));
		return(-1);
	}
	else if (pid > 0) { // Parent
		// Set stdout to caller:
		close(exec_pipe[REMOTE]);

		// Return file desc of pipe to child
		*out = exec_pipe[LOCAL];
	}
	else { // Child
		close(exec_pipe[LOCAL]);
		dup2(exec_pipe[REMOTE], REMOTE);
		close(exec_pipe[REMOTE]);

		// Exec command
		if (execv(path, args)) {
			fprintf(stderr, "%s:%s: Failed to exec %s. %s\n", 
					"__FILE__", "__LINE__", path, strerror(errno));
			exit(1);
		}
	}
	return pid;
}
#undef LOCAL
#undef REMOTE

// add_line :: Adds a string to a buffer
void add_line(struct buffer_desc* buf_desc, char *line)
{
	int line_len;

	if (buf_desc->superseded == 1)
		return;

	line_len = strlen(line);
	while ((buf_desc->buffer_pos + line_len) >= (buf_desc->buffer_size + 1)) // One for null
	{
		char *tmp_ptr;
		// Resize buffer
		buf_desc->buffer_size *= 2;
		if (buf_desc->buffer_size < LINE_LENGTH)
			buf_desc->buffer_size = LINE_LENGTH;
		tmp_ptr = (char *)realloc(buf_desc->buffer, buf_desc->buffer_size);
		if (tmp_ptr == NULL) {
			fprintf(stderr, "Agggghhh! No more core! Resetting buffer\n");
			free(tmp_ptr);
			buf_desc->buffer = malloc(LINE_LENGTH);
			if (buf_desc->buffer == 0)
				exit(1);
			clear_buffer(buf_desc);
			add_line(buf_desc, "<error>256\nRan out of memory");
			buf_desc->superseded = 1;
		}
		else {
			buf_desc->buffer = tmp_ptr;
		}
	}
	strcpy(buf_desc->buffer + buf_desc->buffer_pos, line);
	buf_desc->buffer_pos += line_len;

	// We don't want messages longer than 256k
	if (buf_desc->buffer_size > 262144) {
		char *message = "\n\n(Message too long, snip)\n";
		int mess_len;

		buf_desc->superseded = 1;
		mess_len = strlen(message);
		strcpy(buf_desc->buffer + buf_desc->buffer_pos - mess_len, message);
	}

	*(buf_desc->buffer + buf_desc->buffer_pos + 1) = '\0';
}

// clear_buffer :: Resets a buffer, does not free it
void clear_buffer(struct buffer_desc* buf_desc)
{
	buf_desc->buffer_pos = 0;
	*(buf_desc->buffer) = '\0';
	buf_desc->superseded = 0;
}

// get_match :: Returns whether a string exists in a file
int get_match(char *needle, FILE *haystack)
{
	static char buffer[LINE_LENGTH];
	long start_pos = 0;

	for (;;) {
		if (fgets(buffer, LINE_LENGTH, haystack) == NULL) {

			// If we are at the beginning, then this is an empty file, exit
			if (ftell(haystack) == start_pos)
				return 0;

			// Rewind
			fseek(haystack, 0, SEEK_SET);
		}
		else {
			// Kill return
			int last_char = strlen(buffer) - 1;
			if (buffer[last_char] == '\n')
				buffer[last_char] = '\0';

			// Have we found it?
			if (strncmp(buffer, needle, LINE_LENGTH) == 0)
				return 1;

			// Have we been here before?
			if (ftell(haystack) == start_pos)
				return 0;

			if (start_pos == 0)
				start_pos = ftell(haystack);
		}
	}
}

// db_diff :: Runs a program, compares the results to an old file and returns anything new
char *db_diff(char *prog, char *prog_name, char *db_file)
{
	int in, pid;
	FILE *exec_pipe;
	FILE *old_db;
	char *needle;
	char *split_cpy;
	int ret_val, ret_wait;
	
	clear_buffer(&buf);
	clear_buffer(&buf2);

	// Execute program, getting a pipe to the process
	pid = safe_exec_pipe(&in, prog, prog_name, 0);
	if (pid < 0) {
		fprintf(stderr, "%s:%d:Failed to execute %s\n", __FILE__, __LINE__, prog_name);
		return "<error>1\nFailed to execute command";
	}

	// Open pipe as stream
	exec_pipe = fdopen(in, "r");
	if (exec_pipe == NULL) {
		fprintf(stderr, "%s:%d:Failed to open pipe as stream\n", __FILE__, __LINE__);
		return "<error>2\nFailed to open pipe as stream";
	}
		
	// Grab data then reap process
	while (fgets(line, LINE_LENGTH, exec_pipe)) {
		// Sort out lines we don't want
		if (strncmp("find: proc", line, 10) == 0) continue;
		// Add to buffer
		add_line(&buf2, line);
	}
	ret_wait = wait(&ret_val);
	if (ret_wait == -1 || WEXITSTATUS(ret_val) != 0) {
		fprintf(stderr, "%s:%d:Failed to execute %s\n", __FILE__, __LINE__, prog_name);
		return "<error>1\nFailed to execute command";
	}
	
	// Open the old data file for reading
	if (!(old_db = fopen(db_file, "r"))) {
		fprintf(stderr, "%s:%d:Failed opening db file for reading\n", __FILE__, __LINE__);
		return "<error>4\nFailed opening db file for reading";
	}

	// Loop, comparing the new to old
	split_cpy = (char *)strdup(buf2.buffer);
	needle = (char *)strtok(split_cpy, "\n");
	while (needle != NULL) {
		if (!get_match(needle, old_db)) {
			// If it isn't in the old one, add to the buffer we're going to return
			add_line(&buf, needle);
			add_line(&buf, "\n");
		}
		needle = (char *)strtok(NULL, "\n");
	}

	// Clean up
	free(split_cpy);
	fclose(old_db);

	// Copy the new results over the old db
	// -- Open the old data file for writing
	if (!(old_db = fopen(db_file, "w"))) {
		fprintf(stderr, "%s:%d:Failed opening db file for writing\n", __FILE__, __LINE__);
		return "<error>5\nFailed opening db file for writing";
	}

	// -- Dump the buffer to file
	fprintf(old_db, buf2.buffer);
	fclose(old_db);

	return buf.buffer;
}


// file_copy :: Super lame file copying
int file_copy(char *from, char *to)
{
	FILE *in;
	FILE *out;
	char copy_buffer[COPY_BUFFER_SIZE];
	int data_len;

	in = fopen(from, "r");
	if (in == NULL) {
		fprintf(stderr, "Failed opening input file: %s : %s\n", from, strerror(errno));
		return 1;
	}

	out = fopen(to, "w");
	if (out == NULL) {
		fprintf(stderr, "Failed opening output file: %s\n", strerror(errno));
		return 1;
	}

	// Loop & copy 
	while ((data_len = fread(copy_buffer, 1, COPY_BUFFER_SIZE, in))) {
		if (fwrite(copy_buffer, 1, data_len, out) < data_len) {
			fprintf(stderr, "Failed while writing to output file: %s\n", strerror(errno));
			return 1;
		}
	}

	// Close files
	fclose(in);
	fclose(out);

	return 0;
}


