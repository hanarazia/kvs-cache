#include "kvs_fifo.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct fifo_cache_entry {
  char key[KVS_KEY_MAX];
  char val[KVS_VALUE_MAX];
  int dob;
  int modified;
};

typedef struct fifo_cache_entry fifo_cache_entry;

struct kvs_fifo {
  kvs_base_t* kvs_base;
  int capacity;
  fifo_cache_entry* cache;
  int curr_time;
  int modified_flag;
};

kvs_fifo_t* kvs_fifo_new(kvs_base_t* kvs, int capacity) {
  kvs_fifo_t* kvs_fifo = malloc(sizeof(kvs_fifo_t));
  kvs_fifo->kvs_base = kvs;
  kvs_fifo->capacity = capacity;
  kvs_fifo->curr_time = 1;
  kvs_fifo->modified_flag = 1;

  kvs_fifo->cache = malloc(sizeof(fifo_cache_entry) * capacity);
  for (int i = 0; i < capacity; i++) {
    kvs_fifo->cache[i].dob = 0;
    kvs_fifo->cache[i].modified = 0;
  }

  return kvs_fifo;
}

void kvs_fifo_free(kvs_fifo_t** ptr) {
  kvs_fifo_t* kvs_fifo = *ptr;
  free(kvs_fifo->cache);
  free(kvs_fifo);
  kvs_fifo = NULL;
}

int kvs_fifo_set(kvs_fifo_t* kvs_fifo, const char* key, const char* value) {
  kvs_fifo->curr_time = kvs_fifo->curr_time + 1;

  // Check if we already have this key in cache
  for (int i = 0; i < kvs_fifo->capacity; i++) {
    if (kvs_fifo->cache[i].dob && !strcmp(key, kvs_fifo->cache[i].key)) {
      strcpy(kvs_fifo->cache[i].val, value);
      kvs_fifo->cache[i].modified = kvs_fifo->modified_flag;
      return SUCCESS;
    }
  }

  // Key was not found in cache, see if we can find empty location in cache
  for (int i = 0; i < kvs_fifo->capacity; i++) {
    if (kvs_fifo->cache[i].dob == 0) {
      strcpy(kvs_fifo->cache[i].key, key);
      strcpy(kvs_fifo->cache[i].val, value);
      kvs_fifo->cache[i].dob = kvs_fifo->curr_time;
      kvs_fifo->cache[i].modified = kvs_fifo->modified_flag;
      return SUCCESS;
    }
  }

  // No empty location found, evict entry with smallest number for dob
  int smallest_dob = kvs_fifo->curr_time;
  int evict_index = -1;
  for (int i = 0; i < kvs_fifo->capacity; i++) {
    if (kvs_fifo->cache[i].dob < smallest_dob) {
      smallest_dob = kvs_fifo->cache[i].dob;
      evict_index = i;
    }
  }

  if (kvs_fifo->cache[evict_index].modified) {
    kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->cache[evict_index].key,
                 kvs_fifo->cache[evict_index].val);
  }

  strcpy(kvs_fifo->cache[evict_index].key, key);
  strcpy(kvs_fifo->cache[evict_index].val, value);
  kvs_fifo->cache[evict_index].dob = kvs_fifo->curr_time;
  kvs_fifo->cache[evict_index].modified = kvs_fifo->modified_flag;
  return SUCCESS;
}

int kvs_fifo_get(kvs_fifo_t* kvs_fifo, const char* key, char* value) {
  // Check if key matches with existing key in cache
  for (int i = 0; i < kvs_fifo->capacity; i++) {
    if (kvs_fifo->cache[i].dob && !strcmp(key, kvs_fifo->cache[i].key)) {
      strcpy(value, kvs_fifo->cache[i].val);
      return SUCCESS;
    }
  }

  // Check if key exists on disk
  kvs_base_get(kvs_fifo->kvs_base, key, value);

  if (strcmp(value, "")) {
    kvs_fifo->modified_flag = 0;
    kvs_fifo_set(kvs_fifo, key, value);
    kvs_fifo->modified_flag = 1;
  }
  return SUCCESS;
}

int kvs_fifo_flush(kvs_fifo_t* kvs_fifo) {
  for (int i = 0; i < kvs_fifo->capacity; i++) {
    if (kvs_fifo->cache[i].dob && kvs_fifo->cache[i].modified) {
      kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->cache[i].key,
                   kvs_fifo->cache[i].val);
      kvs_fifo->cache[i].dob = 0;
    }
  }
  return SUCCESS;
}
