#ifndef _SUVA_RECEIVER_H
#define _SUVA_RECEIVER_H

void suva_init_receiver();
void suva_get_request();
void throw_error(int err_no, char* err_text);

void Software_CreatePackageList_skel();
void Software_FetchPackageList_skel();
void Software_InstallRPM_skel();
void Software_InstallPKG_skel();
void Software_UnInstallPKG_skel();
void Software_ResetPKGDir_skel();


#endif // _SUVA_RECEIVER_H
