#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <suva_conf.h>
#include <mem.h>
#include <suva.h>
#include <suva_api.h>

#include "suva_receiver.h"
#include "bwmeter.h"

struct suva_conn bwm_conn;
struct suva_conn *conn = &bwm_conn;

void suva_init_receiver()
{
	unsigned int in_mem_id = 0, out_mem_id = 0;

	suva_api_init();

	memset(conn, '\0', sizeof(struct suva_conn));

	conn->in = STDIN_FILENO;
	conn->out = STDOUT_FILENO;

	// Set up the incoming and outgoing packet memory ids
	while(mem_add_type(conn->in_mem_id, "SCL Inbound Packet Memory") < 0)
		conn->in_mem_id = ++in_mem_id;

	out_mem_id = in_mem_id;

	while(mem_add_type(conn->out_mem_id, "SCL Outbound Packet Memory") < 0)
		conn->out_mem_id = ++out_mem_id;

	// Create the first ele of the element queue
	conn->out_cur_ele = (struct scl_ele *)mem_alloc(conn->out_mem_id, sizeof(struct scl_ele));
	conn->out_ele_queue = conn->out_cur_ele;
}

void suva_get_request()
{
	int alen = 1;
	char *func_name;

	scl_read_packet(conn);

	switch(conn->in_packet_opcode)
	{
		case SCL_FCALL:
			break;
		case SCL_DSUVLET:
			fprintf(stderr, "Destroy suvlet.\n");
			deconstructor();
			exit(0);
		default:
			fprintf(stderr, "Unexpected op-code: %02X.\n", conn->in_packet_opcode);
			throw_error(1, "Expected: SCL_FCALL");
			return;
	}

	func_name = (char *)scl_get_value(conn, SCL_STRING, &alen);

	fprintf(stderr, "Requested function call: %s.\n", func_name);

	if(strcmp(func_name, "getVersion") == 0)
		get_version();
	else if(strcmp(func_name, "xfer_chunk") == 0)
		bwm_xfer_chunk();
	else
	{
		// TODO: Handle non-existant function calls.
		conn->out_packet_opcode = SCL_FERROR;

		fprintf(stderr, "Invalid function call: %s.\n", func_name);
	}
}

// Utility functions
void get_version()
{
	int value = 1234;

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &value, SCL_INT, 1);
	scl_write_out(conn);
}

void throw_error(int err_no, char* err_text)
{
	conn->out_packet_opcode = SCL_FERROR;
	scl_queue_value(conn, &err_no, SCL_INT, 1);
	scl_queue_value(conn, err_text, SCL_STRING, 1);
	scl_write_out(conn);
}

// Defined functions
void bwm_xfer_chunk()
{
	unsigned char *chunk;
	int *length;
	unsigned long retval;
	int chunk_len;
	int len_len;

	chunk = (unsigned char *)scl_get_value(conn, SCL_DATA, &chunk_len);
	length = (int *)scl_get_value(conn, SCL_INT, &len_len);

	retval = xfer_chunk(chunk, chunk_len);

	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &retval, SCL_INT, 1);

	scl_write_out(conn);
}
