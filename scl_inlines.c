#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>

#include "suva.h"
#include "suva_conf.h"
#include "mem.h"
#include "suva_api.h"
#include "suva_util.h"

// Note:  The local side of this is a gross hack job.  I recommend a rewrite to 
//		  java and/or at least separate the scl code out.  hj
// Also, an astute observer would note that I have wildly varying return codes from the
// remote side of things.  I was on crack.  Also it was my second C program.  Sorry :|


// This var is located in the suva_receiver.c file
extern int suvlet_version;

////\\\////\\\\////\\\\////\\\\////\\\\////\\\\\
// Remote functions
////\\\////\\\\////\\\\////\\\\////\\\\////\\\\\

void scl_call_inlines(struct suva_conn *conn, char *func_name)
{
	if (strcmp(func_name, "inline-open_file") == 0)
		scl_il_r_open_file(conn);
	else if (strcmp(func_name, "inline-close_file") == 0)
		scl_il_r_close_file(conn);
	else if (strcmp(func_name, "inline-get_data") == 0)
		scl_il_r_get_data(conn);
	else if (strcmp(func_name, "inline-put_data") == 0)
		scl_il_r_put_data(conn);
	else if (strcmp(func_name, "inline-get_info") == 0)
		scl_il_r_get_info(conn);
	else if (strcmp(func_name, "inline-get_fsize_mode") == 0)
		scl_il_r_get_fsize_mode(conn);
	else if (strcmp(func_name, "inline-set_fmode") == 0)
		scl_il_r_set_fmode(conn);
}

void scl_throw_error(struct suva_conn *conn, int err_no, char* err_text)
{
   	conn->out_packet_opcode = SCL_FERROR;
	scl_queue_value(conn, &err_no, SCL_INT, 1);
	scl_queue_value(conn, err_text, SCL_STRING, 1);
	scl_write_out(conn);

	exit(1);
}

void scl_il_r_open_file(struct suva_conn *conn)
{
	char *file_name;
	char *mode;
	int zero = 0, one = 1;

	file_name = (char *)scl_get_value(conn, SCL_STRING, NULL);
	mode = (char *)scl_get_value(conn, SCL_STRING, NULL);
	
	scl_inline_file = fopen(file_name, mode);
	scl_free_packet_mem(conn);

	// If we failed to open the file, send back the errno from that request
	conn->out_packet_opcode = SCL_FRETURN;
	if (scl_inline_file != 0)
	{
		scl_queue_value(conn, &zero, SCL_INT, 1);
		scl_queue_value(conn, NULL, SCL_STRING, 1);
	}
	else
	{
		scl_queue_value(conn, &one, SCL_INT, 1);
		scl_queue_value(conn, (void *)strerror(errno), SCL_STRING, 1);
	}
	scl_write_out(conn);
}

void scl_il_r_close_file(struct suva_conn *conn)
{
	fflush(scl_inline_file);
	fclose(scl_inline_file);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_write_out(conn);
}

void scl_il_r_put_data(struct suva_conn *conn)
{	
	void *data;
	int data_len;
	char write_success;
	char ret_val;

	data = scl_get_value(conn, SCL_DATA, &data_len);
	write_success = (fwrite(data, data_len, 1, scl_inline_file)) ? 1 : 0;

	if (write_success == 0)
		ret_val = 1;
	else
		ret_val = 0;
		
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &ret_val, SCL_BOOL, 1);
	scl_write_out(conn);
}

void scl_il_r_get_data(struct suva_conn *conn)
{	
	void *data;
	int data_len;
	int bytes;

	data_len = *(int *)scl_get_value(conn, SCL_INT, NULL);
	scl_free_packet_mem(conn);

	data = malloc(data_len);
	
	conn->out_packet_opcode = SCL_FRETURN;

	bytes = fread(data, 1, data_len, scl_inline_file);

	if (bytes)
		scl_queue_value(conn, data, SCL_DATA, bytes);
	else
		scl_queue_value(conn, NULL, SCL_DATA, 0);
	scl_write_out(conn);
	free(data);
}

