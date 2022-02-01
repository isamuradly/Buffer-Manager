/*
CS525: Advanced Database Organization
Fall 2021 | Group 22
Isa Muradli           (imuradli@hawk.iit.edu)       (Section 01)
Andrew Petravicius    (apetravicius@hawk.iit.edu)   (Section 01)
Christopher Sherman   (csherman1@hawk.iit.edu)      (Section 02)
*/

#include "../util/dberror.h"
#include "../sm/storage_mgr.h"
#include "../bm/buffer_mgr.h"
#include "../bm/buffer_mgr_extra.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		const int numPages, ReplacementStrategy strategy,
		void *stratData);

int main() {
  char* fileName = "file.bin";
  BM_BufferPool bm;
  createPageFile(fileName);
  initBufferPool(&bm, fileName, 10, RS_FIFO, NULL);
  shutdownBufferPool(&bm);
  destroyPageFile(fileName);
  return 0;
}
