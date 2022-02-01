/*
CS525: Advanced Database Organization
Fall 2021 | Group 22
Isa Muradli           (imuradli@hawk.iit.edu)       (Section 01)
Andrew Petravicius    (apetravicius@hawk.iit.edu)   (Section 01)
Christopher Sherman   (csherman1@hawk.iit.edu)      (Section 02)
*/

#ifndef BUFFER_MGR_EXTRA_H
#define BUFFER_MGR_EXTRA_H

#include "../ht/hashtable.h"

// might be more complex than BM_PageHandle once we account for replacement strats
typedef struct BM_PageFrame {
  char data[PAGE_SIZE];
  // we might need more for strategy pointers later
} BM_PageFrame;

typedef struct BM_MgmtData {
  SM_FileHandle* fHandle;       // file handle for buffer pool page file
  BM_PageFrame* pFrames;        // list of page handles (page slots in buffer)
  int nextNewFrameIdx;          // the next free frame to be allocated in frames
  hashtable_t* frameMap;         // hashtable to map page number to frame index
  bool* dirtyFlags;             // dirty flags array
  int* fixCounts;               // fix counts array
  int* frameContents;           // pageNumbers at every index
  void* replacementData;        // struct for replacement strategy data
  int countReadIO;              // number of reads from disk
  int countWriteIO;             // number of writes to disk
} BM_MgmtData;

typedef struct FIFO_Data {
  int fifoNextIdx;
} FIFO_Data;

typedef struct LRU_Data {
  int counter;
  int* lastUsed;
} LRU_Data;

#endif
