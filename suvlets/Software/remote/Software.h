#ifndef _Software_h_
#define _Software_h_

int main(int argc, char *argv[]);
void constructor();
void deconstructor();

int Software_CreatePackageList(int devicetype);
char* Software_FetchPackageList();
char* Software_InstallRPM(char* filename);
char* Software_InstallPKG(char* filename);
char* Software_UnInstallPKG(char* filename);
char* Software_ResetPKGDir();


#endif  // _Software_h_
