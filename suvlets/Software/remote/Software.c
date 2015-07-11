#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Software.h"
#include "suva_receiver.h"

#define XMLSIZE 92160
#define BUFSIZE 500
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

char* Software_FetchPackageList()
{
	FILE *package_xml;
	char line[BUFSIZE];

	strcpy(payload, "");
	if ((package_xml = fopen("packages.xml", "r"))) {
		while (fgets(line, BUFSIZE, package_xml)) {
			strcat(payload, line);
		}
	} else {
		return "error";
	}
	return payload;
}


int Software_CreatePackageList(int devicetype)
{
	int count;
	FILE *package_list;
	FILE *package_info;
	FILE *package_xml;
	char *cobaltver;
	char *cobaltrel;
	char *cobaltname;
	char cobaltextraversion[5] = "";
	char line[BUFSIZE];
	char line1[BUFSIZE];
	char name[100];
	char version[100];
	char release[20];
	char size[20];
	char summary[500];
	char command[200];
	char thedate[100];
	time_t stamp;

	stamp = time(NULL);
	strftime(thedate, 100, "%B %d, %Y - %H:%M:%S", localtime(&stamp));

	if ((package_xml = fopen("packages.xml.new", "w"))) {
		fprintf(package_xml, "<?xml version='1.0' ?>\n");
		fprintf(package_xml, "<Software>\n");
		fprintf(package_xml, " <Date>%s</Date>\n", thedate);
		fprintf(package_xml, " <Packages>\n");
	} else {
		return -1;
	}

	// RPM-based systems
	// -----------------

	if ((devicetype >= 1000) && (devicetype < 2000)) {
		package_list = popen("/bin/rpm -qa", "r");
		if (package_list == NULL)
			return -1;

		while (fgets(line, BUFSIZE, package_list)) 
		{
			strcpy(command, "/bin/rpm -q --queryformat ");
			strcat(command, "\"%{NAME}\t%{VERSION}\t%{RELEASE}\t%{SIZE}\t%{SUMMARY}\n\" ");
			strcat(command, line);
			package_info = popen(command, "r");

			if (package_info != NULL)
			{
				fgets(line1, BUFSIZE, package_info);
				if (sscanf(line1, "%s\t%s\t%s\t%s\t%[^\n]s", name, version, release, size, summary) != 5)
					return -1;
				fprintf(package_xml, "  <Package>\n");
				fprintf(package_xml, "   <Name>%s</Name>\n", name);
				fprintf(package_xml, "   <Version>%s</Version>\n", version);
				fprintf(package_xml, "   <Release>%s</Release>\n", release);
				fprintf(package_xml, "   <Size>%s</Size>\n", size);
				fprintf(package_xml, "   <Summary>%s</Summary>\n", summary);
				fprintf(package_xml, "  </Package>\n");
			}
			pclose(package_info);
		}
	}


	// Cobalt
	//-------

	if ((devicetype >= 2000) && (devicetype < 3000)) {
		package_list = popen("/bin/ls /var/lib/cobalt/*.md5lst", "r");
		if (package_list == NULL)
			return -1;

		while ( fgets(line, BUFSIZE, package_list) ) {
			line[strlen(line)-8] = '\0';

   			// Release
			cobaltrel = rindex(line, '-');
			strcpy(release, cobaltrel);
			line[strlen(line)-strlen(release)] = '\0';

			// Version
			// - some packages don't have a release numbers... we adjust
			cobaltver = rindex(line, '-');

			// Cobalt weirdness - some packages have an extra release number
			// e.g. 1.0.1-2-2124 ... we set 1.0.1-2 to be the version
			if ((cobaltver != NULL) && (strlen(cobaltver) <= 2)) {
				strcpy(cobaltextraversion, cobaltver);
				line[strlen(line)-strlen(cobaltextraversion)] = '\0';
				cobaltver = rindex(line, '-');
			}

			if ((cobaltver != NULL) && (cobaltver[1] < 'A')) {
				strcpy(version, cobaltver);
				//strcat(version, cobaltextraversion);
				cobaltextraversion[0] = '\0';
			} else {
				strcpy(version, release);
				strcpy(release, "-");
			}

			// Name
			// - Cobalt's packages have release/version twice... we adjust
			count = strlen(line);
			while (line[count] < 'a') {
				count--;
			}	
			line[count+1] = '\0';

			// Doh... some versions have letters too!  Linux must standardize
			// - if wee detect a decimal point, continue
			cobaltname = rindex(line, '.');
			if (cobaltname != NULL) {
				cobaltname = rindex(line, '-');
				if (cobaltname != NULL)
					line[strlen(line)-strlen(cobaltname)] = '\0';
			}

			fprintf(package_xml, "  <Package>\n");
			fprintf(package_xml, "   <Name>%s</Name>\n", line+16);
			fprintf(package_xml, "   <Version>%s</Version>\n", version+1);
			fprintf(package_xml, "   <Release>%s</Release>\n", release+1);
			fprintf(package_xml, "  </Package>\n");
		}
	}

	fprintf(package_xml, " </Packages>\n");
	fprintf(package_xml, "</Software>");
	pclose(package_list);
	fclose(package_xml);
	rename("packages.xml.new", "packages.xml");
	return 0;
}


char* Software_InstallRPM(char* filename)
{
	return "not implemented";
}


char* Software_InstallPKG(char* filename)
{
	FILE* status;
	char command[200];
	char line[2001];

	strcpy(command, "./software-wrapper -i ");
	strcat(command, filename);
	status = popen(command, "r");
		
	if (status == NULL)
		return "error: wrapper did not run";

	while (fgets(line, 2000, status)) {
		strcat(payload, line);
	}
	pclose(status);

	return payload;
}


char* Software_UnInstallPKG(char* filename)
{
	FILE* status;
	char command[200];
	char line[2001];

	strcpy(command, "./software-wrapper -e ");
	strcat(command, filename);
	status = popen(command, "r");
		
	if (status == NULL)
		return "error: wrapper did not run";

	while (fgets(line, 2000, status)) {
		strcat(payload, line);
	}
	pclose(status);

	return payload;
}


char* Software_ResetPKGDir()
{
	FILE* status;
	char command[200];
	char line[200];

	strcpy(command, "./software-wrapper -r none");
	status = popen(command, "r");
		
	if (status == NULL)
		return "error: wrapper did not run";

	while (fgets(line, 200, status)) {
		strcat(payload, line);
	}
	pclose(status);

	return payload;
}


void constructor()
{}

void deconstructor()
{}
