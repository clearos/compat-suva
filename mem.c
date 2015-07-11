// Memory management routines
// $Id: mem.c,v 1.24 2004/03/25 20:43:14 darryl Exp $
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#include "suva.h"
#include "suva_conf.h"
#include "sutil.h"
#include "mem.h"

extern int errno;

// Memory pointer hash table:
//static MemList **pMemList = NULL;
static MemList *pMemList[MEM_HASH_SIZE];

// Memory type/category hash table:
//static MemInfo **pMemInfo = NULL;
static MemInfo *pMemInfo[MEM_HASH_SIZE];

//void mem_init(MemoryCTX *p_ctx)
void mem_init(void)
{
//	pMemList = p_ctx->pMemList;
//	pMemInfo = p_ctx->pMemInfo;

	memset(pMemList, '\0', sizeof(pMemList));
	memset(pMemInfo, '\0', sizeof(pMemInfo));
}

int mem_add_type(unsigned int type, char *name)
{
	unsigned int hash;
	MemInfo *p_new = NULL, *p_info = mem_find_type(type);

	// 0 is reserved:
	if(!type)
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: memory types must be greater than zero.", __func__);
#endif

		return MEM_RES_BADTYPE;
	}

	// Make sure type doesn't already exist:
	if(p_info)
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: memory type \"%s\" already exists.", __func__, p_info->name);
#endif

		return MEM_RES_EXISTS;
	}

	hash = mem_hash(&type, sizeof(unsigned int));

	// Allocate new MemInfo struct:
	if(!(p_new = (MemInfo *)malloc(sizeof(MemInfo))))
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: failed to allocate %d byte(s) for MemInfo entry: %s.",
			__func__, sizeof(MemInfo), strerror(errno));
#endif

		return MEM_RES_SYSCALL;
	}

	// Initialize new MemInfo pointer:
	memset(p_new, '\0', sizeof(MemInfo));
	p_new->type = type;
	strncpy(p_new->name, name, MEM_MAX_NAME - 1);

	p_info = pMemInfo[hash];
	pMemInfo[hash] = p_new;
	p_new->p_next = p_info;

#ifndef SCO_MEM_HUSH
	output(5, "%s: added new memory category/type \"%s\".",
		__func__, p_new->name);
#endif

	return MEM_RES_OK;
}

// Delete memory category/type:
int mem_del_type(unsigned int type)
{
	MemList *p_list;
	unsigned int i, hash;
	MemInfo *p_tmp = NULL, *p_info = mem_find_type(type);

	if(!p_info)
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: can't find memory category/type: %d", __func__, type);
#endif
		return MEM_RES_EXISTS;
	}

	// Return if this type is still in use:
	for(i = 0u; i < MEM_HASH_SIZE; i++)
	{
		for(p_list = pMemList[i]; p_list; p_list = p_list->p_next)
		{
			if(p_list->p_info->type == type)
			{
#ifndef SCO_MEM_HUSH
				output(1,
					"%s: the memory category/type \"%s\" is in use.",
					__func__, p_info->name);
#endif
				return MEM_RES_INUSE;
			}
		}
	}

	hash = mem_hash(&type, sizeof(unsigned int));

	for(p_info = pMemInfo[hash]; p_info; p_info = p_info->p_next)
	{
		if(p_info->type == type)
		{
			if(!p_tmp)
				pMemInfo[hash] = p_info->p_next;
			else
				p_tmp->p_next = p_info->p_next;
#ifndef SCO_MEM_HUSH
				output(5,
					"%s: deleted memory category/type \"%s\".",
					__func__, p_info->name);
#endif
				// Free MemInfo pointer:
				free(p_info);
				break;
			}

		p_tmp = p_info;
	}

	return MEM_RES_OK;
}

