// Suva utility (general-purpose) support routines
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// $Id: sutil.c,v 1.31 2004/03/25 20:43:14 darryl Exp $
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <syslog.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#include "suva_conf.h"
#include "suva.h"
#include "sutil.h"
#include "mem.h"

extern int errno;

//! External SuvaConfig structure
extern struct SuvaConfig_t SuvaConfig;

//! Static output buffer
static char output_buffer[MAX_BUFFER];

int pid_lock(pid_t pid)
{
	int fd;
	struct flock pidlock;
	char pidfile[MAX_PATH];

	sprintf(pidfile, "%s%s%d", SD_BASE, SF_PIDLOCK, pid);

	if((fd = open(pidfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
	{
		output(1, "%s: open(%s): %s.", __func__, pidfile, strerror(errno));

		return -1;
	}

	memset(&pidlock, '\0', sizeof(struct flock));

	pidlock.l_type = F_WRLCK;

	if(fcntl(fd, F_SETLK, &pidlock) < 0)
	{
		output(1, "%s: fcntl(%s): %s.", __func__, pidfile, strerror(errno));
		close(fd);
		unlink(pidfile);

		return -1;
	}

	if(write(fd, &pid, sizeof(pid)) != sizeof(pid))
	{
		output(1, "%s: write(%s, %d): %s.", __func__, pidfile, pid, strerror(errno));
		close(fd);
		unlink(pidfile);

		return -1;
	}

	output(5, "%s: locked %s.", __func__, pidfile);

	return fd;
}

void pid_unlock(pid_t pid)
{
	char pidfile[MAX_PATH];

	sprintf(pidfile, "%s%s%d", SD_BASE, SF_PIDLOCK, pid);

	if(unlink(pidfile) < 0)
		output(1, "%s: unlink(%s): %s.", __func__, pidfile, strerror(errno));
	else
		output(5, "%s: un-locked %s.", __func__, pidfile);
}

int pid_status(pid_t pid)
{
	int fd;
	struct stat pidstat;
	struct flock pidlock;
	char pidfile[MAX_PATH];

	sprintf(pidfile, "%s%s%d", SD_BASE, SF_PIDLOCK, pid);

	if((fd = open(pidfile, O_RDWR)) < 0)
	{
		if(errno == ENOENT)
			return PID_STALE;

		output(1, "%s: open(%s): %s.", __func__, pidfile, strerror(errno));
		return -1;
	}

	memset(&pidlock, '\0', sizeof(struct flock));

	pidlock.l_type = F_WRLCK;

	if(fcntl(fd, F_GETLK, &pidlock) < 0)
	{
		output(1, "%s: fcntl(%s): %s.", __func__, pidfile, strerror(errno));
		memset(&pidlock, '\0', sizeof(struct flock));
	}

	if(fstat(fd, &pidstat) < 0)
	{
		output(1, "%s: fstat(%s): %s.\n", __func__, pidfile, strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	if(pidlock.l_pid)
		return PID_RUN;

	return PID_STALE;
}

//#if defined SCO_PACKET_DUMP || defined SCO_SUVA_DEBUG
void print_hex(char *header, unsigned char *s, int n)
{
	int i, j, k, p, o, r = 0, fd;
	unsigned long offset = (unsigned long)s;
	char line_this[70], line_last[70], buffer[20], s_offset[12], log[MAX_PATH];
#ifdef SCO_EMBED
	sprintf(log, "/mnt/tmp/suva-debug.%d", getpid());
#else
	sprintf(log, "/tmp/suva-debug.%d", getpid());
#endif
	if((fd = open(log, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
	{
		output(1, "Couldn't open debug log: %s.", log);
		return;
	}

	if(header)
	{
		write(fd, header, strlen(header));
		write(fd, "\n", 1);
	}

	memset(line_last, '\0', 70);

	for(i = 0; i < n; i += j)
	{
		k = 0; p = 0; o = 0;

		memset(line_this, '\0', 70);

		sprintf(s_offset, "%08lX  ", offset);

		for(j = i; j < (i + 16) && j < n; j++)
		{
			sprintf(buffer, "%02X ", s[j]);
			strcat(line_this, buffer);

			if(++k == 4)
			{
				o = (o) ? 0 : 1;
				strcat(line_this, " ");
				k = 0;
				p++;
			}
		}

		for(o = 16 - (j - i); o > 0; o--)
			strcat(line_this, "   ");

		for(o = 0; o < 4 - p; o++)
			strcat(line_this, " ");

		for(j = i; j < (i + 16) && j < n; j++)
		{
			sprintf(buffer, "%c", (isprint(s[j])) ? s[j] : '.');
			strcat(line_this, buffer);
		}
#ifndef SCO_EMBED
		if(strcmp(line_this, line_last) == 0)
			r++;
		else
		{
			if(r)
			{
				output(1, "Last line repeated %d times.");
				r = 0;
			}
#endif
			//output(1, "%s%s", s_offset, line_this);
			write(fd, s_offset, strlen(s_offset));
			write(fd, line_this, strlen(line_this));
			write(fd, "\n", 1);
#ifndef SCO_EMBED
			strcpy(line_last, line_this);
		}
#endif			
		j -= i;
		offset += 16;
	}

	write(fd, "\n", 1);
	close(fd);
}
//#endif

uid_t get_uid(const char *user_id)
{
#ifndef SCO_EMBED
	struct passwd *p_pw = NULL;

	if(!user_id)
		return -1;

	if(isdigit(user_id[0]))
		p_pw = getpwuid(atoi(user_id));

	if(!p_pw && !(p_pw = getpwnam(user_id)))
		return -1;

	return p_pw->pw_uid;
#else
	return 0;
#endif
}

gid_t get_gid(const char *group_id)
{
#ifndef SCO_EMBED
	struct group *p_gr = NULL;
	struct passwd *p_pw = NULL;

	if(!group_id)
		return -1;

	if(isdigit(group_id[0]))
		p_pw = getpwuid(atoi(group_id));
	
	if(!p_pw && !(p_pw = getpwnam(group_id)))
		return -1;

	if(isdigit(group_id[0]))
		p_gr = getgrgid(atoi(group_id));

	if(!p_gr && !(p_gr = getgrnam(group_id)))
		return -1;

	// Will cause a 'comparison between signed and unsigned' warning:
	if(p_pw->pw_gid != p_gr->gr_gid)
		return -1;

	return p_pw->pw_gid;
#else
	return 0;
#endif
}

char **arg_array(char **array, const char *arg)
{
	int q = 0;
	char *arg_start, *arg_end, *arg_tmp;

	arg_start = arg_end = arg_tmp = strdup(arg);

	while(arg_start[0])
	{
		if(arg_start[0] == ' ')
		{
			arg_end = ++arg_start;
			continue;
		}

		if(arg_start[0] == '"')
		{
			q = 1;
			arg_end = ++arg_start;
			continue;
		}

		if(((arg_end[0] == ' ' || !arg_end[0]) && !q) || (arg_end[0] == '"' && q))
		{
			if(!arg_end[0])
			{
				array = array_push(array, arg_start);
				break;
			}

			q = 0;
			arg_end[0] = '\0';
			array = array_push(array, arg_start);
			arg_start = ++arg_end;

			continue;
		}

		arg_end++;
	}

	free(arg_tmp);

	return array;
}

char **dir_array(char **array, const char *dir)
{
	char *dir_start, *dir_end, *dir_tmp, buffer[MAX_BUFFER];

	buffer[0] = '/'; 
	dir_start = dir_end = dir_tmp = strdup(dir);

	while(dir_start[0])
	{
		if(dir_start[0] == ' ')
		{
			dir_end = ++dir_start;
			continue;
		}

		if(dir_end[0] == '.' || !dir_end[0])
		{	
			buffer[1] = '\0';

			if(!dir_end[0])
			{
				strncat(buffer, dir_start,
					MAX_BUFFER - strlen(dir_start) - 2);

				array = array_push(array, buffer);
				break;
			}

			dir_end[0] = '\0';
			strncat(buffer, dir_start, MAX_BUFFER - strlen(dir_start) - 2);
			array = array_push(array, buffer);
			dir_start = ++dir_end;

			continue;
		}

		dir_end++;
	}

	free(dir_tmp);

	return array;
}

char **env_array(char **array, const char *format, ...)
{
	va_list ap;
	FILE *h_f = NULL;
	char line[MAX_BUFFER];
	char buffer[MAX_BUFFER];

	va_start(ap, format);
	vsnprintf(buffer, MAX_BUFFER, format, ap);
	va_end(ap);

	if(!(h_f = fopen(buffer, "r")))
	{
		output(1, "Unable to open environment file %s: %s.",
			buffer, strerror(errno));

		return NULL;
	}

	while(!feof(h_f))
	{
		if(!fgets(buffer, MAX_BUFFER, h_f))
			break;

		memset(line, '\0', sizeof(line));

		if(sscanf(buffer, "%1023[A-z0-9/:_=\" -]", line))
			array = array_push(array, line);
	}

	fclose(h_f);

	return array;
}

char **array_push(char **array, const char *string)
{
	register int i = 0;

	while(array && array[i])
		i++;

	array = (char **)realloc(array, (i + 2) * sizeof(char *));
	ASSERT(array);

	array[i] = strdup(string);
	array[i + 1] = '\0';

	return array;
}

char **array_reverse(char **array)
{
	int len = 0;
	char **new_array = NULL;

	while(array && array[len]) len++;

	if(!len)
		return array;

	while(--len >= 0)
		new_array = array_push(new_array, array[len]);

	array_free(array);

	return new_array;
}

void array_free(char **array)
{
	register int i;

	for(i = 0; array && array[i]; i++)
		free(array[i]);

	free(array);
}

void output(int level, const char *format, ...)
{
	va_list ap;

 	if(level > SuvaConfig.log_level && level != 100 && level != 1024)
		return;

	va_start(ap, format);
	vsnprintf(output_buffer, MAX_BUFFER, format, ap);
	va_end(ap);

	if(level < 0 && !(SuvaConfig.flags & SCF_SUVLET))
	{
		fprintf(stdout, "%s\n", output_buffer);
		return;
	}
	else if(level == 100)
	{
		int fd;
		char log[MAX_PATH];

		sprintf(log, "/tmp/suva-debug.%d", getpid());

		if((fd = open(log, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
			return;

		write(fd, output_buffer, strlen(output_buffer));
		write(fd, "\n", 1);

		close(fd);
		return;
	}
	else if(level == 1024 && !(SuvaConfig.flags & SCF_SUVLET))
	{
		if((SuvaConfig.flags & SCF_QUIET))
			return;

		fprintf(stdout, "%s\n", output_buffer);
		return;
	}

	if((SuvaConfig.flags & SCF_DEBUG))
	{
		if((SuvaConfig.flags & SCF_SUVAD))
		{
			fprintf(stderr, "[%d] %s\n", getpid(), output_buffer);
			return;
		}

		fprintf(stderr, "%s\n", output_buffer);
	}
	else if((SuvaConfig.flags & SCF_SUVLET))
	{
		int fd;
		char log[MAX_PATH];

		sprintf(log, "/tmp/suvlet.%d", getpid());

		if((fd = open(log, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
			return;

		write(fd, output_buffer, strlen(output_buffer));
		write(fd, "\n", 1);

		close(fd);
//		syslog(SuvaConfig.log_facility | LOG_NOTICE, output_buffer);
		return;
	}
	else
		syslog(SuvaConfig.log_facility | LOG_NOTICE, output_buffer);
}

char *get_output(void)
{
	char *s;

	if((s = memchr(output_buffer, '\n', MAX_BUFFER - 1)))
		*s = '\0';

	return output_buffer;
}

unsigned long get_ifip(char *ifname)
{
	int s;
	struct ifreq ifr;
	struct sockaddr_in addr;

	if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return 0;

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

	if(ioctl(s, SIOCGIFADDR, &ifr) < 0)
	{
		close(s);

		return 0;
	}

	close(s);

	addr = *((struct sockaddr_in *)&ifr.ifr_addr);

	return addr.sin_addr.s_addr;
}

#define SWAP(n) ((n << 24) | ((n & 65280) << 8) | ((n & 16711680) >> 8) | (n >> 24))

void swap(void *src, size_t len)
{
	register unsigned char *a, *b, tmp;

	a = (unsigned char *)src;
	b = a + len;

	while(a < b)
	{
		tmp = *a; *a++ = *--b; *b = tmp;
	}
}

char *legal_name(char *name, int len)
{
	int i, j = 0, c, l = 0;
	char *new_name = malloc(len);

	if(!new_name)
		return NULL;

	memset(new_name, '\0', len);

	for(i = 0; i < len; i++, c = 0)
	{
		if(isalnum(name[i]))
			c = name[i];
		else if(name[i] == '_')
			c = name[i];
		else if(name[i] == '.' && l != '.')
			c = name[i];
		else if(!name[i])
			break;

		if(c)
		{
			l = c;
			new_name[j++] = c;
		}
	}

	memcpy(name, new_name, len);
	free(new_name);

	return name;
}

char *legal_string(char *name, int len)
{
	int i, j = 0, c, l = 0;
	char *new_name = malloc(len);

	if(!new_name)
		return NULL;

	memset(new_name, '\0', len);

	for(i = 0; i < len; i++, c = 0)
	{
		if(isalnum(name[i]))
			c = name[i];
		else if(name[i] == '_')
			c = name[i];
		else if(name[i] == '-')
			c = name[i];
		else if(name[i] == '.' && l != '.')
			c = name[i];
		else if(name[i] == ' ' && l != ' ')
			c = name[i];
		else if(!name[i])
			break;

		if(c)
		{
			l = c;
			new_name[j++] = c;
		}
	}

	memcpy(name, new_name, len);
	free(new_name);

	return name;
}
