// Suva configuration.
#ifndef _CONFIG_H
#define _CONFIG_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <limits.h>

#include "suva.h"

//! The install directory
#define SD_BASE				"/var/lib/suvlets"

//! Default STL port
#define DEFAULT_TPORT		1875

//! Default SCL port
#define DEFAULT_CPORT		1876

//! Maximum tolerated bad packets (before connection is dropped)
#define MAX_BADREQUESTS		10

//! Maximum STL/SCL connections per IP
#define MAX_CONNECTIONS		3

//! Maximum number of requests to queue
#define MAX_QUEUE			5

//! Number of seed bytes used for salting RSA key generation
#define MAX_RSA_SEED		1024

//! (OBSOLETE) Max packet/payload size accepted
#define MAX_PACKET			1024000

//! Maximum RAW packet read size
#define MAX_RAWPACKET		8192

//! Maximum packet fragment size
#define MAX_FRAG			8192

//! Socket sleep time in usec
#define MAX_SOCKSLEEP		256

//! Number of seconds to cache keys
#define MAX_KEYCACHE		60 * 50

//! Key polling time-out
#define MAX_KEYPOLL			60

//! SDB Hash table size
#define MAX_INDEXHASH		1024

//! Memory pointer hash table size
#define MEM_HASH_SIZE		10240

//! Maximum path length
#ifndef PATH_MAX
#define MAX_PATH			1024
#else
#define MAX_PATH			PATH_MAX
#endif

//! Maximum group/user name length (includes NULL)
#define MAX_GUID			256

//! Define this to remove assertions
#undef NDEBUG

//! Define this if you want just a client
#define SCO_CLIENT 1

//! Enable public key polling
#undef SCO_PUBKEY_POLL

//! Define this to hex dump packet contents
#undef SCO_PACKET_DUMP

//! Build a debug version of Suva (won't daemonize)
#undef SCO_SUVA_DEBUG

//! Disable all memory related output
#define SCO_MEM_HUSH 1

//! Define this to prevent root from being able to start Suva
#undef SCO_NO_ROOT

//! Define this for the Suva API
#define SCO_SUVA_API 1

// Files, directories, and device names:
#define SD_KEYS			"/keys"
#ifndef SCO_COLDFIRE
#define SD_SUVLETS		"/suvlets"
#else
#define SD_SUVLETS		"/var/suvlets"
#endif

#define SB_SUVAD		"/bin/suvad"
#define SB_SUVASU		"/bin/suvasu"

#ifndef SCO_COLDFIRE
#define SF_ENV			"/etc/env.conf"
#define SF_CONF			"/etc/suva.conf"
#define SF_HOSTKEY		"/etc/host.key"
#else
#define SF_ENV			"/var/etc/env.conf"
#define SF_CONF			"/var/etc/suva.conf"
#define SF_HOSTKEY		"/var/etc/host.key"
#endif

#define SF_UNPSOCKET		"/var/run/sipc."
#define SF_PIDLOCK		"/var/run/lock."
#define SF_SDBDSOCKET		"/var/run/sdbd.socket"
#define SF_SUVLETS		"/var/db/suvlets.dat"
#define SF_IPLIST		"/var/db/iplist.dat"
#define SF_ORGS			"/var/db/org.dat"
#define SF_STATUS		"/var/db/status.dat"
#define SF_TUNNELS		"/var/db/tunnels.dat"
#define SF_HKEYDB		"/var/db/hosts.dat"
#define SF_KEYCACHE		"/var/db/pkcache.dat"
#define SF_SERVER		"/var/db/servers.dat"
#define SF_KEYS			"/keys.dat"

#define SF_RSARAND		"/dev/random"
#define SF_AESRAND		"/dev/urandom"

//! Suva configuration structure/typedef
struct SuvaConfig_t
{
	//! Suva server STL port
	unsigned long stl_port;

	//! Suva server SCL port
	unsigned long scl_port;

	//! Maximum number of connections per IP
	unsigned int max_connect;

	//! Maximum number of bad requests (bad packets)
	unsigned int max_badrequest;

	//! Maximum packet size
	unsigned int max_packet;

	//! Maximum connections to queue
	unsigned int max_queue;

	//! RSA key bits
	int rsa_bits;

	//! Authentication time-out value (in seconds)
	int auth_timeout;

	//! Socket connection time-out value (in seconds)
	int sock_timeout;

	//! Session key time-out value (in seconds)
	int skey_timeout;

	//! Key poll time-out value (in seconds)
	int poll_timeout;

	//! Key cache time-out value (in seconds)
	int key_cache;

	//! Public key successful poll percentage
	int pkey_percent;

	//! Suva server user id/group id
//	uid_t suva_uid;
//	gid_t suva_gid;
	char suva_uid[MAX_GUID];
	char suva_gid[MAX_GUID];

	//! Suvlet user id/group id
//	uid_t suvlet_uid;
//	gid_t suvlet_gid;
	char suvlet_uid[MAX_GUID];
	char suvlet_gid[MAX_GUID];

	//! Administrative key-gen interval (in seconds)
	int akg_interval;

	//! Working key-gen interval (in seconds)
	int wkg_interval;

	//! Verbosity level
	int log_level;

	//! Syslog facility
	int log_facility;

	//! STL interface IP
	long stl_ifip;

	//! SCL interface IP
	long scl_ifip;

	//! Suva server flags
	unsigned int flags;

	//! Configuration file
	char config[MAX_PATH];

	//! Random device used for RSA key generation.
	//! /dev/random is recommended over /dev/urandom here for greater entropy
	char rsa_rand[MAX_PATH];

	//! Random device used for AES key generation.
	//! /dev/urandom is recommended because it's faster (in most cases)
	char aes_rand[MAX_PATH];
};

//! Less typing
typedef unsigned long uint32;

//! Suva key server
#define SCF_KEYSERVER	0x001

//! Suva master key server (also generates keys)
#define SCF_KEYMASTER	0x002

//! Debug mode
#define SCF_DEBUG		0x004

//! Enable tunnel support
#define SCF_TUNNEL		0x008		

//! I am suvad (for output)
#define SCF_SUVAD		0x010		

//! I am suvactl (for output)
#define SCF_SUVACTL		0x020		

//! I am suvasu (for output)
#define SCF_SUVASU		0x040

//! I am sdbd
#define SCF_SDBD		0x080

//! I am a suvlet
#define SCF_SUVLET		0x100

//! Be quiet
#define SCF_QUIET		0x200

//! Initialize configuration with compile-time defaults
//void config_init(struct SuvaConfig_t *p_config);
void config_init(void);

//! Load configuration
//int config_load(struct SuvaConfig_t *p_config, const char *config_file);
int config_load(const char *config_file);

//! Define to remove *all* text output from suvad, suvasu, and suvactl
//#define output(n1, s1, ...) "/* output(n1, s1, ...) */"

#endif // _CONFIG_H
