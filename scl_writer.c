#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "suva_conf.h"
#include "mem.h"
#include "sutil.h"
#include "suva_api.h"
#include "suva_util.h"

static struct scl_ele *cur_ele = NULL;

#define scl_fwrite scl_write

int scl_write(int sd, void *p_data, unsigned int len)
{
	size_t bytes;
	void *p_chunk = p_data;
	unsigned int length = len;
	time_t start_time = time(NULL), write_time = 0;

	while(write_time < (time_t)SCL_TIMEOUT)
	{
		if((bytes = write(sd, p_chunk, length)) == length)
			break;

		if(bytes == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				usleep(256);
				write_time = time(NULL) - start_time;
				continue;
			}

			fprintf(stderr, "%s: Unable to write %u bytes(s): %s.\n",
				__func__, length, strerror(errno));

			return -1;
		}

		if(bytes == 0)
		{
			fprintf(stderr, "%s: Remote host hung-up.\n", __func__);
			return 0;
		}

		length -= (unsigned int)bytes; p_chunk += bytes;
		start_time = time(NULL); write_time = 0;
		usleep(256);
	}

	if(write_time >= (time_t)SCL_TIMEOUT && (unsigned int)bytes != len)
	{
		fprintf(stderr, "%s: Exceeded write time-out (%d seconds).\n",
			__func__, SCL_TIMEOUT);
		return -1;
	}

	return (int)len;
}

// Writeout
int scl_write_out(struct suva_conn *conn)
{
	int reserved1 = 0;
	int reserved2 = 0;
	struct scl_ele *last_ele = NULL;

#ifdef SCO_BIG_ENDIAN
	swap(&conn->out_packet_opcode, SCL_INT_LEN);
	swap(&reserved1, SCL_INT_LEN);
	swap(&reserved2, SCL_INT_LEN);
	swap(&conn->out_packet_len, SCL_INT_LEN);
#endif
	// Print out packet header
	scl_fwrite(conn->out, &conn->out_packet_opcode, SCL_INT_LEN);
	scl_fwrite(conn->out, &reserved1, SCL_INT_LEN);
	scl_fwrite(conn->out, &reserved2, SCL_INT_LEN);
	scl_fwrite(conn->out, &conn->out_packet_len, SCL_INT_LEN);

	// Loop through the element queue
	cur_ele = conn->out_ele_queue;

	while(cur_ele->next_ele != NULL)
	{
		scl_write_value(conn,
			cur_ele->object, cur_ele->type, cur_ele->array_len);

		last_ele = cur_ele;
		cur_ele = cur_ele->next_ele;

		mem_free(last_ele);
	}

	conn->out_ele_queue = cur_ele;
	conn->out_packet_len = 0;

	return 1;
}

// This function queues whatever value it is passed, and adds its length
// to the running total of that packet
int scl_queue_value(struct suva_conn *conn,
	void *val, enum scl_type type, int array_len)
{
	int count = 0;

	// Assign values to the list element
	cur_ele = conn->out_cur_ele;

	cur_ele->type = type;
	cur_ele->object = val;
	cur_ele->array_len = array_len;

	// Check to see it it is an array
	if (type >= scl_type_complex || type == SCL_STRING)
	{
		// If so add the length prefix to the overall length
		conn->out_packet_len += SCL_LEN_LEN;
	}

	// Reflect the elements length
	switch(type)
	{
		case SCL_INT:
			conn->out_packet_len += SCL_INT_LEN;
			break;
		case SCL_LONG:
			conn->out_packet_len += SCL_LONG_LEN;
			break;
		case SCL_FLOAT:
			conn->out_packet_len += SCL_FLOAT_LEN;
			break;
		case SCL_DOUB:
			conn->out_packet_len += SCL_DOUB_LEN;
			break;
		case SCL_BOOL:
			conn->out_packet_len += SCL_BOOL_LEN;
			break;
		case SCL_INT_A:
			conn->out_packet_len += SCL_INT_LEN * array_len;
			break;
		case SCL_LONG_A:
			conn->out_packet_len += SCL_LONG_LEN * array_len;
			break;
		case SCL_FLOAT_A:
			conn->out_packet_len += SCL_FLOAT_LEN * array_len;
			break;
		case SCL_DOUB_A:
			conn->out_packet_len += SCL_DOUB_LEN * array_len;
			break;
		case SCL_BOOL_A:
			conn->out_packet_len += SCL_BOOL_LEN * array_len;
			break;
		case SCL_DATA:
			conn->out_packet_len += array_len;
			break;
		case SCL_STRING:
			// Account for extra byte
			conn->out_packet_len++;

			if (val != NULL)
				conn->out_packet_len += strlen((char *)val);
			break;
		case SCL_STRING_A:
			// Account for extra byte
			conn->out_packet_len++;

			if (val != NULL)
			{
				// Add all the prefix lengths
				conn->out_packet_len += SCL_LEN_LEN * array_len;

				// Then all the string lengths
				for (count = 0; count < array_len; count++)
				{
					char *cur_string;
					cur_string =((char **)val)[count];
					if (cur_string != NULL)
						conn->out_packet_len += strlen(cur_string);
				}
			}
			break;
	}

	scl_alloc_ele(conn);

	return 1;
}


