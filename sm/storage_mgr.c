/*
CS525: Advanced Database Organization
Fall 2021 | Group 22
Isa Muradli           (imuradli@hawk.iit.edu)       (Section 01)
Andrew Petravicius    (apetravicius@hawk.iit.edu)   (Section 01)
Christopher Sherman   (csherman1@hawk.iit.edu)      (Section 02)
*/

#include "../util/dberror.h"
#include "storage_mgr.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


/************************************************************
 *                   helper interface                       *
 ************************************************************/
int getTotalNumPages(struct stat*);

typedef struct SM_MgmtInfo {
  FILE* file;
} SM_MgmtInfo;

/************************************************************
 *                    implementation                        *
 ************************************************************/

/* manipulating page files */

// FIXME: what is this supposed to do?
void initStorageManager (void) {
  return;
}

RC createPageFile (char *fileName) {
  /*
    open a new file using fileName
    write '\0' * PAGE_SIZE
    result should be a file with the size of one PAGE_SIZE in bytes
    we can verify all null characters using "xxd" command
  */
  // FIXME: What if we go to create a file that already exists?

  // if file exists, delete it
  FILE* file;
  file = fopen(fileName, "r+b");
  if(file != NULL) {
    remove(fileName);
  }

  // create new file
  file = fopen(fileName, "w+b");

  // populate PAGE_SIZE string of null characters
  char resultString[PAGE_SIZE];
  for(int i = 0; i < PAGE_SIZE; i++) {
    resultString[i] = '\0';
  }

  // write PAGE_SIZE number of null characters to the page file
  fwrite(resultString, PAGE_SIZE, 1, file);
  fclose(file);

  return RC_OK;
}

RC openPageFile (char *fileName, SM_FileHandle *fHandle){
  /*
    FILE* fd = open(filename, "rw")
    if fd is null (or whatever it is that file doesnt exist)
      error handle with RC_NOT_FOUND
    else
      fill fHandle-><stuff> pointer with information about the file
      calculate number of pages from file size
    check man page for fstat to get file info

    we need a struct for storing mgmtInfo in the SM_FileHandle struct

    typedef struct SM_MgmtInfo {
      FILE* fd;
    } SM_MgmtInfo;

    fileHandle probably passed in similar to:
    ```
      SM_FileHandle fileHandle;
      openPageFile(fileName, &fileHandle);
    ```
  */

  FILE *file;

  // open fileName
  file = fopen(fileName, "r+b");

  // check if RC_NOT_FOUND
  if(file == NULL) {
    return RC_FILE_NOT_FOUND;
  }

  // get file descriptor
  int fd = fileno(file);

  // get file stats
  struct stat fileStats;
  fstat(fd, &fileStats);

  //populate fHandle
  fHandle->fileName = fileName;
  fHandle->curPagePos = 0;
  fHandle->totalNumPages = getTotalNumPages(&fileStats);

  // allocate struct for SM_MgmtInfo
  SM_MgmtInfo *mgmtInfo = malloc(sizeof(SM_MgmtInfo));
  mgmtInfo->file = file;
  fHandle->mgmtInfo = mgmtInfo;

  return RC_OK;
}

/**
  Using the file stats structure, return the number of pages using the number
  of bytes in the page file.
*/
int getTotalNumPages(struct stat* fileStats) {
  return fileStats->st_size / PAGE_SIZE;
}

RC closePageFile (SM_FileHandle *fHandle){
  /*
    close the file descriptor in ((SM_MgmtInfo*)fHandle->mgmtInfo)->fd
    remember to deallocate (free) mgmtInfo
  */
  //extract mgmtInfo
  SM_MgmtInfo* mgmtInfo = fHandle->mgmtInfo;

  // close the file using mgmtInfo
  fclose(mgmtInfo->file);

  // then free mgmtInfo LAST
  free(fHandle->mgmtInfo);

  return RC_OK;
}

RC destroyPageFile (char *fileName){
  /*
    https://linux.die.net/man/3/remove
  */
  if(remove(fileName)== 0){
    return RC_OK;
  }
  else{
    // most likely error, file not found
    return RC_FILE_NOT_FOUND;
  }
}

/* reading blocks from disc */

RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
  /*
    -------------------------
    |   0   |   1   |   2   |
    -------------------------
    |   3   |   4   |   5   |
    -------------------------
    |   6   |   7   |   8   |
    -------------------------

    we can find the desired byte offset using lseek with SEEK_SET directive
    with 0-indexed page positions/numbers, we can use this formula

    BYTES = PAGE_SIZE * page_pos

    i.e. the first block is byte 0 to byte PAGE_SIZE-1
         the second block is byte PAGE_SIZE to byte 2*PAGE_SIZE-1

    if we want to read 6th block, we lseek to 5*PAGE_SIZE, read a PAGE_SIZE,
      then the offset will be at the start of page position 6, but we will say
      the current page pos in fileHandle is 5 still. The pagePos should be of
      the block we just read, per lab handout

    fileHandle->currBlockPos = pageNum;
  */

    SM_MgmtInfo* mgmtInfo;
    FILE* file;

    // ensure that pageNum is less than totalNumPages and not less than 0
    if(pageNum < 0 || pageNum >= fHandle->totalNumPages) {
      return RC_READ_NON_EXISTING_PAGE;
    }

    // extract file object from management info
    mgmtInfo = fHandle->mgmtInfo;
    file = mgmtInfo->file;

    // change offset to specified page number
    fseek(file, pageNum*PAGE_SIZE, SEEK_SET);

    // read one PAGE_SIZE into memPage
    fread(memPage, PAGE_SIZE, 1, file);

    // change curPagePos into the page we just read
    fHandle->curPagePos = pageNum;

    return RC_OK;
}

int getBlockPos (SM_FileHandle *fHandle) {
  /*
    per lab handout, blocks are pages (page 1 of lab handout)
    just extract this from fHandle ez-pz
  */
  return fHandle->curPagePos;
}

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
  /*
    special case of readBlock
  */
  readBlock(0, fHandle, memPage);
  return RC_OK;
}

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
  /*
    special case of readBlock
  */
  readBlock(fHandle->curPagePos - 1, fHandle, memPage);
  return RC_OK;
}

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
  /*
    special case of readBlock
  */
  readBlock(fHandle->curPagePos, fHandle, memPage);
  return RC_OK;
}

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
  /*
    special case of readBlock
  */
  readBlock(fHandle->curPagePos + 1, fHandle, memPage);
  return RC_OK;
}

RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
  /*
    special case of readBlock
  */
  readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
  return RC_OK;
}

/* writing blocks to a page file */

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
  /*
    very similar to readBlock, just backwards (memPage to page file)
  */

  // get FILE object
  SM_MgmtInfo* mgmtInfo = fHandle->mgmtInfo;
  FILE* file = mgmtInfo->file;

  fseek(file, pageNum*PAGE_SIZE, SEEK_SET);   // set offset with pageNum

  fHandle->curPagePos = pageNum;            // set pageNum to page just wrote

  // write mempage to page file or handle failure
  if(fwrite(memPage, PAGE_SIZE, 1, file) == 0) {
    return RC_WRITE_FAILED;
  }

  return RC_OK;
}

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
  /*
    special case of writeBlock
  */
  writeBlock(fHandle->curPagePos, fHandle, memPage);
  return RC_OK;
}

RC appendEmptyBlock (SM_FileHandle *fHandle) {
  /*
    special case of ensureCapacity
  */
  ensureCapacity(fHandle->totalNumPages + 1, fHandle);
  return RC_OK;
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
  /*
    if numberOfPages is greater than the number of pages in the page file,
    append (numberOfPages - totalNumPages) * PAGE_SIZE of '\0' (null chars)
    to the end of the page file
  */
  SM_MgmtInfo* mgmtInfo;
  FILE* file;

  if(numberOfPages > fHandle->totalNumPages) {
    // extract file object
    mgmtInfo = fHandle->mgmtInfo;
    file = mgmtInfo->file;

    // creat all null char page handle
    SM_PageHandle memPage = malloc(PAGE_SIZE);
    for(int pageByte = 0; pageByte < PAGE_SIZE; ++pageByte) {
      memPage[pageByte] = '\0';
    }

    // seek to the very end of the file
    fseek(file, fHandle->totalNumPages*PAGE_SIZE, SEEK_SET);

    // repeatedly write page handle until we have enough pages
    for(int i = 0; i < numberOfPages - fHandle->totalNumPages; ++i) {
      fwrite(memPage, PAGE_SIZE, 1, file);
    }

    // free heap allocated for mempage
    free(memPage);

    // update file handle with new total number of pages
    fHandle->totalNumPages = numberOfPages;
  }
  return RC_OK;
}
