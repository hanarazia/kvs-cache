#include "kvs_clock.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct clock_cache_entry {
  char key[KVS_KEY_MAX];
  char val[KVS_VALUE_MAX];
  int reference;
  int modified;
  int valid;
};

typedef struct clock_cache_entry clock_cache_entry;

struct kvs_clock {
  kvs_base_t* kvs_base;
  int capacity;
  clock_cache_entry* cache;
  int modified_flag;
  int write_ptr;
};

kvs_clock_t* kvs_clock_new(kvs_base_t* kvs, int capacity) {
  kvs_clock_t* kvs_clock = malloc(sizeof(kvs_clock_t));
  kvs_clock->kvs_base = kvs;
  kvs_clock->capacity = capacity;
  kvs_clock->modified_flag = 1;
  kvs_clock->write_ptr = 0;

  kvs_clock->cache = malloc(sizeof(clock_cache_entry) * capacity);
  for (int i = 0; i < capacity; i++) {
    kvs_clock->cache[i].reference = 0;
    kvs_clock->cache[i].modified = 0;
    kvs_clock->cache[i].valid = 0;
  }

  return kvs_clock;
}

void kvs_clock_free(kvs_clock_t** ptr) {
  kvs_clock_t* kvs_clock = *ptr;
  free(kvs_clock->cache);
  free(kvs_clock);
  kvs_clock = NULL;
}

int kvs_clock_set(kvs_clock_t* kvs_clock, const char* key, const char* value) {
  // Decrement the reference of all entries
  while (1) {
    int write_index = kvs_clock->write_ptr;
    if (kvs_clock->cache[write_index].reference == 0) {
      break;
    }
    kvs_clock->cache[write_index].reference = 0;
    write_index = (write_index + 1) % kvs_clock->capacity;
    kvs_clock->write_ptr = write_index;
  }

  // Check if we already have this key in cache
  for (int i = 0; i < kvs_clock->capacity; i++) {
    if (kvs_clock->cache[i].valid && !strcmp(key, kvs_clock->cache[i].key)) {
      strcpy(kvs_clock->cache[i].val, value);
      kvs_clock->cache[i].modified = kvs_clock->modified_flag;
      kvs_clock->cache[i].reference = 1;
      return SUCCESS;
    }
  }

  // Evict entry from write_ptr
  int evict_index = kvs_clock->write_ptr;
  if (kvs_clock->cache[evict_index].modified) {
    kvs_base_set(kvs_clock->kvs_base, kvs_clock->cache[evict_index].key,
                 kvs_clock->cache[evict_index].val);
  }

  strcpy(kvs_clock->cache[evict_index].key, key);
  strcpy(kvs_clock->cache[evict_index].val, value);
  kvs_clock->cache[evict_index].reference = 1;
  kvs_clock->cache[evict_index].valid = 1;
  kvs_clock->cache[evict_index].modified = kvs_clock->modified_flag;
  return SUCCESS;
}

int kvs_clock_get(kvs_clock_t* kvs_clock, const char* key, char* value) {
  // Check if key matches with existing key in cache
  for (int i = 0; i < kvs_clock->capacity; i++) {
    if (kvs_clock->cache[i].valid && !strcmp(key, kvs_clock->cache[i].key)) {
      strcpy(value, kvs_clock->cache[i].val);
      return SUCCESS;
    }
  }

  // Check if key exists on disk
  kvs_base_get(kvs_clock->kvs_base, key, value);

  if (strcmp(value, "")) {
    kvs_clock->modified_flag = 0;
    kvs_clock_set(kvs_clock, key, value);
    kvs_clock->modified_flag = 1;
  }
  return SUCCESS;
}

int kvs_clock_flush(kvs_clock_t* kvs_clock) {
  for (int i = 0; i < kvs_clock->capacity; i++) {
    if (kvs_clock->cache[i].valid && kvs_clock->cache[i].modified) {
      kvs_base_set(kvs_clock->kvs_base, kvs_clock->cache[i].key,
                   kvs_clock->cache[i].val);
      kvs_clock->cache[i].valid = 0;
      kvs_clock->cache[i].reference = 0;
      kvs_clock->cache[i].modified = 0;
    }
  }
  return SUCCESS;
}
