#ifndef _SUVA_RECIEVER_H
#define _SUVA_RECIEVER_H

void suva_init_receiver();
void suva_get_request();
void get_version();
void throw_error(int err_no, char* err_text);
void bob_calc();

#endif  // _SUVA_RECIEVER_H