// Note:  This function is non-portable
void scl_il_r_get_fsize_mode(struct suva_conn *conn)
{
	char *file_name;
	struct stat file_stat;
	int file_size;
	int file_mode;

	file_name = (char *)scl_get_value(conn, SCL_STRING, NULL);

	if (stat(file_name, &file_stat) == -1)
	{
		file_size = file_mode = -1;
	}
	else
	{
		file_size = file_stat.st_size;
		file_mode = file_stat.st_mode & 0xfff;
	}

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &file_size, SCL_INT, 0);
	scl_queue_value(conn, &file_mode, SCL_INT, 0);
	scl_write_out(conn);
	scl_free_packet_mem(conn);
}

void scl_il_r_set_fmode(struct suva_conn *conn)
{
	char *file_name;
	int file_mode;
	int ret_val;

	file_name = (char *)scl_get_value(conn, SCL_STRING, NULL);
	file_mode = *(int *)scl_get_value(conn, SCL_INT, NULL);

	ret_val = (chmod(file_name, file_mode) == 0) ? 1 : 0;

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &ret_val, SCL_BOOL, 0);
	scl_write_out(conn);
	scl_free_packet_mem(conn);
}


// serves get_suvlet_version, cur_dir and pid
void scl_il_r_get_info(struct suva_conn *conn)
{
	char *var;

	var = (char *)scl_get_value(conn, SCL_STRING, NULL);

	conn->out_packet_opcode = SCL_FRETURN;
	if (strcmp(var, "suvlet_version") == 0)
	{
		scl_queue_value(conn, &suvlet_version, SCL_INT, 0);
		scl_write_out(conn);
	}
	else if (strcmp(var, "api_version") == 0)
	{
		int api_version = SUVA_API_VERSION;
		scl_queue_value(conn, &api_version, SCL_INT, 0);
		scl_write_out(conn);
	}
	else if (strcmp(var, "cwd") == 0)
	{
		char *dir;
		dir = (char *)malloc(MAXPATHLEN + 1);
		getcwd(dir, MAXPATHLEN);

		scl_queue_value(conn, dir, SCL_STRING, 0);	
		scl_write_out(conn);
		free(dir);
	}
	else if (strcmp(var, "suva_base") == 0)
	{
		scl_queue_value(conn, SD_BASE, SCL_STRING, 0);	
		scl_write_out(conn);
	}
	else if (strcmp(var, "pid") == 0)
	{
		int pid;
		pid = getpid();
		scl_queue_value(conn, &pid, SCL_INT, 0);
		scl_write_out(conn);
	}
	scl_free_packet_mem(conn);
}


////\\\////\\\\////\\\\////\\\\////\\\\////\\\\\
// Local functions
////\\\////\\\\////\\\\////\\\\////\\\\////\\\\\

int scl_il_get_suvlet_version(struct suva_conn *conn)
{
	int retval;

	conn->out_packet_opcode = SCL_FCALL;
	scl_queue_value(conn, "inline-get_info", SCL_STRING, 1);
	scl_queue_value(conn, "suvlet_version", SCL_STRING, 1);
	scl_write_out(conn);

	scl_read_packet(conn);
	retval = *(int *)scl_get_value(conn, SCL_INT, NULL);
	scl_free_packet_mem(conn);

	return retval;
}

char *scl_il_get_cwd(struct suva_conn *conn)
{
	char *retval;
	char *dir;
	int dir_len;

	conn->out_packet_opcode = SCL_FCALL;
	scl_queue_value(conn, "inline-get_info", SCL_STRING, 1);
	scl_queue_value(conn, "cwd", SCL_STRING, 1);
	scl_write_out(conn);

	scl_read_packet(conn);
	dir = (char *)scl_get_value(conn, SCL_STRING, NULL);
	dir_len = strlen(dir);

	retval = (char *)malloc(dir_len + 1);
	memset(retval, 0, dir_len + 1);
	memcpy(retval, dir, dir_len);
	scl_free_packet_mem(conn);

	return retval;
}

