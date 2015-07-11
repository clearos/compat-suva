#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "suva_util.h"

void fatal(int err_no, char *text)
{
	fprintf(stderr, "Error: %s", text);
	exit(err_no);
}

#if 0
// Wrapper for write
int scl_fwrite(int fd, void *data, int data_len)
{
	int data_written;
	while (data_len > 0)
	{
		data_written = write(fd, data, data_len);
		if (data_written == -1)
		{
			if (errno == EAGAIN) 
				usleep(256);
			else 
				return 1;
		}
		else if (data_written == 0)
			return 1;
		else
		{
			data_len -= data_written;
			data += data_written;
		}
	}
	return 0;
}

// Wrapper for read
ssize_t scl_fread(int fd, void *data, int data_len)
{
	ssize_t data_read;

	while (data_len > 0)
	{
		data_read = read(fd, data, data_len);
		if (data_read == -1)
		{
			if (errno == EAGAIN) 
				usleep(256);
			else
				return data_read;
		}
		else if (data_read == 0)
			return data_read;
		else
		{
			data_len -= data_read;
			data += data_read;
		}
	}
	return data_read;
}
#endif
