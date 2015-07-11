#ifndef _DeviceInfo_h_
#define _DeviceInfo_h_

int main(int argc, char *argv[]);
void constructor();
void deconstructor();

int DeviceInfo_Processes();
int DeviceInfo_CPUTemp();
double DeviceInfo_Uptime();
char* DeviceInfo_LoadAverage();
char* DeviceInfo_MemoryUsage();
char* DeviceInfo_SwapUsage();
char* DeviceInfo_DiskSpaceTotal();
char* DeviceInfo_DiskSpaceTable();
char* DeviceInfo_DiskSpacePercentages();


#endif  // _DeviceInfo_h_
