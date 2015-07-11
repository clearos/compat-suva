#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "suva_conf.h"
#include "sutil.h"
#include "mem.h"
#include "suva.h"
#include "suva_api.h"
#include "SecurityAudit.h"
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
            fprintf(stderr, "Function call.\n");
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
	else if(strcmp(func_name, "mk_working_dir") == 0)
		SecurityAudit_mk_working_dir_skel();
	else if(strcmp(func_name, "rmdir") == 0)
		SecurityAudit_rmdir_skel();
	else if(strcmp(func_name, "get_md5") == 0)
		SecurityAudit_get_md5_skel();
	else if(strcmp(func_name, "run_aide") == 0)
		SecurityAudit_run_aide_skel();
	else if(strcmp(func_name, "get_new_xuids") == 0)
		SecurityAudit_get_new_xuids_skel();
	else if(strcmp(func_name, "get_new_dot_files") == 0)
		SecurityAudit_get_new_dot_files_skel();
	else if(strcmp(func_name, "get_passwordless") == 0)
		SecurityAudit_get_passwordless_skel();
	else if(strcmp(func_name, "get_uid_zeros") == 0)
		SecurityAudit_get_uid_zeros_skel();
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

void SecurityAudit_mk_working_dir_skel()
{
	char* dir_name;
	char retval;

	dir_name = (char* )scl_get_value(conn, SCL_STRING, NULL);
	retval = SecurityAudit_mk_working_dir(dir_name);
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &retval, SCL_BOOL, 1);
	scl_write_out(conn);
}
void SecurityAudit_rmdir_skel()
{
	char* dir_name;
	char retval;

	dir_name = (char* )scl_get_value(conn, SCL_STRING, NULL);
	retval = SecurityAudit_rmdir(dir_name);
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &retval, SCL_BOOL, 1);
	scl_write_out(conn);
}
void SecurityAudit_get_md5_skel()
{
	char* prog;
	char* file;
	char* retval;

	prog = (char* )scl_get_value(conn, SCL_STRING, NULL);
	file = (char* )scl_get_value(conn, SCL_STRING, NULL);
	retval = SecurityAudit_get_md5(prog, file);
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void SecurityAudit_run_aide_skel()
{
	char* prog;
	char* aide_conf;
	char* working_dir;
	char* retval;

	prog = (char* )scl_get_value(conn, SCL_STRING, NULL);
	aide_conf = (char* )scl_get_value(conn, SCL_STRING, NULL);
	working_dir = (char* )scl_get_value(conn, SCL_STRING, NULL);
	retval = SecurityAudit_run_aide(prog, aide_conf, working_dir);
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void SecurityAudit_get_new_xuids_skel()
{
	char* prog;
	char* retval;

	prog = (char* )scl_get_value(conn, SCL_STRING, NULL);
	retval = SecurityAudit_get_new_xuids(prog);
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void SecurityAudit_get_new_dot_files_skel()
{
	char* prog;
	char* retval;

	prog = (char* )scl_get_value(conn, SCL_STRING, NULL);
	retval = SecurityAudit_get_new_dot_files(prog);
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void SecurityAudit_get_passwordless_skel()
{
	char* prog;
	char* retval;

	prog = (char* )scl_get_value(conn, SCL_STRING, NULL);
	retval = SecurityAudit_get_passwordless(prog);
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, retval, SCL_STRING, 1);
	scl_write_out(conn);
}
void SecurityAudit_get_uid_zeros_skel()
{
	int retval;

	retval = SecurityAudit_get_uid_zeros();
	scl_free_packet_mem(conn);

	conn->out_packet_opcode = SCL_FRETURN;
	scl_queue_value(conn, &retval, SCL_INT, 1);
	scl_write_out(conn);
}
