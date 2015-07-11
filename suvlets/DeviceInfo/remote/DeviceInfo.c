#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DeviceInfo.h"
#include "suva_receiver.h"

#define XMLSIZE 4000
#define BUFSIZE 100

char payload[XMLSIZE];

int main(int argc, char *argv[])
{
	constructor();
	fprintf(stderr, "Init receiver.\n");
	suva_init_receiver();

	for(;;)
		suva_get_request();

	fprintf(stderr, "Shutdown.\n");
}


// Number of processes running
// ---------------------------

int DeviceInfo_Processes()
{
	FILE *output;
	char msg[BUFSIZE];
	int processes;

	output = fopen("/proc/loadavg", "r");
	// test throwing error output = fopen("/proc/meminfo", "r");
	if (output == NULL)
		return -1;

	fgets(msg, BUFSIZE, output);
	if (sscanf(msg, "%*s %*s %*s %*i/%i", &processes) != 1)
		return -1;

   	fclose(output);
   	return processes;
}


// CPU temperature on Cobalts
// --------------------------
int DeviceInfo_CPUTemp()
{
   	FILE *output;
   	char msg[BUFSIZE];
	int temperature = -1;

   	output = fopen("/proc/cpuinfo", "r");
	if (output == NULL)
		return -1;

	while (fgets(msg, BUFSIZE, output)) {
		if (sscanf(msg, "temperature     : %i", &temperature) == 1)
			break;
	}

   	fclose(output);
	return temperature;	
}


// Uptime - in seconds
// -------------------

double DeviceInfo_Uptime()
{
	FILE *output;
	char msg[BUFSIZE];
	double uptime;

	output = fopen("/proc/uptime", "r");
	if (output == NULL)
		return -1;

	fgets(msg, BUFSIZE, output);
	if (sscanf(msg,"%lf %*s", &uptime) != 1)
		return -1;

   	fclose(output);
   	return uptime;
}


// Load average returned as a comma separated list - 1 min, 5 min, 15 min avg.
// ---------------------------------------------------------------------------

char* DeviceInfo_LoadAverage()
{
	FILE *output;
	char msg[BUFSIZE];
	char min1[10];
	char min5[10];
	char min15[10];

	output = fopen("/proc/loadavg", "r");
	if (output == NULL)
		return "error";

	fgets(msg, BUFSIZE, output);
	if (sscanf(msg,"%s %s %s ", min1, min5, min15) != 3)
		return "error";

	strcpy(payload, min1);
	strcat(payload, ",");
	strcat(payload, min5);
	strcat(payload, ",");
	strcat(payload, min15);

   	fclose(output);
   	return payload;
}


// Memory usage
//-------------

char* DeviceInfo_MemoryUsage()
{
   	FILE *output;
   	char msg[BUFSIZE];
	char total[30];
	char used[30];
	char free[30];

   	output = fopen("/proc/meminfo", "r");
	if (output == NULL)
		return "error";

   	fgets(msg, BUFSIZE, output); 
   	fgets(msg, BUFSIZE, output);
	if (sscanf(msg,"%*s %s %s %s %*s %*s %*s", total, used, free) != 3)
		return "error";

	strcpy(payload, total);
	strcat(payload, ",");
	strcat(payload, used);
	strcat(payload, ",");
	strcat(payload, free);
   	fclose(output);
	
	return payload;
}


// Swap space usage
// ----------------

char* DeviceInfo_SwapUsage()
{
   	FILE *output;
   	char msg[BUFSIZE];
	char total[30];
	char used[30];
	char free[30];

   	output = fopen("/proc/meminfo", "r");
	if (output == NULL)
		return "error";

   	fgets(msg, BUFSIZE, output); 
   	fgets(msg, BUFSIZE, output);
   	fgets(msg, BUFSIZE, output);
	if (sscanf(msg,"%*s %s %s %s", total, used, free) != 3)
		return "error";

	strcpy(payload, total);
	strcat(payload, ",");
	strcat(payload, used);
	strcat(payload, ",");
	strcat(payload, free);
   	fclose(output);
	
	return payload;
}


