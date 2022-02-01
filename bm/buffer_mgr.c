/*
CS525: Advanced Database Organization
Fall 2021 | Group 22
Isa Muradli           (imuradli@hawk.iit.edu)       (Section 01)
Andrew Petravicius    (apetravicius@hawk.iit.edu)   (Section 01)
Christopher Sherman   (csherman1@hawk.iit.edu)      (Section 02)
*/

#include "../util/dberror.h"
#include "../sm/storage_mgr.h"
#include "buffer_mgr.h"
#include "buffer_mgr_extra.h"
#include "../ht/hashtable.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


/************************************************************
 *                   helper interface                       *
 ************************************************************/

#define HASHTABLE_FACTOR 5

int evictPage(BM_BufferPool* bm);
int evictFIFO(BM_BufferPool* bm);
int evictLRU(BM_BufferPool* bm);
RC writeBack(BM_BufferPool* bm, int pageNum, SM_FileHandle* fHandle,
  SM_PageHandle memPage);
hashtable_t* createFrameMap(int numPages);
int getFrameIdx(BM_BufferPool* bm, int pageNum);
void destroyBufferPool(BM_BufferPool* bm);
void incFixCount (BM_BufferPool *const bm, BM_PageHandle *const page);
void incFixCountFromIdx(BM_BufferPool* bm, int frameIdx);
void decFixCount (BM_BufferPool *const bm, BM_PageHandle *const page);
void decFixCountFromIdx(BM_BufferPool* bm, int frameIdx);
void resetDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
void resetDirtyFromIdx(BM_BufferPool* bm, int idx);
void markDirtyFromIdx(BM_BufferPool* bm, int idx);
void incReadIO(BM_BufferPool* bm);
void incWriteIO(BM_BufferPool* bm);

/************************************************************
 *                    implementation                        *
 ************************************************************/


// Buffer Manager Interface Pool Handling

/**
initBufferPool

creates a new buffer pool with numPages page frames using the page replacement
strategy strategy. The pool is used to cache pages from the page file with name
pageFileName. Initially, all page frames should be empty. The page file should
already exist, i.e., this method should not generate a new page file. stratData
can be used to pass parammeters for the page replacement strategy. For example,
for LRU-k this could be the parameter k.
*/
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		const int numPages, ReplacementStrategy strategy,
		void *stratData)
{
  // get the file name
  char* copyPageFileName = malloc(strlen(pageFileName) + 1);
  strcpy(copyPageFileName, pageFileName);

  // open the pagefile
  SM_FileHandle* fHandle = malloc(sizeof(SM_FileHandle));
  openPageFile(copyPageFileName, fHandle);

  // copy immediate information into bm
  bm->pageFile = copyPageFileName;
  bm->numPages = numPages;
  bm->strategy = strategy;

  // create mgmtData with fHandle and array of pFrames
  BM_MgmtData* mgmtData = malloc(sizeof(BM_MgmtData));
  mgmtData->fHandle = fHandle;
  mgmtData->frameMap = createFrameMap(numPages);
  mgmtData->pFrames = (BM_PageFrame*) malloc(numPages * sizeof(BM_PageFrame));
  mgmtData->dirtyFlags = (bool*) malloc(numPages * sizeof(bool));
  mgmtData->fixCounts = (int*) malloc(numPages * sizeof(int));
  mgmtData->frameContents = (int*) malloc(numPages * sizeof(int));

  // set defaults for every idx for pFrames and supporting fixCounts and
  //  dirtyFlags
  for(int i = 0; i < numPages; ++i) {
    // all frameContents are noPage
    mgmtData->frameContents[i] = NO_PAGE;

    // all dirty flags false
    mgmtData->dirtyFlags[i] = false;

    // all fixCounts 0
    mgmtData->fixCounts[i] = 0;
  }

  // next frame to write to will be 0
  mgmtData->nextNewFrameIdx = 0;

  // no reads or writes yet
  mgmtData->countReadIO = 0;
  mgmtData->countWriteIO = 0;

  // prepare replacement strategy data
  // FIXME remove unused replacement strategies before submission
  FIFO_Data* fifoData;
  LRU_Data* lruData;
  switch(strategy) {
    case RS_FIFO:
      fifoData = (FIFO_Data*) malloc(sizeof(FIFO_Data));
      fifoData->fifoNextIdx = 0;
      mgmtData->replacementData = fifoData;
      break;
    case RS_LRU:
      lruData = (LRU_Data*) malloc(sizeof(LRU_Data));
      lruData->counter = 0;
      lruData->lastUsed = (int*) malloc(numPages * sizeof(int));
      mgmtData->replacementData = lruData;
      break;
    case RS_CLOCK:
      break;
    case RS_LRU_K:
      break;
    case RS_LFU:
      break;
  }

  // put mgmtData into buffer pool object
  bm->mgmtData = mgmtData;

  return RC_OK;

}

