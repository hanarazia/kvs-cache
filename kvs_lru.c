#include "kvs_lru.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct lru_cache_entry {
  char key[KVS_KEY_MAX];
  char val[KVS_VALUE_MAX];
  int dob;
  int modified;
};

typedef struct lru_cache_entry lru_cache_entry;

struct kvs_lru {
  kvs_base_t* kvs_base;
  int capacity;
  lru_cache_entry* cache;
  int curr_time;
  int modified_flag;
};

kvs_lru_t* kvs_lru_new(kvs_base_t* kvs, int capacity) {
  kvs_lru_t* kvs_lru = malloc(sizeof(kvs_lru_t));
  kvs_lru->kvs_base = kvs;
  kvs_lru->capacity = capacity;
  kvs_lru->curr_time = 1;
  kvs_lru->modified_flag = 1;

  kvs_lru->cache = malloc(sizeof(lru_cache_entry) * capacity);
  for (int i = 0; i < capacity; i++) {
    kvs_lru->cache[i].dob = 0;
    kvs_lru->cache[i].modified = 0;
  }

  return kvs_lru;
}

void kvs_lru_free(kvs_lru_t** ptr) {
  kvs_lru_t* kvs_lru = *ptr;
  free(kvs_lru->cache);
  free(kvs_lru);
  kvs_lru = NULL;
}

int kvs_lru_set(kvs_lru_t* kvs_lru, const char* key, const char* value) {
  kvs_lru->curr_time = kvs_lru->curr_time + 1;

  // Check if we already have this key in cache
  for (int i = 0; i < kvs_lru->capacity; i++) {
    if (kvs_lru->cache[i].dob && !strcmp(key, kvs_lru->cache[i].key)) {
      strcpy(kvs_lru->cache[i].val, value);
      kvs_lru->cache[i].dob = kvs_lru->curr_time;
      kvs_lru->cache[i].modified = kvs_lru->modified_flag;
      return SUCCESS;
    }
  }

  // Key was not found in cache, see if we can find empty location in cache
  for (int i = 0; i < kvs_lru->capacity; i++) {
    if (kvs_lru->cache[i].dob == 0) {
      strcpy(kvs_lru->cache[i].key, key);
      strcpy(kvs_lru->cache[i].val, value);
      kvs_lru->cache[i].dob = kvs_lru->curr_time;
      kvs_lru->cache[i].modified = kvs_lru->modified_flag;
      return SUCCESS;
    }
  }

  // No empty location found, evict entry with smallest number for dob
  int smallest_dob = kvs_lru->curr_time;
  int evict_index = -1;
  for (int i = 0; i < kvs_lru->capacity; i++) {
    if (kvs_lru->cache[i].dob < smallest_dob) {
      smallest_dob = kvs_lru->cache[i].dob;
      evict_index = i;
    }
  }

  if (kvs_lru->cache[evict_index].modified) {
    kvs_base_set(kvs_lru->kvs_base, kvs_lru->cache[evict_index].key,
                 kvs_lru->cache[evict_index].val);
  }

  strcpy(kvs_lru->cache[evict_index].key, key);
  strcpy(kvs_lru->cache[evict_index].val, value);
  kvs_lru->cache[evict_index].dob = kvs_lru->curr_time;
  kvs_lru->cache[evict_index].modified = kvs_lru->modified_flag;
  return SUCCESS;
}

int kvs_lru_get(kvs_lru_t* kvs_lru, const char* key, char* value) {
  kvs_lru->curr_time = kvs_lru->curr_time + 1;

  // Check if key matches with existing key in cache
  for (int i = 0; i < kvs_lru->capacity; i++) {
    if (kvs_lru->cache[i].dob && !strcmp(key, kvs_lru->cache[i].key)) {
      strcpy(value, kvs_lru->cache[i].val);
      kvs_lru->cache[i].dob = kvs_lru->curr_time;
      return SUCCESS;
    }
  }

  // Check if key exists on disk
  kvs_base_get(kvs_lru->kvs_base, key, value);

  if (strcmp(value, "")) {
    kvs_lru->modified_flag = 0;
    kvs_lru_set(kvs_lru, key, value);
    kvs_lru->modified_flag = 1;
  }
  return SUCCESS;
}

int kvs_lru_flush(kvs_lru_t* kvs_lru) {
  for (int i = 0; i < kvs_lru->capacity; i++) {
    if (kvs_lru->cache[i].dob && kvs_lru->cache[i].modified) {
      kvs_base_set(kvs_lru->kvs_base, kvs_lru->cache[i].key,
                   kvs_lru->cache[i].val);
      kvs_lru->cache[i].dob = 0;
    }
  }
  return SUCCESS;
}
