#ifndef _SUVA_RECEIVER_H
#define _SUVA_RECEIVER_H

void suva_init_receiver();
void suva_get_request();
void throw_error(int err_no, char* err_text);

void DeviceInfo_Processes_skel();
void DeviceInfo_CPUTemp_skel();
void DeviceInfo_Uptime_skel();
void DeviceInfo_LoadAverage_skel();
void DeviceInfo_MemoryUsage_skel();
void DeviceInfo_SwapUsage_skel();
void DeviceInfo_DiskSpaceTotal_skel();
void DeviceInfo_DiskSpaceTable_skel();
void DeviceInfo_DiskSpacePercentages_skel();


#endif // _SUVA_RECEIVER_H
