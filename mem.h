// Memory management header
// $Id: mem.h,v 1.17 2003/04/26 04:28:07 darryl Exp $
#ifndef _MEM_H
#define _MEM_H

//! Define to zero-fill (re)allocated memory
#define MEM_ZEROFILL

//! Maximum type/category name length
#define MEM_MAX_NAME	32

//! Bad memory category or type
#define MEM_RES_BADTYPE	-6

//! NULL value
#define MEM_RES_ISNULL	-5

//! Pointer is a stored pointer
#define MEM_RES_STORED	-4

//! Category or type is still in use
#define MEM_RES_INUSE	-3

//! System call error
#define MEM_RES_SYSCALL	-2

//! Pointer or category already exists in list
#define MEM_RES_EXISTS	-1

//! Operation successful
#define MEM_RES_OK		0

//! Stored user supplied pointer
#define MEM_FLG_STORED	1

//! Shared memory segment (not implemented, yet)
#define MEM_FLG_SHARED	2

//! Buffer mem_dump() output
#define DMP_BUFFER		1

//! Summarize mem_dump() output
#define DMP_SUMMARY		2

//! Custom free call-back type
typedef void (*mem_cb_free_t)(int, void *, int);

//! Memory message call-back type
typedef void (*mem_cb_message_t)(int, char *);

//! The memory type information structure/typedef:
//! This structure holds information about user-definable memory categories or
//! 'types'
typedef struct MemInfo_t
{
	//! Unique memory category/type id
	unsigned int type;

	//! Human-readable category/type name
	char name[MEM_MAX_NAME];

	//! Custom free callback handler
	mem_cb_free_t cb_free;

	//! Next MemInfo structure
	struct MemInfo_t *p_next;
} MemInfo;

//! Memory allocation list structure
typedef struct MemList_t
{
	//! Pointer to user-data
	void *p_ptr;

	//! Memory chunk's size (in bytes)
	unsigned int size;

	//! Memory chunk's flags
	unsigned int flags;

	//! Pointer to memory category/type
	struct MemInfo_t *p_info;

	//! Next MemList structure
	struct MemList_t *p_next;
} MemList;

//! Memory manager context:
typedef struct MemoryCTX_t
{
	// Memory pointer hash table:
	MemList *pMemList[MEM_HASH_SIZE];

	// Memory type/category hash table:
	MemInfo *pMemInfo[MEM_HASH_SIZE];
} MemoryCTX;

// Public function prototypes:
///////////////////////////////////////////////////////////////////////////////
//!Initialize hash tables
//void mem_init(MemoryCTX *p_ctx);
void mem_init(void);

//! Define new memory category/type:
//! NOTE: Valid category/type ids must be greater than 0 because 0 is reserved
//! for use in mem_free_all()
int mem_add_type(unsigned int type, char *name);

//! Delete existing memory category/type:
//! This function will not remove the requested category/type if there are
//! pointers found elsewhere in MemList of the same category/type.  Is isn't
//! necessary to call this function for every type you add.  Just make sure
//! you call mem_free_all() before your application terminates so all previously
//! allocated types get freed
int mem_del_type(unsigned int type);

//! Retrieve memory category/type name
char *mem_get_type(unsigned int type);

//! Allocate new memory chunk
void *mem_alloc(unsigned int type, unsigned int size);

//! Allocate new memory array
void *mem_calloc(unsigned int type, unsigned int members, unsigned int size);

//! Reallocate existing memory chunk
void *mem_realloc(void *p_ptr, unsigned int new_size);

//! Store an arbitrary pointer in the MemList:
//! You need to supply a type id and you should supply the size of memory p_ptr
//! points to zero, but it's not required.  If the pointer is associated with a
//! memory category/type that has a custom free callback function set, mem_free()
//! will call it, otherwise mem_free() won't try to free() it like mem_alloc()'d
//! memory
int mem_store(void *p_ptr, unsigned int type, unsigned int size);

//! Free previously allocated memory
void mem_free(void *p_ptr);

//! Free all memory of specified category/type:
//! This function will free *all* memory if 'type' is false (0)
void mem_free_all(unsigned int type);

//! Get total memory size (in bytes) of specified category/type
unsigned int mem_get_size(unsigned int type);

//! Set custom callback function to free memory of a particular category/type
void mem_cb_free(unsigned int type, mem_cb_free_t handler);

//! Dump all allocated memory (for debug)
#ifndef SCO_SUVA_API
void mem_plot(void);
char **mem_dump(unsigned int flags);
char **mem_buffer_line(char **buffer, const char *format, ...);
#endif

// Private function prototypes:

//! Generate hash value
unsigned int mem_hash(void *p_key, unsigned int len);

//! Find memory pointer in pMemList
MemList *mem_find_ptr(void *p_ptr);

//! Find memory category/type in pMemInfo list
MemInfo *mem_find_type(unsigned int type);

#endif	// _MEM_H
