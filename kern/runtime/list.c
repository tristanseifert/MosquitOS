#include <types.h>
#include "list.h"

/*
 * Traverses the list for the first free entry.
 */
static list_entry_t *find_first_free_entry(list_t *list) {
	
}

/*
 * Gets the entry at the specified index from the list, if it exists.
 */
static list_entry_t *get_index(list_t *list, unsigned int index) {

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
void list_destroy(list_t *list) {
	// Delete all entries within the list.

	// Finally free the list structure itself.
	kfree(list);
}

/*
 * Adds a list entry at the first free point in the list, then returns its
 * index.
 */
unsigned int list_add(list_t *list, void* data) {

}

/*
 * Inserts the specified data at index. If data already exists in index, it is
 * replaced if the bit LIST_OVERWRITE is set on index. Returns -1 if failure,
 * or the original index if success.
 */
unsigned int list_insert(list_t *list, void* data, unsigned int index) {

}

/*
 * Tries to retrieve the item at index from the list. Returns a pointer to its
 * data, or NULL if not found.
 */
void* list_get(list_t *list, unsigned int index) {

}

/*
 * Removes the item at index from the list, if it exists. In addition, if
 * free_ptr is true, the list will call kfree() with the list entry's data
 * pointer as the parameter in addition to freeing the list_entry_t structure.
 */
void list_delete(list_t *list, unsigned int index, bool free_ptr) {

}