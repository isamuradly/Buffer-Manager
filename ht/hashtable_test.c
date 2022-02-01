/*
CS525: Advanced Database Organization
Fall 2021 | Group 22
Isa Muradli           (imuradli@hawk.iit.edu)       (Section 01)
Andrew Petravicius    (apetravicius@hawk.iit.edu)   (Section 01)
Christopher Sherman   (csherman1@hawk.iit.edu)      (Section 02)
*/

#include "hashtable.h"

#include <stdlib.h>
#include <stdio.h>

#define NUM_BUCKETS 10

void runTests(void);
void testInitAndDestroy(void);
void testPutSameKeyTwice(void);
void testPutAndGet(void);
void testGetNonexistant(void);
void testRemovingItem(void);

int main() {
  printf("Starting Hashtable Tests!\n");
  runTests();
  printf("Testing Completed Successfully\n");
  return 1;
}

void runTests() {
  testInitAndDestroy();
  testPutSameKeyTwice();
  testPutAndGet();
  testGetNonexistant();
  testRemovingItem();

  // case 1

  hashtable_t* ht = hashtable_init(5);

  for(int i = 0; i < 100; i++) {
    hashtable_put(ht, i, i);
  }
  
  for(int i = 0; i < 100; i++) {
    hashtable_remove(ht, i);
  }

  hashtable_destroy(ht);

  // case 2

  ht = hashtable_init(5);

  for(int i = 0; i < 100; i++) {
    hashtable_put(ht, i, i);
  }

  hashtable_destroy(ht);


  return;
}

void testInitAndDestroy() {
  printf("Testing init and destroy methods... ");
  hashtable_t* ht = hashtable_init(NUM_BUCKETS);
  hashtable_destroy(ht);
  printf("success!\n");
  return;
}

void testPutSameKeyTwice() {
  printf("Testing failure upon putting same key twices... ");
  hashtable_t* ht = hashtable_init(NUM_BUCKETS);
  if(!hashtable_put(ht, 1, 2)) {
    printf("failure. put caused error on first put");
    exit(1);
  }
  if(hashtable_put(ht, 1, 2)) {
    printf("failure. put did not cause error on second identical put");
    exit(1);
  }
  hashtable_destroy(ht);
  printf("success!\n");
}

void testPutAndGet() {
  printf("Testing putting in (1,2) and getting 2 from get(1)... ");
  hashtable_t* ht = hashtable_init(NUM_BUCKETS);
  hashtable_put(ht, 1, 2);
  if(hashtable_get(ht, 1) != 2) {
    printf("failure. Put (1,2), did not get 2 out");
    exit(1);
  }
  hashtable_destroy(ht);
  printf("success!\n");
}

void testGetNonexistant() {
  printf("Testing failure on getting key=2 without putting first... ");
  hashtable_t* ht = hashtable_init(NUM_BUCKETS);
  if(hashtable_get(ht, 2) != -1) {
    printf("failure. Getting key=2 should have resulted in -1 return.");
    exit(1);
  }
  hashtable_destroy(ht);
  printf("success!\n");
}


void testRemovingItem(){
  printf("Testing removing item... ");
  hashtable_t* ht = hashtable_init(NUM_BUCKETS);
  hashtable_put(ht, 1, 2);
  if (!hashtable_remove(ht, 1)){
    printf("Failure, should not found it!");
    exit(1);
  }
  if(hashtable_get(ht,1) != -1 ){
    printf("failure, element still in hashtable");
    exit(1);
  }
  hashtable_destroy(ht);
  printf("success!\n");
  return;
}
