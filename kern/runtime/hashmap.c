#include <types.h>

#include "hashmap.h"

/*
 * The default hash function used by the hash table implementation. Based on the
 * Jenkins Hash Function (http://en.wikipedia.org/wiki/Jenkins_hash_function)
 */
static uint32_t default_hash(char *key, size_t len) {
	uint32_t hash, i;

	for(hash = i = 0; i < len; ++i) {
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

/*
 * Allocates a hashmap that supports 256 different buckets.
 */
hashmap_t *hashmap_allocate() {
	hashmap_t *hashmap = (hashmap_t *) malloc(sizeof(hashmap_t));
	memclr(hashmap, sizeof(hashmap_t));
	hashmap->num_buckets = 256;

	// Allocate bucket structures
	hashmap_bucket_t* bucket, prev_bucket;

	for(int i = 0; i < hashmap->num_buckets; i++) {
		bucket = kmalloc(sizeof(hashmap_bucket_t));
		memclr(bucket, sizeof(hashmap_bucket_t));

		// Set the bucket's next structure
		if(prev_bucket) {
			bucket->next = prev_bucket;
		}

		// If it's the 0th bucket, set it in the hashmap
		if(i == 0) {
			hashmap->buckets = bucket;
		}

		// Store a pointer to this bucket so we can set its next pointer
		prev_bucket = bucket;
	}
}

/*
 * Releases the memory for a hashmap.
 */
void hashmap_release(hashmap_t* map) {
	hashmap_bucket_t* bucket = map->buckets;
	hashmap_data_t* data;

	// Deallocate buckets
	while(bucket != NULL) {
		data = bucket->data;

		// Deallocate the data in the bucket.
		while(data != NULL) {
			free(data->key);
			free(data);
			data = data->next;
		}
	}

	// Clear remaining memory
	free(map);
}

/*
 * Searches a bucket for a specified key.
 */
static void* hashmap_search_bucket(hashmap_bucket_t* bucket, void* key) {

}

/*
 * Inserts an item into the hashmap. If the key already exists, its data is
 * overwritten.
 */
void hashmap_insert(hashmap_t*, void* key, void* data) {

}

/*
 * Retrieves an item from the hashmap, or returns NULL if not found.
 */
void* hashmap_get(hashmap_t*, void* key) {

}

/*
 * Removes an entry from the hashmap.
 */
int hashmap_delete(hashmap_t*, void* key) {

}