char *scl_il_get_suva_base(struct suva_conn *conn)
{
	char *retval;
	char *dir;
	int dir_len;

	conn->out_packet_opcode = SCL_FCALL;
	scl_queue_value(conn, "inline-get_info", SCL_STRING, 1);
	scl_queue_value(conn, "suva_base", SCL_STRING, 1);
	scl_write_out(conn);

	scl_read_packet(conn);
	dir = (char *)scl_get_value(conn, SCL_STRING, NULL);
	dir_len = strlen(dir);

	retval = (char *)malloc(dir_len + 1);
	memset(retval, 0, dir_len + 1);
	memcpy(retval, dir, dir_len);
	scl_free_packet_mem(conn);

	return retval;
}

int scl_il_get_pid(struct suva_conn *conn)
{
	int retval;

	conn->out_packet_opcode = SCL_FCALL;
	scl_queue_value(conn, "inline-get_info", SCL_STRING, 1);
	scl_queue_value(conn, "pid", SCL_STRING, 1);
	scl_write_out(conn);

	scl_read_packet(conn);
	retval = *(int *)scl_get_value(conn, SCL_INT, NULL);
	scl_free_packet_mem(conn);

	return retval;
}

int scl_il_get_api_version(struct suva_conn *conn)
{
	int retval;

	conn->out_packet_opcode = SCL_FCALL;
	scl_queue_value(conn, "inline-get_info", SCL_STRING, 1);
	scl_queue_value(conn, "api_version", SCL_STRING, 1);
	scl_write_out(conn);

	scl_read_packet(conn);
	retval = *(int *)scl_get_value(conn, SCL_INT, NULL);
	scl_free_packet_mem(conn);

	return retval;
}

#define scl_il_xfer_exitm(a, b, c, d) scl_il_xfer_exit(a, b, c, d, conn, file_handle, data, direction)

int scl_il_xfer(	struct suva_conn *conn, char *local_file_name, 
							char *remote_file_name, int direction)
{
	return scl_il_xfer_ex(	conn, local_file_name, remote_file_name, direction,
									SCL_IL_XFER_DEFAULT_PSIZE, NULL, 0);
}