// Retrieve memory category/type name:
char *mem_get_type(unsigned int type)
{
	MemInfo *p_info = mem_find_type(type);

	if(!p_info)
	{
#ifndef SCO_MEM_HUSH
		output(10,
			"%s: can't find memory category/type: %d", __func__, type);
#endif

		return NULL;
	}

	return p_info->name;
}

// Allocate new memory:
void *mem_alloc(unsigned int type, unsigned int size)
{
	unsigned int hash;
	MemList *p_new = NULL, *p_list = NULL;
	MemInfo *p_info = mem_find_type(type);

	if(!p_info)
	{
#ifndef SCO_MEM_HUSH
		output(5,
			"%s: can't find memory category/type: %d", __func__, type);
#endif
		return NULL;
	}

	// Allocate new list entry:
	if(!(p_new = (MemList *)malloc(sizeof(MemList))))
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: failed to allocate %d byte(s) for MemList entry: %s.",
			__func__, sizeof(MemList), strerror(errno));
#endif
		return NULL;
	}

	// Initialize new list entry:
	memset(p_new, '\0', sizeof(MemList));
	p_new->p_info = p_info;
	p_new->size = size;

	// Get some core...
	if(!(p_new->p_ptr = malloc(size)))
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: failed to allocate %d byte(s): %s.",
			__func__, size, strerror(errno));
#endif
		free(p_new);
		return NULL;
	}

#ifdef MEM_ZEROFILL
	memset(p_new->p_ptr, '\0', size);
#endif

	hash = mem_hash(&p_new->p_ptr, sizeof(void *));

	p_list = pMemList[hash];
	pMemList[hash] = p_new;
	p_new->p_next = p_list;

#ifndef SCO_MEM_HUSH
	output(5, "%s: allocated %d byte(s) @ %p of \"%s\" memory (hash: %u).",
		__func__, p_new->size, p_new->p_ptr, p_info->name, hash);
#endif
	return p_new->p_ptr;
}

// Allocate new array:
void *mem_calloc(unsigned int type, unsigned int members, unsigned int size)
{
	unsigned int hash;
	MemList *p_new = NULL, *p_list = NULL;
	MemInfo *p_info = mem_find_type(type);

	if(!p_info)
	{
#ifndef SCO_MEM_HUSH
		output(5,
			"%s: can't find memory category/type: %d", __func__, type);
#endif
		return NULL;
	}

	// Allocate new list entry:
	if(!(p_new = (MemList *)malloc(sizeof(MemList))))
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: failed to allocate %d byte(s) for MemList entry: %s.",
			__func__, sizeof(MemList), strerror(errno));
#endif
		return NULL;
	}

	// Initialize new list entry:
	memset(p_new, '\0', sizeof(MemList));
	p_new->p_info = p_info;
	p_new->size = size;

	// Get some core...
	if(!(p_new->p_ptr = calloc(members, size)))
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: failed to allocate %u x %u byte(s): %s.",
			__func__, members, size, strerror(errno));
#endif
		free(p_new);
		return NULL;
	}

#ifdef MEM_ZEROFILL
	memset(p_new->p_ptr, '\0', size * members);
#endif

	hash = mem_hash(&p_new->p_ptr, sizeof(void *));

	p_list = pMemList[hash];
	pMemList[hash] = p_new;
	p_new->p_next = p_list;

#ifndef SCO_MEM_HUSH
	output(5, "%s: allocated %u x %u byte(s) @ %p of \"%s\" memory (hash: %u).",
		__func__, p_new->members, p_new->size, p_new->p_ptr, p_info->name, hash);
#endif
	return p_new->p_ptr;
}

