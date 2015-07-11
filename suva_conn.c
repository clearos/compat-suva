#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "suva_conf.h"
#include "suva.h"
#include "mem.h"
#include "suva_api.h"
#include "suva_util.h"

// The default error handler
void default_err_handler(int err_no, char *err_text)
{
	output(1, "[%02X] %s", err_no, err_text);
//	fprintf(stderr, "[%02X] %s\n", err_no, err_text);
}

// Open connection to suvads
struct suva_conn *open_suva_conn(char *local_host, int local_port, char *remote_host, int remote_port, char *org, int timeout)
{
	char *return_text;
	int suvad_sockfd;
	int return_no, on = 1;;
	struct hostent *suvad_he;
	struct sockaddr_in suvad_addr;
	struct suva_conn *conn = 0;
	static int in_mem_id = 0, out_mem_id = 0;

	suva_api_init();

	conn = (struct suva_conn *)mem_alloc(1024, sizeof(struct suva_conn));

	// Open tcp/ip connection to suvad 
	if((suvad_he = gethostbyname(local_host)) == NULL)
	{
		mem_free(conn);

		return NULL;
	}

	if((suvad_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		mem_free(conn);

		return NULL;
	}

	suvad_addr.sin_family = AF_INET;
	suvad_addr.sin_port = htons(local_port);
	suvad_addr.sin_addr = *((struct in_addr *)suvad_he->h_addr);
	bzero(&(suvad_addr.sin_zero), 8);

	// Set default output handler:
	conn->err_handler = default_err_handler;

	if(connect(suvad_sockfd, (struct sockaddr *)&suvad_addr,sizeof(struct sockaddr)) == -1)
	{
		close(suvad_sockfd);
		mem_free(conn);

		return NULL;
	}

	// Make socket non-blocking...
	if(fcntl(suvad_sockfd, F_SETFL, O_NONBLOCK) == -1)
	{
		close(suvad_sockfd);
		mem_free(conn);

		return NULL;
	}

	// Change the file descriptor into the i/o streams
	conn->in = suvad_sockfd;
	conn->out = suvad_sockfd;

	// Set up the incoming and outbound packet memory id
	while(mem_add_type(conn->in_mem_id, "SCL Inbound Packet Memory") < 0)
		conn->in_mem_id = ++in_mem_id;

	out_mem_id = in_mem_id;

	while(mem_add_type(conn->out_mem_id, "SCL Outbound Packet Memory") < 0)
		conn->out_mem_id = ++out_mem_id;

	// Create the first ele of the element queue
	conn->out_cur_ele = (struct scl_ele *)mem_alloc(conn->out_mem_id, sizeof(struct scl_ele));

	conn->out_ele_queue = conn->out_cur_ele;

	// Send some scl to tell the local suvad to init the connection to the remote one
	conn->out_packet_opcode = SCL_OCONN;
	scl_queue_value(conn, &timeout, SCL_INT, 1);
	scl_queue_value(conn, &remote_port, SCL_INT, 1);
	scl_queue_value(conn, remote_host, SCL_STRING, 1);
	scl_queue_value(conn, org, SCL_STRING, 1);

	scl_write_out(conn);

	// Grab 'connecting' message
	if(scl_read_packet(conn) <= 0)
		return NULL;

	return_no = *(int *)scl_get_value(conn, SCL_INT, NULL);
	return_text = scl_get_value(conn, SCL_STRING, NULL);

	conn->err_handler(return_no, return_text);

	if(return_no != SRC_CONNECT)
	{
		conn->err_no = 1;
		conn->err_text = return_text;
		close(suvad_sockfd);
		scl_free_packet_mem(conn);
		mem_free(conn);

		return NULL;
	}
	
	// Grab 'connected' message
	scl_read_packet(conn);
	return_no = *(int *)scl_get_value(conn, SCL_INT, NULL);
	return_text = scl_get_value(conn, SCL_STRING, NULL);

	conn->err_handler(return_no, return_text);

	if(return_no != SRC_CONNECTED)
	{	
		conn->err_no = 1;
		conn->err_text = return_text;
		close(suvad_sockfd);
		scl_free_packet_mem(conn);
		mem_free(conn);

		return NULL;
	}

	// Free the memory we used to getb those last two messages
	scl_free_packet_mem(conn);

	return conn;
}

// Close the connection to the suvas
void close_suva_conn(struct suva_conn *conn)
{
	// Free the starter ele in element queue
	mem_free(conn->out_ele_queue);

	// Free any incoming packet mem
	scl_free_packet_mem(conn);

	// Tell the local suvad to disconnect from the remote one.
	conn->out_packet_opcode = SCL_CCONN;
	scl_write_out(conn);

	// Get response, we don't care about it though
	scl_read_packet(conn);
	scl_free_packet_mem(conn);

	// Drop the tcp/ip connection
	close(conn->in);

	// Drop the conn struct
	mem_free(conn);
}

// Creates a suvlet
int create_suvlet(struct suva_conn *conn, char *suvlet_name, int exclusive, int encrypted, int ttl)
{
	int result_code;
	char *result_text;

	// Write out the 'create' scl packet
	conn->out_packet_opcode = SCL_CSUVLET;
	scl_queue_value(conn, &ttl, SCL_INT, 1);
	scl_queue_value(conn, &exclusive, SCL_INT, 1);
	scl_queue_value(conn, &encrypted, SCL_INT, 1);
	scl_queue_value(conn, suvlet_name, SCL_STRING, 1);

	scl_write_out(conn);

	// Get response
	scl_read_packet(conn);

	result_code = *(int *)scl_get_value(conn, SCL_INT, NULL);
	result_text = scl_get_value(conn, SCL_STRING, NULL);

	conn->err_handler(result_code, result_text);

	if(result_code == SRC_OKAY)
	{
		scl_free_packet_mem(conn);
		return 1;
	}
	else
	{
		if(conn->in_packet_opcode != SCL_RETURN)
			conn->err_handler(0, "Received unexpected return code.");

		scl_free_packet_mem(conn);
		return 0;
	}
}

// Destroys the current suvlet
void destroy_suvlet(struct suva_conn *conn)
{
	// Write out the 'destroy' scl packet
	conn->out_packet_opcode = SCL_DSUVLET;
	scl_write_out(conn);

	// Get response, we don't care about it though
	scl_read_packet(conn);
	scl_free_packet_mem(conn);
}

// Throw error
void suva_throw_err(struct suva_conn *conn, int err_no, char *err_text)
{
	if(conn->err_handler)
		conn->err_handler(err_no, err_text);
}

// We get this if there was an unexpected return code waiting for a function return
void suva_unexpected_return(struct suva_conn *conn)
{
	int return_no;
	char *return_text;

	if (conn->in_packet_opcode == SCL_FERROR)
	{
		scl_read_packet(conn);

		return_no = *(int *)scl_get_value(conn, SCL_INT, NULL);
		return_text = scl_get_value(conn, SCL_STRING, NULL);

		suva_throw_err(conn, return_no, return_text);
	}
}

void *get_additional_packet_mem(struct suva_conn *conn, unsigned int size)
{
	return mem_alloc(conn->out_mem_id, size);
}