/**
shutdownBufferPool

destroys a buffer pool. This method should free up all resources associated
with buffer pool. For example, it should free the memory allocated for page
frames. If the buffer pool contains any dirty pages, then these pages should be
written back to disk before destroying the pool. It is an error to shutdown a
buffer pool that has pinned pages
*/
RC shutdownBufferPool(BM_BufferPool *const bm)
{
  // FIXME: email TA asking about shutting down buffer pool with pinned pages

  // force all dirty pages to be written back to pagefile
  forceFlushPool(bm);

  // free everything allocated in initBufferPool @Andrew @Isa
  destroyBufferPool(bm);

  return RC_OK;
}

/**
forceFlushPool

causes all dirty pages (with fix count 0) from the buffer pool to be written to
disk.
*/
RC forceFlushPool(BM_BufferPool *const bm)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  BM_PageHandle tempPHandle;

  // call forcePage on all dirty, no fixCount pages
  for(int i = 0; i < bm->numPages; i++) {
    if(mgmtData->dirtyFlags[i] && mgmtData->fixCounts[i] == 0){
      tempPHandle.pageNum = mgmtData->frameContents[i];
      tempPHandle.data = mgmtData->pFrames[i].data;
      forcePage(bm, &tempPHandle);
    }
  }

  return RC_OK;
}

// Buffer Manager Interface Access Pages


/**
markDirty

marks a page as dirty
*/
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
  int frameIdx;
  frameIdx = getFrameIdx(bm, page->pageNum);
  markDirtyFromIdx(bm, frameIdx);
  return RC_OK;
}

/**
unpinPage

unpins the page page. The pageNum field of page should be used to figure out
which page to unpin.
*/
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
  int frameIdx;
  BM_MgmtData* mgmtData = bm->mgmtData;
  if((frameIdx = getFrameIdx(bm, page->pageNum)) >= 0) {
    mgmtData->fixCounts[frameIdx] -= 1;
  }
  return RC_OK;
}

/**
forcePage

write the current content of the page back to the page file on disk. If the
frame's fix count has a fix count of 0, reset the dirty flag.
*/
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{

  char* data;
  int pageNum;
  BM_MgmtData* mgmtData;
  SM_FileHandle* fHandle;

  mgmtData = bm->mgmtData;

  // prepare arguments
  fHandle = mgmtData->fHandle;
  pageNum = page->pageNum;
  data = page->data;

  // write block to memory
  writeBack(bm, pageNum, fHandle, data);

  return RC_OK;
}

/**
pinPage

pins the page with page number pageNum. The buffer manager is responsible to set
the pageNum field of the page handle passed to the method. Similarly, the data
field should point to the page frame the page is stored in (the area in memory
storing the content of the page)
*/
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
		const PageNumber pageNum)
{
  /*
    BM_PageHandle is what the user (or record manager) will use to modify data
    on the user end (outside this function)

    the pointer to data in a page handle should point to same memory data
    as the page handle in our buffer pool

    pinning a page needs us to sync these addresses
  */
  int frameIdx;
  BM_MgmtData *mgmtData = bm->mgmtData;
  RC readResult;
  LRU_Data* lruData;

  // first, check if it is in the hashtable
  // if in hashtable, find frame and return the pointer
  if((frameIdx = getFrameIdx(bm, pageNum)) >= 0) {
    BM_PageFrame* pFrame = mgmtData->pFrames + frameIdx;
    page->pageNum = pageNum;
    page->data = pFrame->data;
    incFixCountFromIdx(bm, frameIdx);
  }
  else {
    // check if the nextNewFrameIdx is < numPages
    if(mgmtData->nextNewFrameIdx < bm->numPages) {
      // get frame idx then increment the next new frame index
      frameIdx = mgmtData->nextNewFrameIdx++;
    }
    else {
      // all slots taken, we must evict a page
      frameIdx = evictPage(bm);
    }

    // ensure the page exists
    ensureCapacity(pageNum + 1, mgmtData->fHandle);

    // read `pageNum` of the page file into the frame at frame index
    BM_PageFrame* pFrame = mgmtData->pFrames + frameIdx;
    readResult = readBlock(pageNum, mgmtData->fHandle, pFrame->data);
    incReadIO(bm);  // incrememnt number of read operations

    // if error, fail and return the error
    if(readResult != RC_OK) return readResult;

    // synchronize user page handle with page frame
    page->data = pFrame->data;

    // set page numbers
    mgmtData->frameContents[frameIdx] = pageNum;
    page->pageNum = pageNum;

    // set dirty flag
    mgmtData->dirtyFlags[frameIdx] = false;

    // set fix count to 1 (since it's a new page)
    mgmtData->fixCounts[frameIdx] = 1;

    // add to lookup table
    hashtable_put(mgmtData->frameMap, pageNum, frameIdx);
  }

  // if LRU replacement policy, increase last used value
  if(bm->strategy == RS_LRU) {
    lruData = mgmtData->replacementData;
    lruData->lastUsed[frameIdx] = lruData->counter++;
  }
  return RC_OK;
}