// Reallocate existing memory:
void *mem_realloc(void *p_ptr, unsigned int new_size)
{
	unsigned int hash;
	void *p_tmp = NULL;
	MemList *p_list, *p_last = NULL, *p_entry = mem_find_ptr(p_ptr);

	if(!p_entry)
	{
#ifndef SCO_MEM_HUSH
		output(5, "%s: can't find pointer in list: %p",
			__func__, p_ptr);
#endif

		return NULL;
	}

	// Make sure we're not trying to reallocate a stored pointer:
	if((p_entry->flags & MEM_FLG_STORED) == MEM_FLG_STORED)
	{
#ifndef SCO_MEM_HUSH
		output(1, "%s: can't reallocate stored pointers.", __func__);
#endif

		return NULL;
	}

	// Reallocate pointer to new_size bytes:
	if(!(p_tmp = realloc(p_ptr, new_size)))
	{
#ifndef SCO_MEM_HUSH
		output(1, "%s: failed to reallocate %d byte(s) of"
			" \"%s\" memory: %s.", __func__, new_size, p_entry->p_info->name,
			strerror(errno));
#endif
		return NULL;
	}
#ifdef MEM_ZEROFILL
	// If we've reallocated more memory, zero newly allocated bytes:
	if(new_size > p_entry->size)
	{
		register unsigned int i;
		register unsigned char *p_data = (unsigned char *)p_tmp;

		for(i = p_entry->size; i < new_size; i++)
			p_data[i] = '\0';
	}
#endif
	if(p_tmp != p_entry->p_ptr)
	{
		hash = mem_hash(&p_entry->p_ptr, sizeof(void *));

		for(p_list = pMemList[hash]; p_list; p_list = p_list->p_next)
		{
			if(p_list->p_ptr == p_entry->p_ptr)
			{
				if(!p_last)
					pMemList[hash] = p_entry->p_next;
				else
					p_last->p_next = p_entry->p_next;

				break;
			}

			p_last = p_list;
		}

		hash = mem_hash(&p_tmp, sizeof(void *));

		p_list = pMemList[hash];
		pMemList[hash] = p_entry;

		p_entry->p_ptr = p_tmp;
		p_entry->p_next = p_list;
	}

#ifndef SCO_MEM_HUSH
	output(5, "%s: reallocated %d byte(s) @ %p (%p) of \"%s\" memory.",
		__func__, new_size, p_tmp, p_entry->p_ptr, p_entry->p_info->name);
#endif

	// Set new size:
	p_entry->size = new_size;

	// Return (new) pointer:
	return p_tmp;
}

// Store a pointer:
int mem_store(void *p_ptr, unsigned int type, unsigned int size)
{
	unsigned int hash;
	MemInfo *p_info = mem_find_type(type);
	MemList *p_new = mem_find_ptr(p_ptr), *p_list;

	if(!p_ptr)
	{
#ifndef SCO_MEM_HUSH
		output(1, "%s: supplied pointer is NULL.", __func__);
#endif
		return MEM_RES_ISNULL;
	}

	// Make sure pointer isn't already in pMemList:
	if(p_new)
	{
#ifndef SCO_MEM_HUSH
		output(1, "%s: pointer already exists in list: %p.",
			__func__, p_ptr);
#endif
		return MEM_RES_EXISTS;
	}

	if(!p_info)
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: can't find memory category/type: %d", __func__, type);
#endif

		return MEM_RES_EXISTS;
	}

	// Allocate new list entry:
	if(!(p_new = (MemList *)malloc(sizeof(MemList))))
	{
#ifndef SCO_MEM_HUSH
		output(1,
			"%s: failed to allocate %d byte(s) for MemList entry: %s.",
			__func__, sizeof(MemList), strerror(errno));
#endif

		return MEM_RES_SYSCALL;
	}

	// Initialize new list entry:
	memset(p_new, '\0', sizeof(MemList));

	// Set new MemList p_ptr, flags, p_info, and size:
	p_new->p_ptr = p_ptr;
	p_new->flags = MEM_FLG_STORED;
	p_new->p_info = p_info;
	p_new->size = size;

	// Add new list pointer to pMemList:
	hash = mem_hash(&p_ptr, sizeof(void *));

	p_list = pMemList[hash];
	pMemList[hash] = p_new;
	p_new->p_next = p_list;

