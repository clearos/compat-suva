// Suva utilities
// $Id: sutil.h,v 1.12 2002/10/17 21:09:34 darryl Exp $
#ifndef _SUTIL_H
#define _SUTIL_H

// Custom assert macro...
#define ASSERT(expr) \
{ \
	if(!(expr)) \
	{ \
		output(1, "ASSERT: %s: %s(%s): %d", __FILE__, \
			__ASSERT_FUNCTION, __STRING(expr), __LINE__); \
		abort(); \
	} \
}
		
//! Process still running
#define PID_RUN		0

//! Process lock file is stale
#define PID_STALE	1

//! Create and lock pid file
int pid_lock(pid_t pid);

//! Unlock (unlink) pid file
void pid_unlock(pid_t pid);

//! Get pid lock status
int pid_status(pid_t pid);

#if defined _PACKET_DUMP || defined _SUVA_DEBUG
//! Print data in hexadecimal
void print_hex(char *header, unsigned char *s, int n);
#endif

//! Get UID for user
uid_t get_uid(const char *user_id);

//! Get GID for group:
gid_t get_gid(const char *group_id);

//! Break-up arguments into an array
char **arg_array(char **array, const char *arg);

//! Break-up directory components into an array
char **dir_array(char **array, const char *dir);

//! Load environment file into an array
char **env_array(char **array, const char *format, ...);

//! Add string to dynamic array
char **array_push(char **array, const char *string);

//! Reverse array order
char **array_reverse(char **array);

//! Free dynamic array
void array_free(char **array);

//! Handle output
void output(int level, const char *format, ...);

//! Return last output string
char *get_output(void);

//! Get IP address of an interface
unsigned long get_ifip(char *ifname);

//! Byte order swapper...
void swap(void *src, size_t len);

//! Make any name (organization, suvlet, tunnel, etc.) legal
char *legal_name(char *name, int len);

//! Make generic string legal
char *legal_string(char *name, int len);

#endif // _SUTIL_H
