#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "suva_conf.h"
#include "mem.h"
#include "sutil.h"
#include "suva_api.h"

extern int errno;

static int scl_in_pheader[4];

void scl_free_packet_mem(struct suva_conn *conn)
{
	mem_free_all(conn->in_mem_id);
}

#define scl_fread scl_read

int scl_read(int sd, void *p_data, unsigned int len)
{
	size_t bytes;
	void *p_chunk = p_data;
	unsigned int length = len;
	time_t start_time = time(NULL), read_time = 0;

	while(read_time < (time_t)SCL_TIMEOUT)
	{
		if((bytes = read(sd, p_chunk, length)) == length)
			break;

		if(bytes == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				usleep(256);
				read_time = time(NULL) - start_time;
				continue;
			}

			fprintf(stderr, "%s: Unable to read %u bytes(s): %s.\n",
				__func__, length, strerror(errno));

			return -1;
		}

		if(bytes == 0)
		{
			fprintf(stderr, "%s: Remote host hung-up.\n", __func__);
			return 0;
		}

		length -= (unsigned int)bytes; p_chunk += bytes;
		start_time = time(NULL); read_time = 0;
		usleep(256);
	}

	if(read_time >= (time_t)SCL_TIMEOUT && (unsigned int)bytes != len)
	{
		fprintf(stderr, "%s: Exceeded read time-out (%d seconds).\n",
			__func__, SCL_TIMEOUT);
		return -1;
	}

	return (int)len;
}

ssize_t scl_read_packet(struct suva_conn *conn)
{
	ssize_t bytes;

	// Grab header
	bytes = scl_fread(conn->in, scl_in_pheader, SCL_INT_LEN * 4);

	if(!bytes) return 0;

	if(bytes != SCL_INT_LEN * 4) return -1;

	// Set the opcode in the conn struct
	conn->in_packet_opcode = scl_in_pheader[SCL_IN_PACKET_OPCODE];

	// Free memory from last packet
	scl_free_packet_mem(conn);

	// Allocate memory for the packet and read it in
	conn->in_pdata = mem_alloc(conn->in_mem_id,
		scl_in_pheader[SCL_IN_PACKET_LEN]);
	conn->in_pcursor = conn->in_pdata;

	bytes = scl_fread(conn->in, conn->in_pdata,
		scl_in_pheader[SCL_IN_PACKET_LEN]);

	if(!bytes)
		return 0;

	if(bytes != scl_in_pheader[SCL_IN_PACKET_LEN])
		return -1;

	return bytes;
}

void *scl_get_value(struct suva_conn *conn, enum scl_type type, int *array_len)
{
	void *retval = 0;
	char **char_pointers = 0;
	int string_len = 0, count = 0;

	// Get array element count (if applicable) 
	if(array_len && type >= scl_type_complex)
	{
#ifdef SCO_BIG_ENDIAN
		swap(conn->in_pcursor, sizeof(int));
#endif
		*array_len = *((int *)conn->in_pcursor);
		// Push the cursor past the element count integer
		conn->in_pcursor += SCL_LEN_LEN;
	}

	// Set return value to current location
	retval = conn->in_pcursor;
	
//	if(h_f)
//	{
//		fprintf(h_f, "%s: type: %d, retval: 0x%08x, array_len: %d\n",
//			__func__, type, retval, *array_len);
//		fflush(h_f);
//	}

	// Move the cursor ahead the apropriate amount 
	switch(type)
	{
        case SCL_INT:
			conn->in_pcursor += SCL_INT_LEN;
			break;
        case SCL_LONG:
			conn->in_pcursor += SCL_LONG_LEN;
			break;
        case SCL_FLOAT:
			conn->in_pcursor += SCL_FLOAT_LEN;
			break;
        case SCL_DOUB:
			conn->in_pcursor += SCL_DOUB_LEN;
			break;
        case SCL_BOOL:
			conn->in_pcursor += SCL_BOOL_LEN;
			break;
        case SCL_INT_A:
			if(!array_len)
				return NULL;

			conn->in_pcursor += (SCL_INT_LEN * *array_len);
			break;
        case SCL_LONG_A:
			if(!array_len)
				return NULL;

			conn->in_pcursor += (SCL_LONG_LEN * *array_len);
			break;
        case SCL_FLOAT_A:
			if(!array_len)
				return NULL;

			conn->in_pcursor += (SCL_FLOAT_LEN * *array_len);
			break;
        case SCL_DOUB_A:
			if(!array_len)
				return NULL;

			conn->in_pcursor += (SCL_DOUB_LEN * *array_len);
			break;
        case SCL_BOOL_A:
			if(!array_len)
				return NULL;

			conn->in_pcursor += (SCL_BOOL_LEN * *array_len);
			break;
        case SCL_DATA:
			if(!array_len)
				return NULL;

			conn->in_pcursor += *array_len;
			break;
        case SCL_STRING:
			// Get length then move the cursor forward
#ifdef SCO_BIG_ENDIAN
			swap(conn->in_pcursor, sizeof(int));
#endif
			string_len = *((int *)(conn->in_pcursor));
			conn->in_pcursor += SCL_LEN_LEN;

			// If the length is zero then this is an empty string
			if(!string_len)				
			{
				// Move the cursor over the trailing byte
				conn->in_pcursor++;
				return NULL;
			}

			// Adjust the return value to account for the char count integer
			retval = conn->in_pcursor;

			// Make certain that the trailing byte is zeroed
			*((char *)(conn->in_pcursor + string_len)) = 0;

			// Move the cursor over the string and the trailing byte
			conn->in_pcursor += (string_len + 1);
			break;
        case SCL_STRING_A:
			if(!array_len)
			{
				// Move the cursor over the trailing byte
				conn->in_pcursor++;
				return NULL;
			}

			// Get some memory for the char pointers
			char_pointers = 
				(char **)mem_alloc(conn->in_mem_id, sizeof(char *) * *array_len);
			retval = char_pointers;

			// Scan through the packet, set the char pointers as we go
			for (count = 0; count < *array_len; count++)
			{
				// Get length then zero the length field
				// In this way it acts as the previous string's zero terminator
#ifdef SCO_BIG_ENDIAN
				swap(conn->in_pcursor, sizeof(int));
#endif
				string_len = *((int *)(conn->in_pcursor));
				*((int *)(conn->in_pcursor)) = 0;

				// Then move the cursor forward
				conn->in_pcursor += SCL_LEN_LEN;

				// Set the pointer
				if (string_len == 0)
					char_pointers[count] = NULL;
				else
				{
					char_pointers[count] = (char *)(conn->in_pcursor);
					// Move the cursor forward
					conn->in_pcursor += string_len;
				}
			}

			// Make certain that the trailing byte is zeroed
			*((char *)(conn->in_pcursor)) = 0;

			// Move the cursor over the trailing byte
			conn->in_pcursor++;
			break;
	}

	return retval;
}