#ifndef SCO_MEM_HUSH
	output(5,
		"%s: stored pointer to %d byte(s) of \"%s\" memory.",
		__func__, p_new->size, p_info->name);
#endif

	return MEM_RES_OK;
}

// Free previously allocated memory:
void mem_free(void *p_ptr)
{
	unsigned int hash;
	MemList *p_list, *p_tmp = NULL, *p_entry = mem_find_ptr(p_ptr);

	if(!p_entry)
	{
#ifndef SCO_MEM_HUSH
		output(1, "%s: pointer doesn't exist in list: %p.",
			__func__, p_ptr);
#endif
		return;
	}

	hash = mem_hash(&p_ptr, sizeof(void *));

	for(p_list = pMemList[hash]; p_list; p_list = p_list->p_next)
	{
		if(p_list->p_ptr == p_entry->p_ptr)
		{
			if(!p_tmp)
				pMemList[hash] = p_list->p_next;
			else
				p_tmp->p_next = p_list->p_next;

			break;
		}

		p_tmp = p_list;
	}

	// If this pointer's type has a free() callback set, call it:
	if(p_entry->p_info->cb_free)
	{
		p_entry->p_info->cb_free(p_entry->p_info->type, p_ptr, p_entry->size);

#ifndef SCO_MEM_HUSH
		output(10, "%s: called custom callback free() for "
			"%d byte(s) of \"%s\" memory.", __func__, p_entry->size, p_entry->p_info->name);
#endif
	}
	else if(!(p_entry->flags & MEM_FLG_STORED))
	{
		// Clear memory first (worth the extra time - secure):
		memset(p_ptr, '\0', p_entry->size);

		// Free pointer normally if it isn't a stored pointer:
		free(p_ptr);

#ifndef SCO_MEM_HUSH
		output(5, "%s: freed %d byte(s) @ %p of \"%s\" memory.",
			__func__, p_entry->size, p_ptr, p_entry->p_info->name);
#endif
	}
	else
	{
		// Remind user that no action was taken for stored pointer:
#ifndef SCO_MEM_HUSH
		output(10, "%s: not freeing stored pointer to %d "
			"byte(s) of \"%s\" memory.", __func__, p_entry->size,
			p_entry->p_info->name);
#endif
	}

	// Free list entry:
	free(p_entry);

	return;
}

// Free all memory of specified category/type:
void mem_free_all(unsigned int type)
{
	MemList *p_list;
	MemInfo *p_info;
	void *p_tmp = NULL;
	unsigned int i, list_type;

#ifndef SCO_MEM_HUSH
	if(!type)
		output(5, "%s: freeing all allocated memory.",
			__func__);
#endif

	// Call mem_free() for every pointer in pMemList:
	for(i = 0u; i < MEM_HASH_SIZE; i++)
	{
		for(p_list = pMemList[i]; p_list; )
		{
			p_tmp = p_list->p_ptr;
			list_type = p_list->p_info->type;
			p_list = p_list->p_next;

			if(!type || list_type == type)
				mem_free(p_tmp);
		}
	}

	if(type)
		return;

	// Call mem_del_type() for every memory category/type:
	for(i = 0u; i < MEM_HASH_SIZE; i++)
	{
		for(p_info = pMemInfo[i]; p_info; )
		{
			list_type = p_info->type;
			p_info = p_info->p_next;
			
			mem_del_type(list_type);
		}
	}

	return;
}

// Get total memory size (in bytes) of specified category/type:
unsigned int mem_get_size(unsigned int type)
{
	register MemList *p_list;
	register unsigned int i, total = 0;

	// Seek through entire memory list:
	for(i = 0u; i < MEM_HASH_SIZE; i++)
	{
		for(p_list = pMemList[i]; p_list; p_list = p_list->p_next)
		{
			if(p_list->p_info->type == type)
				total += p_list->size;
		}
	}

	// Return memory category/type byte count:
	return total;
}

