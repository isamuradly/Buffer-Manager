/*
CS525: Advanced Database Organization
Fall 2021 | Group 22
Isa Muradli           (imuradli@hawk.iit.edu)       (Section 01)
Andrew Petravicius    (apetravicius@hawk.iit.edu)   (Section 01)
Christopher Sherman   (csherman1@hawk.iit.edu)      (Section 02)
*/

#include "hashtable.h"

#include <stdlib.h>

/************************************************************
 *                   helper interface                       *
 ************************************************************/

int hash(int toHash);

/************************************************************
 *                    implementation                        *
 ************************************************************/

// allocation functions

/**
hashtable_init

allocate memory for the hashtable and set defaults
*/
hashtable_t* hashtable_init(int numBuckets) {
  hashtable_t* newHashtable = (hashtable_t*) malloc(sizeof(hashtable_t));
  newHashtable->len = 0;
  newHashtable->numBuckets = numBuckets;
  newHashtable->buckets = (hashtable_node_t**) malloc(
  numBuckets * sizeof(hashtable_node_t*)
  );

  // all bucket pointers should be null pointers at the start
  for(int i = 0; i < numBuckets; ++i) {
    newHashtable->buckets[i] = NULL;
  }

  return newHashtable;
}

/**
hashtable_free

free all space allocated by the hashtable
*/
void hashtable_destroy(hashtable_t* ht) {
  int numBuckets = ht->numBuckets;
  hashtable_node_t* current = NULL;
  hashtable_node_t* next = NULL;

  // free each linked list bucket
  for(int i = 0; i < numBuckets; ++i) {
    next = ht->buckets[i];
    while(next) {
      current = next;
      next = current->next;
      free(current);
    }
  }

  // free buckets array
  free(ht->buckets);

  // free the hashtable
  free(ht);
  return;
}

// access functions

/**
hashtable_put

add key, value pair to the hashtable; error if the key already is in the
hashtable by returning 0
*/
int hashtable_put(hashtable_t* ht, int key, int value) {
  int bucketIdx = hash(key) % ht->numBuckets;
  hashtable_node_t* next = ht->buckets[bucketIdx];
  hashtable_node_t* current = NULL;

  while(next) {
    current = next;
    if(current->key == key) {
      return 0;
    }
    next = current->next;
  } // at this point, next is a null pointer

  // current == NULL means ht->buckets[bucketIdx] still null
  if(!current) {
    ht->buckets[bucketIdx] = (hashtable_node_t*) malloc(sizeof(hashtable_node_t));
    current = ht->buckets[bucketIdx];
  }
  else {
    current->next = (hashtable_node_t*) malloc(sizeof(hashtable_node_t));
    current = current->next;
  }

  current->key = key;
  current->value = value;
  current->next = NULL;

  return 1;
}

/**
hashtable_get

return the value mapped to key; return -1 if not found
*/
int hashtable_get(hashtable_t* ht, int key) {
  int bucketIdx = hash(key) % ht->numBuckets;
  hashtable_node_t* next = ht->buckets[bucketIdx];
  hashtable_node_t* current = NULL;

  while(next) {
    current = next;
    if(current->key == key) {
      return current->value;
    }
    next = current->next;
  } // at this point, next is a null pointer

  return -1;
}

/**

*/
int hashtable_remove(hashtable_t* ht, int key) {
  hashtable_node_t* current;
  int bucketIdx = hash(key) % ht->numBuckets;
  current = ht->buckets[bucketIdx];

  if (current == NULL){
      return 0;
  }

  if(current->next == NULL){
    if(current->key == key){
      ht->buckets[bucketIdx] = NULL;
      free(current);
      return 1;
    }
    return 0;
  }
  else {
    if(current->key == key) {
      ht->buckets[bucketIdx] = current->next;
      free(current);
      return 1;
    }
    while(current->next) {
      if(current->next->key == key) {
        hashtable_node_t* temp = current->next;
        current->next = current->next->next;
        free(temp);
        return 1;
      }
      current = current->next;
    }
  return 0;
  }
}

/**
hashtable_length

returns the length of the hashtable
*/
int hashtable_length(hashtable_t* ht) {
  return ht->len;
}

/************************************************************
*                helper implementation                     *
************************************************************/

/**
hash

maps the input a number in the hashtable
*/
int hash(int toHash) {
  return toHash;
}
