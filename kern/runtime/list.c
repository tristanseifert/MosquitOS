#include <types.h>
#include "list.h"

/*
 * Traverses the list for the first free entry.
 */
static list_entry_t *find_first_free_entry(list_t *list, unsigned int* index) {
	// Create a new entry and link it to the last entry in the list.
	list_entry_t *newLast = (list_entry_t *) kmalloc(sizeof(list_entry_t));
	memclr(newLast, sizeof(list_entry_t));

	// The list does not yet contain anything
	if(!list->first && !list->last) {
		list->first = newLast;
		list->last = newLast;
	} else { // There's other items in the chain
		list->last->next = newLast;
		newLast->prev = list->last;
		list->last = newLast;
	}

	// Increment number count
	list->num_entries++;

	if(index) {
		*index = list->num_entries;
	}

	return newLast;
}

/*
 * Gets the entry at the specified index from the list, if it exists.
 */
static list_entry_t *get_index(list_t *list, unsigned int index) {
	list_entry_t *entry = list->first;

	for(int i = 0; i < index; i++) {
		if(entry->next == NULL) break;

		entry = entry->next;
	}

	return NULL;
}

/*
 * Allocates memory for a list with no entries.
 */
list_t *list_allocate() {
	list_t *list = (list_t *) kmalloc(sizeof(list_t));
	memclr(list, sizeof(list_t));

	return list;
}

/*
 * Releases all memory associated with a list.
 */
void list_destroy(list_t *list, bool freeData) {
	// Delete all entries within the list.
	list_entry_t *entry = list->first;

	while(entry) {
		entry = entry->next;
	}

	// Finally free the list structure itself.
	kfree(list);
}

/*
 * Adds a list entry at the first free point in the list, then returns its
 * index.
 */
unsigned int list_add(list_t *list, void* data) {
	unsigned int index;
	list_entry_t *entry = find_first_free_entry(list, &index);
	entry->data = data;

	return index;
}

/*
 * Inserts the specified data at index. If data already exists in index, it is
 * replaced if the bit LIST_OVERWRITE is set on index. Returns -1 if failure,
 * or the original index if success.
 */
unsigned int list_insert(list_t *list, void* data, unsigned int index) {
	list_entry_t *entry = get_index(list, index);

	if(entry) {
		if(index | LIST_OVERWRITE) {
			entry->data = data;
			return index;
		}

		return -1;
	} else {
		return -1;
	}
}

/*
 * Tries to retrieve the item at index from the list. Returns a pointer to its
 * data, or NULL if not found.
 */
void* list_get(list_t *list, unsigned int index) {
	list_entry_t *entry = get_index(list, index);
	return entry->data;
}

/*
 * Removes the item at index from the list, if it exists. In addition, if
 * free_ptr is true, the list will call kfree() with the list entry's data
 * pointer as the parameter in addition to freeing the list_entry_t structure.
 */
void list_delete(list_t *list, unsigned int index, bool free_ptr) {
	list_entry_t *entry = get_index(list, index);

	// Free data if needed
	if(free_ptr) {
		kfree(entry->data);
	}

	// Update linkage
	if(entry->prev) {
		entry->prev->next = entry->next;
	} else { // Deleting first entry in chain
		list->first = entry->next;
	}

	if(entry->next) {
		entry->next->prev = entry->prev;
	} else { // Deleting last entry in chain
		list->last = entry->prev;
	}

	// Clear memory allocated to entry
	kfree(entry);
}