// Total disk space information (adding all the patitions)
// -------------------------------------------------------

char* DeviceInfo_DiskSpaceTotal()
{
	char msg[BUFSIZE];
   	FILE *output;
	long partitiontotal;
	long partitionused;
	long partitionfree;
	long total = 0;
	long used = 0;
	long free = 0;

   	output = popen("/bin/df | grep ^/dev", "r");
	if (output == NULL)
		return "error";
		
	fgets(msg, BUFSIZE, output);  // Discard header line

	while (fgets(msg, BUFSIZE, output)) {
		if (sscanf(msg, "%*s %li %li %li %*s %*s", &partitiontotal, &partitionused, &partitionfree) != 3)
			return "error";
		total += partitiontotal;
		used += partitionused;
		free += partitionfree;
	}
   	fclose(output);
	sprintf(payload, "%li,%li,%li", total, used, free);
	
	return payload;
}


// Disk space table - in XML
// -------------------------

char* DeviceInfo_DiskSpaceTable()
{
	char msg[BUFSIZE];
   	FILE *output;
	char filesystem[200];
	char blocks[30];
	char used[30];
	char available[30];
	char percentused[30];
	char mount[200];

	/*******************************************************************/
	/* Grab the df (disk free) information.							*/
	/* We grab total, used, and available for each harddisk partition  */
	/*******************************************************************/

   	output = popen("/bin/df | grep ^/dev", "r");
	if (output == NULL)
		return "error";
		
	/*******************************************************************/
	/* Send the disk information in XML								*/
	/*******************************************************************/

	strcpy(payload, "<?xml version='1.0' ?>\n");
	strcat(payload, "<Disks>\n");
	
	while (fgets(msg, BUFSIZE, output)) {
		strcat(payload, "<Disk>\n");
		if (sscanf(msg, "%s %s %s %s %s %s",
			filesystem, blocks, used, available, percentused, mount) != 6)
			return "error";
		strcat(payload, "<Filesystem>");
		strcat(payload, filesystem);
		strcat(payload, "</Filesystem>\n");
		strcat(payload, "<Blocks>");
		strcat(payload, blocks);
		strcat(payload, "</Blocks>\n");
		strcat(payload, "<Used>");
		strcat(payload, used);
		strcat(payload, "</Used>\n");
		strcat(payload, "<Available>");
		strcat(payload, available);
		strcat(payload, "</Available>\n");
		strcat(payload, "<Use>");
		strcat(payload, percentused);
		strcat(payload, "</Use>\n");
		strcat(payload, "<Mounted_on>");
		strcat(payload, mount);
		strcat(payload, "</Mounted_on>\n");
		strcat(payload, "</Disk>\n");
	}
	strcat(payload, "</Disks>\n");
	pclose(output);

	return payload;
}


// Disk space percentages and mount points
// ---------------------------------------

char* DeviceInfo_DiskSpacePercentages()
{
	char msg[BUFSIZE];
   	FILE *output;
	char percentused[10];
	char mount[100];

	/*******************************************************************/
	/* Grab the df (disk free) information.							*/
	/* We grab total, used, and available for each harddisk partition  */
	/*******************************************************************/

   	output = popen("/bin/df | grep ^/dev", "r");
	if (output == NULL)
		return "error";
		
	strcpy(payload, "");
	while (fgets(msg, BUFSIZE, output)) {
		if ((sscanf(msg, "%*s %*s %*s %*s %s %s", percentused, mount)) == 2) { 
			strcat(payload, mount);
			strcat(payload, ",");
			strcat(payload, percentused);
			strcat(payload, ",");
		} else {
			return "sscanf error";
		}
	}
	pclose(output);

	return payload;
}


void constructor()
{}

void deconstructor()
{}