// Writes out a queued element
int scl_write_value(struct suva_conn *conn, void *val, enum scl_type type, int array_len)
{
	void *p_ptr;
	int i, count = 0, string_len = 0, zero = 0;

	// Check to see it it is an array
	if (type >= scl_type_complex)
	{
#ifdef SCO_BIG_ENDIAN
		swap(&array_len, SCL_LEN_LEN);
#endif
		// If so write out the length prefix
		scl_fwrite(conn->out, &array_len, SCL_LEN_LEN);
#ifdef SCO_BIG_ENDIAN
		swap(&array_len, SCL_LEN_LEN);
#endif
		// If it's zero, then we're done for this element
		if (array_len == 0)
		{
			// Write out the trailing byte if it's a string array
			if (type == SCL_STRING_A)
				scl_fwrite(conn->out, &zero, 1);
			return 1;
		}
	}

	switch(type)
	{
		case SCL_INT:
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_INT_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_INT_LEN);
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_INT_LEN);
#endif
			break;
		case SCL_LONG:
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_LONG_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_LONG_LEN);
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_LONG_LEN);
#endif
			break;
		case SCL_FLOAT:
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_FLOAT_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_FLOAT_LEN);
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_FLOAT_LEN);
#endif
			break;
		case SCL_DOUB:
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_DOUB_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_DOUB_LEN);
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_DOUB_LEN);
#endif
			break;
		case SCL_BOOL:
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_BOOL_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_BOOL_LEN);
#ifdef SCO_BIG_ENDIAN
			swap(val, SCL_BOOL_LEN);
#endif
			break;
		case SCL_INT_A:
#ifdef SCO_BIG_ENDIAN
			p_ptr = val;

			for(i = 0; i < array_len; i++, (unsigned long)p_ptr += SCL_INT_LEN)
				swap(p_ptr, SCL_INT_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_INT_LEN * array_len);
#ifdef SCO_BIG_ENDIAN
			p_ptr = val;

			for(i = 0; i < array_len; i++, (unsigned long)p_ptr += SCL_INT_LEN)
				swap(p_ptr, SCL_INT_LEN);
#endif
			break;
		case SCL_LONG_A:
#ifdef SCO_BIG_ENDIAN
			p_ptr = val;

			for(i = 0; i < array_len; i++, (unsigned long)p_ptr += SCL_LONG_LEN)
				swap(p_ptr, SCL_LONG_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_LONG_LEN * array_len);
#ifdef SCO_BIG_ENDIAN
			p_ptr = val;

			for(i = 0; i < array_len; i++, (unsigned long)p_ptr += SCL_LONG_LEN)
				swap(p_ptr, SCL_LONG_LEN);
#endif
			break;
		case SCL_FLOAT_A:
#ifdef SCO_BIG_ENDIAN
			p_ptr = val;

			for(i = 0; i < array_len; i++, (unsigned long)p_ptr += SCL_FLOAT_LEN)
				swap(p_ptr, SCL_FLOAT_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_FLOAT_LEN * array_len);
#ifdef SCO_BIG_ENDIAN
			p_ptr = val;

			for(i = 0; i < array_len; i++, (unsigned long)p_ptr += SCL_FLOAT_LEN)
				swap(p_ptr, SCL_FLOAT_LEN);
#endif
			break;
		case SCL_DOUB_A:
#ifdef SCO_BIG_ENDIAN
			p_ptr = val;

			for(i = 0; i < array_len; i++, (unsigned long)p_ptr += SCL_DOUB_LEN)
				swap(p_ptr, SCL_DOUB_LEN);
#endif
			scl_fwrite(conn->out, val, SCL_DOUB_LEN * array_len);
#ifdef SCO_BIG_ENDIAN
			p_ptr = val;

			for(i = 0; i < array_len; i++, (unsigned long)p_ptr += SCL_DOUB_LEN)
				swap(p_ptr, SCL_DOUB_LEN);
#endif
			break;
		case SCL_BOOL_A:
			scl_fwrite(conn->out, val, SCL_BOOL_LEN * array_len);
			break;
		case SCL_DATA:
			scl_fwrite(conn->out, val, array_len);
			break;
		case SCL_STRING:
			if (val == NULL)
				scl_fwrite(conn->out, &zero, SCL_LEN_LEN);
			else
			{
				string_len = strlen((char *)val);
#ifdef SCO_BIG_ENDIAN
				swap(&string_len, sizeof(string_len));
#endif
				scl_fwrite(conn->out, &string_len, SCL_LEN_LEN);
#ifdef SCO_BIG_ENDIAN
				swap(&string_len, sizeof(string_len));
#endif
				scl_fwrite(conn->out, val, string_len);
			}
			// Write out the trailing string byte
			scl_fwrite(conn->out, &zero, 1);
			break;
		case SCL_STRING_A:
			for (count = 0; count < array_len; count++)
			{
				char *cur_string;
				cur_string = ((char **)val)[count];

				if (cur_string == NULL)
					scl_fwrite(conn->out, &zero, SCL_LEN_LEN);
				else
				{
					string_len = strlen(cur_string);
#ifdef SCO_BIG_ENDIAN
					swap(&string_len, sizeof(string_len));
#endif
					scl_fwrite(conn->out, &string_len, SCL_LEN_LEN);
#ifdef SCO_BIG_ENDIAN
					swap(&string_len, sizeof(string_len));
#endif
					scl_fwrite(conn->out, cur_string, string_len);
				}
			}
			// Write out the trailing byte
			scl_fwrite(conn->out, &zero, 1);
			break;
	}

	return 1;
}

void scl_alloc_ele(struct suva_conn *conn)
{
	// Set the 'next element' and allocate new mem for it:
	cur_ele->next_ele = (struct scl_ele *)mem_alloc(conn->out_mem_id, sizeof(struct scl_ele));

	conn->out_cur_ele = cur_ele->next_ele;
}