int scl_il_xfer_ex(	struct suva_conn *conn,  
								char *local_file_name, 
								char *remote_file_name,
								int direction,
								int block_size,
								int (*scl_copy_prog_callback)(struct scl_copy_progress *copy_prog),
								int callback_interval)
{
	FILE *file_handle = NULL;
	void *data = NULL;
	int data_len;
	char return_code;
	int interval_count;
	int bytes_total = 0;
	int bytes_transfered = 0;
	int prev_bytes_transfered = 0;
	int time_sec_offset;
	int time_usec_offset;
	int bps_avgs[SCL_IL_XFER_NUM_BPS_AVGS];
	int bps_avg_index = 0;
	int api_vers;
	int local_mode = -1;
	int remote_mode = -1;
	struct stat local_file_stat;
	struct timeval ntp;
	struct timezone tz;
	int prev_msec;
	int cur_msec = 0;
	
	// 'data' and 'data_len' do a double duty, if this is a put, then they refer to
	// the buffer I allocate right here, if not then they are used as arbitrary storage
	// This is a confusing result of the function merger.  But it works, and I want this
	// out the door, so it's a TODO
	if (direction == SCL_IL_XFER_PUT)
		data = malloc(block_size);

	interval_count = callback_interval;
	memset(bps_avgs, 255, sizeof(float) * SCL_IL_XFER_NUM_BPS_AVGS);

	// Get remote api version
	api_vers = scl_il_get_api_version(conn);

	// Open local file
	if (direction == SCL_IL_XFER_GET)
		file_handle = fopen(local_file_name, "w");
	else
		file_handle = fopen(local_file_name, "r");

	if (!file_handle)
	{
		fprintf(stderr, "Local File Error: %s\n", strerror(errno));
		return scl_il_xfer_exitm(1, 0, 0, 0);
	}

	// Get bytes_total and local/remote_mode
	if (direction == SCL_IL_XFER_GET)
	{	// SCL_IL_XFER_GET
		if (api_vers >= 3) // v3
		{
			conn->out_packet_opcode = SCL_FCALL;
			scl_queue_value(conn, "inline-get_fsize_mode", SCL_STRING, 1);
			scl_queue_value(conn, remote_file_name, SCL_STRING, 1);
			scl_write_out(conn);

			scl_read_packet(conn);
			bytes_total = *(int *)scl_get_value(conn, SCL_INT, NULL);
			remote_mode = *(int *)scl_get_value(conn, SCL_INT, NULL);
			scl_free_packet_mem(conn);

			if (bytes_total == -1)
				bytes_total = 0;
		}
		else
		{
			bytes_total = 0;
			remote_mode = -1;
		}
	}
	else // SCL_IL_XFER_PUT
	{
		if (stat(local_file_name, &local_file_stat) == -1)
		{
			fprintf(stderr, "Failed to retrieve info regarding local file: %s\n", strerror(errno));
			return scl_il_xfer_exitm(2, 0, 1, 0);
		}
		else
		{
			bytes_total = local_file_stat.st_size;
			local_mode = local_file_stat.st_mode;
		}
	}

	// Open remote file
	conn->out_packet_opcode = SCL_FCALL;
	scl_queue_value(conn, "inline-open_file", SCL_STRING, 1);
	scl_queue_value(conn, remote_file_name, SCL_STRING, 1);
	if (direction == SCL_IL_XFER_GET)
		scl_queue_value(conn, "r", SCL_STRING, 1);
	else
		scl_queue_value(conn, "w", SCL_STRING, 1);
	scl_write_out(conn);

	// Check to see if it worked...
	scl_read_packet(conn);
	if (conn->in_packet_opcode != SCL_FRETURN)
	{
		suva_unexpected_return(conn);
		return scl_il_xfer_exitm(3, 1, 1, 0);
	}

	return_code = *(int *)scl_get_value(conn, SCL_INT, NULL);
	if (return_code == 0)
		scl_free_packet_mem(conn);
	else
	{
		fprintf(stderr, "Remote File Error: %s\n", (char *)scl_get_value(conn, SCL_STRING, NULL));
		scl_free_packet_mem(conn);
		return scl_il_xfer_exitm(4, 0, 1, 0);
	}

	// Set up timing offsets
	if (gettimeofday(&ntp, &tz) == -1)
		return scl_il_xfer_exitm(10, 1, 1, 1);
	time_sec_offset = ntp.tv_sec;
	time_usec_offset = ntp.tv_usec;

	// Okay, send 'er over Jim
	for (;;)
	{
		// Okay, this setup is a bit weird.  There are two main chunks of code, 
		// one for get, one for put.  There are likewise two ways to jump out of
		// this endless loop.

		// 'Put' exit
		if (direction == SCL_IL_XFER_PUT)
			if ((data_len = fread(data, 1, block_size, file_handle)) == 0)
				break;
		
		// 'Put' code
		if (direction == SCL_IL_XFER_PUT)
		{
			int ret_val;
	
			bytes_transfered += data_len;
	
			// Transfer
			conn->out_packet_opcode = SCL_FCALL;
			scl_queue_value(conn, "inline-put_data", SCL_STRING, 1);
			scl_queue_value(conn, data, SCL_DATA, data_len);
			scl_write_out(conn);
	
			scl_read_packet(conn);
			if (conn->in_packet_opcode != SCL_FRETURN)
			{
				suva_unexpected_return(conn);
				return scl_il_xfer_exitm(4, 0, 1, 1);
			}
			
			ret_val = *(char *)scl_get_value(conn, SCL_BOOL, NULL);
			scl_free_packet_mem(conn);
			if (ret_val != 0)
				return scl_il_xfer_exitm(6, 0, 1, 1);
		}

		// 'Get' code
		else if (direction == SCL_IL_XFER_GET)
		{
			conn->out_packet_opcode = SCL_FCALL;
			scl_queue_value(conn, "inline-get_data", SCL_STRING, 1);
			scl_queue_value(conn, &block_size, SCL_INT, 1);
			scl_write_out(conn);
	
			scl_read_packet(conn);
			if (conn->in_packet_opcode != SCL_FRETURN)
			{
				suva_unexpected_return(conn);
				return scl_il_xfer_exitm(4, 0, 1, 1);
			}
			
			data = scl_get_value(conn, SCL_DATA, &data_len);
			if (data == NULL)
				return scl_il_xfer_exitm(5, 0, 1, 1);

			bytes_transfered += data_len;
	
			if (!(fwrite(data, data_len, 1, file_handle)))
				return scl_il_xfer_exitm(7, 1, 1, 1);
	
			scl_free_packet_mem(conn);
		} 

		// Callback code
		if (--interval_count == 0)
		{
			if (scl_copy_prog_callback != NULL)
			{
				struct scl_copy_progress copy_prog;
				int count, time_delta, avgs = 0;
				int running_total;
				
				// High resolution timing stuff
				prev_msec = cur_msec;
				if (gettimeofday(&ntp, &tz) == -1)
					return scl_il_xfer_exitm(10, 1, 1, 1);
				cur_msec =	((ntp.tv_sec - time_sec_offset) * 1000) + 
							((ntp.tv_usec - time_usec_offset) / 1000);

				// Assign basic stuff to struct
				copy_prog.bytes_total = bytes_total;
				copy_prog.bytes_transfered = bytes_transfered;
				copy_prog.msec_elapsed = cur_msec;

				// Averages & eta
				if (++bps_avg_index == SCL_IL_XFER_NUM_BPS_AVGS)
					bps_avg_index = 0;

				bps_avgs[bps_avg_index] = ((float)(bytes_transfered - prev_bytes_transfered)
											/ (float)(cur_msec - prev_msec)) * 1000;

				running_total = 0;
				for (count = 0; count < SCL_IL_XFER_NUM_BPS_AVGS; count++)
				{
					if (bps_avgs[count] != -1)
					{
						running_total += bps_avgs[count];
						avgs++;
					}
				}
				copy_prog.avg_bps = running_total / avgs;
				copy_prog.eta = (bytes_total - bytes_transfered) / copy_prog.avg_bps;

				// Callback with the struct
				if ((*scl_copy_prog_callback)(&copy_prog))
					return scl_il_xfer_exitm(8, 1, 1, 1);

				prev_bytes_transfered = bytes_transfered;
			}
			interval_count = callback_interval;
		}

		// 'Get' exit
		if (direction == SCL_IL_XFER_GET)
			if (data_len != block_size)
				break;
	}

	// Set mode on target file
	if (direction == SCL_IL_XFER_GET)
	{	 // SCL_IL_XFER_GET
		if (remote_mode != -1)
			chmod(local_file_name, remote_mode);
	}
	else //SCL_IL_XFER_PUT
	{
		if (api_vers >= 3)  // v3
		{
			int ret_val;
			conn->out_packet_opcode = SCL_FCALL;
			scl_queue_value(conn, "inline-set_fmode", SCL_STRING, 1);
			scl_queue_value(conn, remote_file_name, SCL_STRING, 1);
			scl_queue_value(conn, &local_mode, SCL_INT, 1);
			scl_write_out(conn);

			scl_read_packet(conn);
			ret_val = *(int *)scl_get_value(conn, SCL_INT, NULL);
			scl_free_packet_mem(conn);
			if (ret_val == -1)
				fprintf(stderr, "Warning: Couldn't chmod remote file \n");
		}
	}

	// Exit gracefully
	return scl_il_xfer_exitm(0, 0, 1, 1);
}

int scl_il_xfer_exit(	int exit_code, int print_errno, int kill_local_file,
							int kill_remote_file, struct suva_conn *conn, 
							FILE *file_handle, void *data, int direction)
{
	if (print_errno)
		fprintf(stderr, "Error: %s\n", strerror(errno));

	if (data != NULL && direction == SCL_IL_XFER_PUT)
		free(data);

	scl_free_packet_mem(conn);

	// Close remote file
	if (kill_remote_file)
	{
		conn->out_packet_opcode = SCL_FCALL;
		scl_queue_value(conn, "inline-close_file", SCL_STRING, 1);
		scl_write_out(conn);

		scl_read_packet(conn);

		// We don't care what it says
		scl_free_packet_mem(conn);
	}

	// Close local file
	if (kill_local_file && file_handle != NULL)
		fclose(file_handle);

	return exit_code;
}


