#include <types.h>

#include "hashmap.h"

#define hashsize(n) (((uint32_t) 1) << (n))
#define hashmask(n) (hashsize(n) - 1)

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
 * Releases the memory associated with a a hashmap.
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
 * Searches a bucket for a specified key. Returns NULL if the key could not be
 * found, or a pointer to the data structure if found.
 */
static void* hashmap_search_bucket(hashmap_bucket_t* bucket, void* key) {
	size_t keyLength = strlen(key);

	hashmap_data_t *data = bucket->data;

	// Iterate through all data associated with this bucket.
	while(data != NULL) {
		// Compare keys
		if(memcmp(key, data->key, keyLength) == 0) {
			return data;
		}

		// Check next item in the linked list.
		data = data->next;
	}

	return NULL;
}

/*
 * Inserts an item into the hashmap. If the key already exists, its data is
 * overwritten.
 */
void hashmap_insert(hashmap_t* hashmap, void* key, void* value) {
	// Create a copy of the key.
	size_t keyLength = strlen(key);
	void* keyCopy = (void *) kmalloc(keyLength+1);

	// Calculate hash
	uint32_t hash = default_hash(keyCopy);
	hash &= hashmask(hashmap->num_buckets);

	// Locate the bucket in the hashmap.
	hashmap_bucket_t *bucket = hashmap->buckets;

	for(int i = 0; i < hash; i++) {
		// Go to the next bucket.
		bucket = bucket->next;
	}

	// Search through all data in the bucket to see if the key exists
	hashmap_data_t* data, emptyData;

	data = bucket->data;

	// If there's no data structure in this bucket, create some.
	if(data == NULL) {
		hashmap_data_t* newData = (hashmap_data_t *) kmalloc(sizeof(hashmap_data_t));
		memclr(newData, sizeof(hashmap_data_t));

		newData->data = value;
		newData->key = keyCopy;

		// Set the bucket's data structure
		bucket->data = newData;
		return;
	} else {
		// Search through the data structures instead.
		while(data != NULL) {
			if(data->key == NULL) {
				emptyData = value;
			}

			// Do the keys match?
			if(memcmp(keyCopy, data->key, keyLength) == 0) {
				// Release old data.
				free(data->data);

				// Set data.
				data->data = value;

				// We're done here.
				return;
			}
		}

		// If we have an empty data structure, stuff our data into it.
		if(emptyData) {
			emptyData->key = keyCopy;
			emptyData->data = value;
			return;
		} else { // We need to allocate a data structure
			hashmap_data_t* newData = (hashmap_data_t *) kmalloc(sizeof(hashmap_data_t));
			memclr(newData, sizeof(hashmap_data_t));

			newData->data = value;
			newData->key = keyCopy;

			// Set it as the next data structure.
			data->next = newData;
			return;
		}
	}
}

/*
 * Retrieves an item from the hashmap, or returns NULL if not found.
 */
void* hashmap_get(hashmap_t* hashmap, void* key) {
	// Calculate hash
	size_t keyLength = strlen(key);
	uint32_t hash = default_hash(key);
	hash &= hashmask(hashmap->num_buckets);

	// Locate the bucket in the hashmap.
	hashmap_bucket_t *bucket = hashmap->buckets;

	for(int i = 0; i < hash; i++) {
		// Go to the next bucket.
		bucket = bucket->next;
	}

	// Loop through the data structures in the bucket
	hashmap_data_t* data = bucket->data;

	while(data != NULL) {
		// We've found the key.
		if(memcmp(keyCopy, data->key, keyLength) == 0) {
			return data->data;
		}

		data = data->next;
	}

	// We didn't find the key in the bucket.
	return NULL;
}

/*
 * Removes an entry from the hashmap.
 */
int hashmap_delete(hashmap_t* hashmap, void* key) {
	// Calculate hash
	size_t keyLength = strlen(key);
	uint32_t hash = default_hash(key);
	hash &= hashmask(hashmap->num_buckets);

	// Locate the bucket in the hashmap.
	hashmap_bucket_t *bucket = hashmap->buckets;

	for(int i = 0; i < hash; i++) {
		// Go to the next bucket.
		bucket = bucket->next;
	}

	// Loop through the data structures in the bucket
	hashmap_data_t* data = bucket->data;

	while(data != NULL) {
		// We've found the key, so delete it.
		if(memcmp(keyCopy, data->key, keyLength) == 0) {
			free(data->key);
			data->data = NULL;
			return 0;
		}

		data = data->next;
	}

	// The key couldn't be found in the hashmap
	return ENOTFOUND;
}