// Set custom callback function to free memory of a particular category/type:
void mem_cb_free(unsigned int type, mem_cb_free_t handler)
{
	MemInfo *p_info = mem_find_type(type);

	// Make the the memory category/type exists:
	if(p_info)
	{
		// Set custom free() callback for this memory category/type:
		p_info->cb_free = handler;

#ifndef SCO_MEM_HUSH
		output(5, "%s: set custom free callback for memory"
			" category/type \"%s\".", __func__, p_info->name);
#endif
	}

	return;
}

#ifndef SCO_SUVA_API
void mem_plot(void)
{
	MemInfo *p_info;
	MemList *p_list;
	FILE *h_f = NULL;
	time_t timestamp;
	char file[MAX_PATH];
	unsigned int i, j, total;
	static time_t start_time = 0;

	if(!start_time)
		start_time = time(NULL);

	timestamp = time(NULL) - start_time;

	for(i = 0u; i < MEM_HASH_SIZE; i++)
	{
		for(p_info = pMemInfo[i]; p_info; p_info = p_info->p_next)
		{
			sprintf(file, "/tmp/%s_%06d_%02d.dat", __func__, getpid(), p_info->type);

			if(!(h_f = fopen(file, "a")))
			{
				output(1, "Unable to open memory plot file: %s: %s.",
					file, strerror(errno));

				continue;
			}

			total = 0u;

			for(j = 0u; j < MEM_HASH_SIZE; j++)
			{
				for(p_list = pMemList[j]; p_list; p_list = p_list->p_next)
				{
					if(p_list->p_info != p_info)
						continue;

					total += p_list->size;
				}
			}

			fprintf(h_f, "%u %u\n", timestamp, total);
			fclose(h_f);
		}
	}
}
#endif

// Dump all allocated memory (for debug):
#ifndef SCO_SUVA_API
char **mem_dump(unsigned int flags)
{
	MemInfo *p_info;
	MemList *p_list;
	char **buffer = NULL;
	unsigned int i, j, counter = 0u;
	unsigned long sub_total = 0u, total = 0u, mem_total = 0u;

	// Display field names:
	if(!(flags & DMP_SUMMARY))
	{
		buffer = mem_buffer_line(buffer, "%-32s   %-10s %-10s %-5s",
			"Memory category/type", "Address", "Size", "Flags");
		buffer = mem_buffer_line(buffer, "----------------------------------"
			" ---------- ---------- -----");
	}
	else
	{
		buffer = mem_buffer_line(buffer, "%-32s   %-10s %-10s",
			"Memory category/type", "Count", "Total");
		buffer = mem_buffer_line(buffer, "----------------------------------"
			" ---------- ----------");
	}

	// Loop over pMemInfo list:
	for(i = 0u; i < MEM_HASH_SIZE; i++)
	{
		for(p_info = pMemInfo[i]; p_info; p_info = p_info->p_next)
		{
			// Skip categories with 0 allocations...
			if(!mem_get_size(p_info->type))
				continue;
	
			// Display memory category/type name:
			if(!(flags & DMP_SUMMARY))
				buffer = mem_buffer_line(buffer, "  %-32s", p_info->name);

			mem_total += sizeof(MemInfo);

			for(j = 0u; j < MEM_HASH_SIZE; j++)
			{
				for(p_list = pMemList[j]; p_list; p_list = p_list->p_next)
				{
					char mflags[] = { "---" };

					// Display memory pointers of type p_info:
					if(p_list->p_info != p_info)
						continue;

					mem_total += sizeof(MemList);

					// Set mflags:
					// s-- Stored pointer.
					if((p_list->flags & MEM_FLG_STORED) == 
						MEM_FLG_STORED)
					{
						mflags[0] = 's';
					}

					// -c- Custom free() callback is set.
					if(p_info->cb_free)
						mflags[1] = 'c';

					// Display pointer information line:
					if(!(flags & DMP_SUMMARY))
					{
						buffer = mem_buffer_line(buffer, "  %32s %-10p %10d  %s", "",
							p_list->p_ptr, p_list->size, mflags);
					}

					// Add to sub-total:
					counter++;
					sub_total += (unsigned long)p_list->size;
				}
			}

			// Display sub-total if greater than zero:
			if((flags & DMP_SUMMARY))
			{
				buffer = mem_buffer_line(buffer, "  %-32s %10u %10u",
					p_info->name, counter, sub_total);
			}
			else if(sub_total)
			{
				buffer = mem_buffer_line(buffer,
					"  %32s ---------- ----------", "");
	
				buffer = mem_buffer_line(buffer,
					"  %32s %10u %10u", "", counter, sub_total);
			}
	
			// Add to total and clear sub-total:
			total += sub_total;
			sub_total = counter = 0u;
		}
	}

	// Display grand total:
	buffer = mem_buffer_line(buffer,
		"  %32s %10s ----------", "", "");
	buffer = mem_buffer_line(buffer,
		"  %-32s %10s %10d", "Memory Management", "", mem_total);
	buffer = mem_buffer_line(buffer,
		"---------------------------------- ---------- ---------- %s", 
		(flags & DMP_SUMMARY) ? "" : "-----");
	buffer = mem_buffer_line(buffer, "  %32s %10s %10ld", "", "", total);

	if(!(flags & DMP_BUFFER))
	{
		char *l;
		int i = 0;

		while((l = buffer[i++]))
			output(1, l);

		array_free(buffer);
		buffer = NULL;
	}

	return buffer;
}

