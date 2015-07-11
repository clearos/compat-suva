#ifndef _SUVA_RECEIVER_H
#define _SUVA_RECEIVER_H

void suva_init_receiver();
void suva_get_request();
void throw_error(int err_no, char* err_text);

void SecurityAudit_mk_working_dir_skel();
void SecurityAudit_rmdir_skel();
void SecurityAudit_get_md5_skel();
void SecurityAudit_run_aide_skel();
void SecurityAudit_get_new_xuids_skel();
void SecurityAudit_get_new_dot_files_skel();
void SecurityAudit_get_passwordless_skel();
void SecurityAudit_get_uid_zeros_skel();


#endif // _SUVA_RECEIVER_H
