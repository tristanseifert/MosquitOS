/*
 * Implementation of an unsorted, unordered mutable variable-length array.
 * Automagically allocates memory for new entries and releases memory that is
 * no longer required -- also doesn't explode randomly!
 */
#ifndef LIST_H
#define LIST_H

#include <types.h>

#define LIST_OVERWRITE 0x80000000

typedef struct list_entry list_entry_t;
typedef struct list list_t;

struct list_entry {
	list_entry_t *next, *prev;
	void *data;
};

struct list {
	list_entry_t *first, *last;
	unsigned int num_entries;
};

// Allocation/deallocation functions
list_t* list_allocate();
void list_destroy(list_t*, bool);

// Data manipulation (setters and getters)
unsigned int list_add(list_t*, void*);
unsigned int list_insert(list_t*, void*, unsigned int);
void* list_get(list_t*, unsigned int);
bool list_contains(list_t*, void *);
void list_delete(list_t*, unsigned int, bool);

#endif