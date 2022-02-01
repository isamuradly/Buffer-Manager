# CS525: Advanced Database Organization
# Fall 2021 | Group 22
# Isa Muradli           (imuradli@hawk.iit.edu)       (Section 01)
# Andrew Petravicius    (apetravicius@hawk.iit.edu)   (Section 01)
# Christopher Sherman   (csherman1@hawk.iit.edu)      (Section 02)

.PHONY: default
default: test_assign2_1

test_assign2_1: test/test_assign2_1.c util/dberror.c bm/buffer_mgr.c \
 bm/buffer_mgr_stat.c bm/buffer_mgr.c ht/hashtable.c
	gcc -Wall -o test_assign2_1 util/dberror.c bm/buffer_mgr.c sm/storage_mgr.c \
	 test/test_assign2_1.c bm/buffer_mgr_stat.c ht/hashtable.c

.PHONY: debug
debug: test_assign2_1d

test_assign2_1d: test/test_assign2_1.c util/dberror.c bm/buffer_mgr.c bm/buffer_mgr_stat.c \
 sm/storage_mgr.c ht/hashtable.c
	gcc -Wall -g -o test_assign2_1 util/dberror.c bm/buffer_mgr.c sm/storage_mgr.c \
	 test/test_assign2_1.c bm/buffer_mgr_stat.c ht/hashtable.c

.PHONY: our_tests
our_tests: our_tests.out

our_tests.out: test/our_tests.c util/dberror.c bm/buffer_mgr.c bm/buffer_mgr_stat.c \
 sm/storage_mgr.c ht/hashtable.c
	gcc -Wall -o our_tests.out util/dberror.c bm/buffer_mgr.c sm/storage_mgr.c \
	 test/our_tests.c bm/buffer_mgr_stat.c ht/hashtable.c

.PHONY: hashtable_test
hashtable_test: hashtable_test.out

hashtable_test.out: ht/hashtable_test.c ht/hashtable.c
	gcc -Wall -o hashtable_test.out ht/hashtable_test.c ht/hashtable.c

.PHONY: clean
clean:
	rm -f test_assign2_1
	rm -f test_assign2_2
	rm -f *.bin
	rm -f *.out