// Statistics Interface

/**
getFrameContents

returns an array of PageNumbers (of size numPages) where the ith
element is the number of the page stored in the ith page frame. An empty page
frame is represented using the constant NO_PAGE.
*/
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  return mgmtData->frameContents;
}

/**
getDirtyFlags

returns an array of bools (of size numPages) where the ith elementis TRUE if the
page stored in the ith page frame is dirty. Empty page frames are considered
as clean
*/
bool *getDirtyFlags (BM_BufferPool *const bm)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  return mgmtData->dirtyFlags;
}

/**
getFixCounts

returns an array of ints (of size numPages) where the ith element is
the fix count of the page stored in the ith page frame. Return 0 for empty page
frames
*/
int *getFixCounts (BM_BufferPool *const bm)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  return mgmtData->fixCounts;
}

/**
getNumReadIO

returns the number of pages that have been read from disk since a
buffer pool has been initialized. You code is responsible to initializing this
statistic at pool creating time and update whenever a page is read from the page
file into a page frame.
*/
int getNumReadIO (BM_BufferPool *const bm)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  return mgmtData->countReadIO;
}

/**
getNumWriteIO

returns the number of pages written to the page file since the buffer pool has
been initialized
*/
int getNumWriteIO (BM_BufferPool *const bm)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  return mgmtData->countWriteIO;
}

/************************************************************
 *                helper implementation                     *
 ************************************************************/

/**
evictPage

finds the next page to evict and returns the frame index of the evicted page
*/
int evictPage(BM_BufferPool* bm) {
  // FIXME only declare in future
  int newIdx = 0;
  // FIXME remove unused replacement strategies before submission
  switch(bm->strategy) {
    case RS_FIFO:
      newIdx = evictFIFO(bm);
      break;
    case RS_LRU:
      newIdx = evictLRU(bm);
      break;
    case RS_CLOCK:
      break;
    case RS_LRU_K:
      break;
    case RS_LFU:
      break;
  }
  return newIdx;
}

int evictFIFO(BM_BufferPool* bm) {
  BM_MgmtData* mgmtData;
  FIFO_Data* fifoData;
  int evictingIdx;

  // extract fifo replacement information
  mgmtData = bm->mgmtData;
  fifoData = mgmtData->replacementData;

  int counter = 0;
  do{
    evictingIdx = fifoData->fifoNextIdx;

    // FIXME never evict a page with fixCount > 0
    // prepare the next fifo idx (increment the old value, cycle at numPages)
    fifoData->fifoNextIdx = (evictingIdx + 1) % bm->numPages;
  } while((mgmtData->fixCounts[evictingIdx] > 0) && (counter++ < bm->numPages));

  if(counter >= bm->numPages) {
    // freak out
  }

  // write evicted data back to disk if dirty
  if(mgmtData->dirtyFlags[evictingIdx]) {
    writeBack(bm, mgmtData->frameContents[evictingIdx], mgmtData->fHandle,
      mgmtData->pFrames[evictingIdx].data);
  }

  // remove from hashtable
  hashtable_remove(mgmtData->frameMap, mgmtData->frameContents[evictingIdx]);

  // reset the data frame
  mgmtData->frameContents[evictingIdx] = NO_PAGE;

  return evictingIdx;
}

int evictLRU(BM_BufferPool* bm) {
  BM_MgmtData* mgmtData;
  LRU_Data* lruData;
  int minFrame, evictingIdx;

  mgmtData = bm->mgmtData;
  lruData = mgmtData->replacementData;

  // search lastUsed array for minimum (minimum with fixCount 0)
  minFrame = 0;
  for(int i = 1; i < bm->numPages; ++i) {
    if((lruData->lastUsed[i] < lruData->lastUsed[minFrame])
      && (mgmtData->fixCounts[i] == 0))
    {
      minFrame = i;
    }
  }

  if(minFrame == bm->numPages + 1) {
    // freak out
  }

  evictingIdx = minFrame;

  // write evicted data back to disk if dirty
  if(mgmtData->dirtyFlags[evictingIdx]) {
    writeBack(bm, mgmtData->frameContents[evictingIdx], mgmtData->fHandle,
      mgmtData->pFrames[evictingIdx].data);
  }

  // remove from hashtable
  hashtable_remove(mgmtData->frameMap, mgmtData->frameContents[evictingIdx]);

  // reset the data frame
  mgmtData->frameContents[evictingIdx] = NO_PAGE;

  return evictingIdx;
}

