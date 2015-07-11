#ifndef _BWMETER_H
#define _BWMETER_H

// Local:
#ifdef _BWMETER_LOCAL
int bwm_create(struct suva_conn *conn); 
unsigned long bwm_xfer_chunk(struct suva_conn *conn, unsigned char *data, long length); 
void bwm_result_free(struct suva_conn *conn); 
void bwm_destroy(struct suva_conn *conn);
#endif

// Remote:
#ifdef _BWMETER_REMOTE
int main(int argc, char *argv[]);
unsigned long xfer_chunk(unsigned char *data, long length);
void bwm_xfer_chunk(void);
void constructor();
void deconstructor();
#endif

#endif  // _BWMETER_H
