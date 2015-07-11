#ifndef _SAPI_H
#define _SAPI_H

#include <stdio.h>

// This lib version
#define SUVA_API_VERSION 3


// SCL datatype sizes:
#define SCL_INT_LEN	4
#define SCL_LONG_LEN	8
#define SCL_FLOAT_LEN	4
#define SCL_DOUB_LEN	8
#define SCL_BOOL_LEN	1
#define SCL_LEN_LEN	4

// SCL packet header offsets:
#define SCL_IN_PACKET_OPCODE 0
#define SCL_IN_PACKET_RESERVED1 1
#define SCL_IN_PACKET_RESERVED2 2
#define SCL_IN_PACKET_LEN 3

// SCL Inline call defs
#define SCL_IL_XFER_DEFAULT_PSIZE 8192
#define SCL_IL_XFER_NUM_BPS_AVGS 5
#define SCL_IL_XFER_GET 1
#define SCL_IL_XFER_PUT 2

// Read/write timeout (seconds)
#define SCL_TIMEOUT	30

// Signal handler callback typedef:
typedef void (*err_handler_t)(int, char *);

struct scl_ele
{
	void    *object;
	int     type;
	int	array_len;

	struct	scl_ele *next_ele;
};

struct suva_conn
{
	int		in;
	int		out;
	int		err_no;
	char 		*err_text;
	int		out_packet_len;
	int		out_packet_opcode;
	int 		in_packet_opcode;
	void		*in_pdata;
	void		*in_pcursor;
	err_handler_t 	err_handler;
	unsigned int	in_mem_id;
	unsigned int	out_mem_id;
	struct scl_ele	*out_ele_queue;
	struct scl_ele	*out_cur_ele;
};

struct scl_copy_progress
{
	int bytes_total;
	int bytes_transfered;
	int msec_elapsed;
	int avg_bps;
	int eta;
};

enum scl_type
{
	SCL_INT, 
	SCL_STRING, 
	SCL_FLOAT, 
	SCL_LONG, 
	SCL_DOUB, 
	SCL_BOOL, 

	// Array types:
	SCL_INT_A, 
	SCL_STRING_A, 
	SCL_FLOAT_A, 
	SCL_LONG_A, 
	SCL_DOUB_A, 
	SCL_BOOL_A, 

	SCL_DATA
};

// A var for the suvlet to set so that the suva_api can return it to any askers
int suvlet_version;

#define scl_type_complex SCL_INT_A

// Init api
void suva_api_init(void);

// Create Suva Connection
struct suva_conn *open_suva_conn(char *local_host, int local_port, char *remote_host, int remote_port, char *org, int timeout);

// Drop Suva Connection
void close_suva_conn(struct suva_conn *conn);

// Suvlet creation and destuction
int create_suvlet(struct suva_conn *conn, char *suvlet_name, int exclusive, int encrypted, int ttl);
void destroy_suvlet(struct suva_conn *conn);

// For throwing errors
void suva_unexpected_return(struct suva_conn *conn);
void suva_reg_err(struct suva_conn *conn, err_handler_t err_handler);
void suva_throw_err(struct suva_conn *conn, int err_no, char *err_text);

void *scl_get_value(struct suva_conn *conn, enum scl_type type, int *array_len);

// Free incoming packet mem
void scl_free_packet_mem(struct suva_conn *conn);

// Get some of the incoming packet mem
void *get_additional_packet_mem(struct suva_conn *conn, unsigned int size);

// Write out the queue sitting on a suva connection
int scl_write_out(struct suva_conn *conn);

// Queue function
int scl_queue_value(struct suva_conn *conn, void *val, enum scl_type type, int count);

// Write function
int scl_write_value(struct suva_conn *conn, void *val, enum scl_type type, int count);

// Read packet:
ssize_t scl_read_packet(struct suva_conn *conn);

// Util funcs
void scl_alloc_ele(struct suva_conn *conn);
void scl_fwrite(int fd, void *data, int data_len);
ssize_t scl_fread(int fd, void *data, int data_len);


// Inline functions
// Vars
FILE *scl_inline_file;

// Public functions
// Get info
char *scl_il_get_cwd(struct suva_conn *conn);
char *scl_il_get_suva_base(struct suva_conn *conn);
int scl_il_get_pid(struct suva_conn *conn);
int scl_il_get_suvlet_version(struct suva_conn *conn);
int scl_il_get_api_version(struct suva_conn *conn);

// Xfer file
int scl_il_xfer(struct suva_conn *conn, char *local_file_name, char *remote_file_name, int direction);

// Callback version
int scl_il_xfer_ex(struct suva_conn *conn,
				   	char *local_file_name,
					char *remote_file_name,
					int direction,
					int block_size,
					int (*scl_copy_prog_callback)(struct scl_copy_progress *copy_prog),
					int callback_interval);

// Private functions
// Elseif inline function caller of doom
void scl_call_inlines(struct suva_conn *conn, char *func_name);
int scl_il_xfer_exit(int exit_code, int print_errno, int kill_local_file, int kill_remote_file, struct suva_conn *conn, FILE *file_handle, void *data, int direction);

// For throwing errors across the network
void scl_throw_error(struct suva_conn *conn, int err_no, char* err_text);

// Remote components of the get and put file inline func calls
void scl_il_r_open_file(struct suva_conn *conn);
void scl_il_r_close_file(struct suva_conn *conn);
void scl_il_r_put_data(struct suva_conn *conn);
void scl_il_r_get_data(struct suva_conn *conn);
void scl_il_r_get_fsize_mode(struct suva_conn *conn); // v3
void scl_il_r_set_fmode(struct suva_conn *conn); // v3

// Remote component for getting various info
void scl_il_r_get_info(struct suva_conn *conn);

// CRC
int scl_crc(char *data, int data_len);

#endif  // _SUVA_H