char **mem_buffer_line(char **buffer, const char *format, ...)
{       
	va_list ap;
	char line[MAX_BUFFER];

	if(!format)
		return buffer;

	va_start(ap, format);
	vsnprintf(line, MAX_BUFFER, format, ap);
	va_end(ap);

	return array_push(buffer, line);
}
#endif // ifndef SCO_SUVA_API

// Create hash value:
unsigned int mem_hash(void *p_key, unsigned int len)
{
	unsigned int i = 0u, hash = 0u;
	unsigned char *p_ukey = (unsigned char *)p_key;

	// TODO: Better hashing...
	for(i = 0u; i < len * 8; i += 8)
		hash += (*p_ukey >> i);

	return hash % MEM_HASH_SIZE;
}

// Find memory pointer in pMemList:
MemList *mem_find_ptr(void *p_ptr)
{
	unsigned int hash;
	register MemList *p_list;

	hash = mem_hash(&p_ptr, sizeof(void *));

	for(p_list = pMemList[hash]; p_list && p_list->p_ptr != p_ptr; p_list = p_list->p_next);

	if(!p_list)
	{
		unsigned int i;
#ifndef SCO_MEM_HUSH
		output(5, "%s: can't find memory pointer: %p.", __func__, p_ptr);
#endif
		for(i = 0u; i < MEM_HASH_SIZE; i++)
		{
			for(p_list = pMemList[i]; p_list; p_list = p_list->p_next)
			{
				if(p_list->p_ptr != p_ptr)
					continue;
#ifndef SCO_MEM_HUSH
				output(1,
					"%s: found %p @ hash: %u instead of hash: %u.",
					__func__, p_ptr, i, hash);
#endif
				break;
			}
		}
	}

	return p_list;
}

// Find memory category/type in pMemInfo list:
MemInfo *mem_find_type(unsigned int type)
{
	unsigned int hash;
	register MemInfo *p_info;

	hash = mem_hash(&type, sizeof(unsigned int));

	for(p_info = pMemInfo[hash]; p_info && p_info->type != type; p_info = p_info->p_next);

#ifndef SCO_MEM_HUSH
	if(!p_info)
		output(10, "%s: can't find memory category/type: %d.", __func__, type);
#endif

	return p_info;
}
