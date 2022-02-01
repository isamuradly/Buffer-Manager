/*
CS525: Advanced Database Organization
Fall 2021 | Group 22
Isa Muradli           (imuradli@hawk.iit.edu)       (Section 01)
Andrew Petravicius    (apetravicius@hawk.iit.edu)   (Section 01)
Christopher Sherman   (csherman1@hawk.iit.edu)      (Section 02)
*/

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "../util/dt.h"

// Data Types and Structures
typedef struct hashtable_node_t {
  int key;
  int value;
  struct hashtable_node_t* next;
} hashtable_node_t;

typedef struct hashtable_t {
  int len;
  int numBuckets;
  hashtable_node_t** buckets;
} hashtable_t;

// allocation functions
hashtable_t* hashtable_init(int numBuckets);
void hashtable_destroy(hashtable_t* ht);

// access functions
int hashtable_put(hashtable_t* ht, int key, int value);
int hashtable_get(hashtable_t* ht, int key);
int hashtable_remove(hashtable_t* ht, int key);
int hashtable_length(hashtable_t* ht);

#endif