RC writeBack(BM_BufferPool* bm, int pageNum, SM_FileHandle* fHandle, \
  SM_PageHandle memPage)
{
  RC result;
  int frameIdx;
  BM_MgmtData* mgmtData = bm->mgmtData;

  // write block to memory
  if ((result = writeBlock(pageNum, fHandle, memPage)) != RC_OK) {
    return result;
  }

  // increment number of write IO operations
  incWriteIO(bm);

  // if fixCount=0, then reset dirty flag
  frameIdx = getFrameIdx(bm, pageNum);
  if(mgmtData->fixCounts[frameIdx] == 0) {
    resetDirtyFromIdx(bm, frameIdx);
  }

  // return the result of the
  return RC_OK;
}

/**
createFrameMap

creates a hashtable that maps pageNum to frame index. By setting numBuckets
to numPages / HASHTABLE_FACOR + 1, the function creates a hashtable that
has to search at most a linked list in a bucket greater than the
HASHTABLE_FACTOR.
*/
hashtable_t* createFrameMap(int numPages) {
  return hashtable_init((numPages / HASHTABLE_FACTOR) + 1);
}

/**
getFrameIdx

converts pageNum to frameIdx using hashtable
*/
int getFrameIdx(BM_BufferPool* bm, int pageNum)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  return hashtable_get(mgmtData->frameMap, pageNum);
}

/**
destroyBufferPool
 closes streams and frees memory
*/
void destroyBufferPool(BM_BufferPool* bm)
{
  BM_MgmtData* mgmtData = (BM_MgmtData*) bm->mgmtData;
  LRU_Data* lruData;
   // free malloc'd strings and arrays
  free(bm->pageFile);
  free(mgmtData->pFrames);
  free(mgmtData->dirtyFlags);
  free(mgmtData->fixCounts);
  free(mgmtData->frameContents);
  if(bm->strategy == RS_LRU) {
    lruData = mgmtData->replacementData;
    free(lruData->lastUsed);
  }
  free(mgmtData->replacementData);

  // close and free fHandle
  closePageFile(mgmtData->fHandle);
  free(mgmtData->fHandle);

  // destroy the hashtable
  hashtable_destroy(mgmtData->frameMap);

  // free mgmtData struct
  free(mgmtData);

  return;
}

/**
incFixCount

increment fix count of the frame corresponding to the `pageNum` in `page`
*/
void incFixCount (BM_BufferPool *const bm, BM_PageHandle *const page)
{
  int frameIdx;
  frameIdx = getFrameIdx(bm, page->pageNum);
  incFixCountFromIdx(bm, frameIdx);
  return;
}

/**
incFixCountFromIdx

incremement the fix count at frame idx
*/
void incFixCountFromIdx(BM_BufferPool* bm, int frameIdx) {
  BM_MgmtData* mgmtData = bm->mgmtData;
  mgmtData->fixCounts[frameIdx] += 1;
}

/**
decFixCount

decrement fix count of the frame corresponding to the `pageNum` in `page`
*/
void decFixCount (BM_BufferPool *const bm, BM_PageHandle *const page)
{
  int frameIdx;
  frameIdx = getFrameIdx(bm, page->pageNum);
  decFixCountFromIdx(bm, frameIdx);
  return;
}

/**
decFixCountFromIdx

decremement the fix count at frame idx
*/
void decFixCountFromIdx(BM_BufferPool* bm, int frameIdx) {
  BM_MgmtData* mgmtData = bm->mgmtData;
  mgmtData->fixCounts[frameIdx] -= 1;
}

/**
resetDirty

resets a page to not dirty
*/
void resetDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
  int frameIdx;
  frameIdx = getFrameIdx(bm, page->pageNum);
  resetDirtyFromIdx(bm, frameIdx);
  return;
}

/**
resetDirtyFromIdx

makes the entry for dirtyFlag at provided idx false
*/
void resetDirtyFromIdx(BM_BufferPool* bm, int idx)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  mgmtData->dirtyFlags[idx] = false;
}

/**
makeDirtyFromIdx

makes the entry for dirtyFlag at provided idx true
*/
void markDirtyFromIdx(BM_BufferPool* bm, int idx)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  mgmtData->dirtyFlags[idx] = true;
}

/**
incReadIO

increments the counter of reads to the disk
*/
void incReadIO(BM_BufferPool* bm)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  mgmtData->countReadIO += 1;
  return;
}

/**
incWriteIO

increments the counter of reads to the disk
*/
void incWriteIO(BM_BufferPool* bm)
{
  BM_MgmtData* mgmtData = bm->mgmtData;
  mgmtData->countWriteIO += 1;
  return;
}
