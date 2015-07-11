#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "suva_conf.h"
#include "suva.h"
#include "suva_api.h"
#include "mem.h"
#include "DeviceInfo.h"
#include "suva_receiver.h"

struct suva_conn _conn;
struct suva_conn *conn = &_conn;

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

	if(strncmp(func_name, "inline-", 7) == 0)
		scl_call_inlines(conn, func_name);
	else if(strcmp(func_name, "Processes") == 0)
		DeviceInfo_Processes_skel();
	else if(strcmp(func_name, "CPUTemp") == 0)
		DeviceInfo_CPUTemp_skel();
	else if(strcmp(func_name, "Uptime") == 0)
		DeviceInfo_Uptime_skel();
	else if(strcmp(func_name, "LoadAverage") == 0)
		DeviceInfo_LoadAverage_skel();
	else if(strcmp(func_name, "MemoryUsage") == 0)
		DeviceInfo_MemoryUsage_skel();
	else if(strcmp(func_name, "SwapUsage") == 0)
		DeviceInfo_SwapUsage_skel();
	else if(strcmp(func_name, "DiskSpaceTotal") == 0)
		DeviceInfo_DiskSpaceTotal_skel();
	else if(strcmp(func_name, "DiskSpaceTable") == 0)
		DeviceInfo_DiskSpaceTable_skel();
	else if(strcmp(func_name, "DiskSpacePercentages") == 0)
		DeviceInfo_DiskSpacePercentages_skel();
    else
    {
        // TODO: Handle non-existant function calls.
        conn->out_packet_opcode = SCL_FERROR;

        fprintf(stderr, "Invalid function call: %s.\n", func_name);
    }
}

// Util Functions
void throw_error(int err_no, char* err_text)
{
	scl_throw_error(conn, err_no, err_text);
}

// Defined functions

void DeviceInfo_Processes_skel()
{
	int retval;

	retval = DeviceInfo_Processes();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &retval, SCL_INT, 1);
	scl_write_out(conn);
}
void DeviceInfo_CPUTemp_skel()
{
	int retval;

	retval = DeviceInfo_CPUTemp();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &retval, SCL_INT, 1);
	scl_write_out(conn);
}
void DeviceInfo_Uptime_skel()
{
	double retval;

	retval = DeviceInfo_Uptime();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &retval, SCL_DOUB, 1);
	scl_write_out(conn);
}
void DeviceInfo_LoadAverage_skel()
{
	char* retval;

	retval = DeviceInfo_LoadAverage();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void DeviceInfo_MemoryUsage_skel()
{
	char* retval;

	retval = DeviceInfo_MemoryUsage();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void DeviceInfo_SwapUsage_skel()
{
	char* retval;

	retval = DeviceInfo_SwapUsage();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void DeviceInfo_DiskSpaceTotal_skel()
{
	char* retval;

	retval = DeviceInfo_DiskSpaceTotal();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void DeviceInfo_DiskSpaceTable_skel()
{
	char* retval;

	retval = DeviceInfo_DiskSpaceTable();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void DeviceInfo_DiskSpacePercentages_skel()
{
	char* retval;

	retval = DeviceInfo_DiskSpacePercentages();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
