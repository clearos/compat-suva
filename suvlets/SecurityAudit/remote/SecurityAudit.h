#ifndef _SecurityAudit_h_
#define _SecurityAudit_h_

int main(int argc, char *argv[]);
void constructor();
void deconstructor();

char SecurityAudit_mk_working_dir(char* dir_name);
char SecurityAudit_rmdir(char* dir_name);
char* SecurityAudit_get_md5(char* prog, char* file);
char* SecurityAudit_run_aide(char* prog, char* aide_conf, char* working_dir);
char* SecurityAudit_get_new_xuids(char* prog);
char* SecurityAudit_get_new_dot_files(char* prog);
char* SecurityAudit_get_passwordless(char* prog);
int SecurityAudit_get_uid_zeros();


#endif  // _SecurityAudit_h_
