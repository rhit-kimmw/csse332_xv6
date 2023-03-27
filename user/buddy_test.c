/****************************************************************************
 * Copyright © 2022 Rose-Hulman Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 ****************************************************************************/
#include "kernel/types.h"
#include "user/user.h"
#include "user/rhtest.h"
#include "user/rhmalloc.h"

#define HEAP_MEM_START heap_start()

//permutation of length step [0,STEP]
#define STEP 8
static int step_perm[STEP] = {3, 4, 1, 2, 0, 5, 6, 7};

/* Simple random number generator ported from grind.c */
unsigned long rand_next = 1;
int rand(void);

/**
 * This is a bit tricky. We only want to free all in case the freelist head is
 * occupied, otherwise, very bad things can happen.
 */
static void reset_if_needed(void)
{
  if(is_initialized()) rhfree_all();
}

void test_get_buddy(void)
{
  rhassert_ptr_equals((void*)1, get_buddy((void*)0, 0));
  rhassert_ptr_equals((void*)0, get_buddy((void*)1, 0));

  rhassert_ptr_equals((void*)0xa01, get_buddy((void*)0xa00, 0));
  rhassert_ptr_equals((void*)0xa00, get_buddy((void*)0xa01, 0));
  rhassert_ptr_equals((void*)0xa04, get_buddy((void*)0xa00, 2));
  rhassert_ptr_equals((void*)0xa00, get_buddy((void*)0xa04, 2));
  rhassert_ptr_equals((void*)0xa08, get_buddy((void*)0xa00, 3));
  rhassert_ptr_equals((void*)0xa00, get_buddy((void*)0xa08, 3));
}

/**
 * Test basic allocation strategies.
 *
 * @warning This routine does not free the memory it allocates, so make sure to
 * reset after calling it.
 */
static void test_basic_alloc(void)
{
  void *pointer1, *pointer2, *pointer3, *pointer4, *pointer5;

  // make sure to reset if there is anything needed
  reset_if_needed();

  pointer1 = rhmalloc(4);
  pointer2 = rhmalloc(4);
  rhassert_int_equals(pointer2 - pointer1, 32);

  // this chunk should already be available without needing to split
  // blocks
  pointer3 = rhmalloc(65);
  rhassert_int_equals(pointer3 - pointer1, 128);

  // ok lets split the remaining 64 bit block into a 32 bit
  pointer4 = rhmalloc(4);
  rhassert_int_equals(pointer4 - pointer1, 64);

  // and now the buddy should be available
  pointer5 = rhmalloc(4);
  rhassert_int_equals(pointer5 - pointer1, 64 + 32);
}

/**
 * Test basic free strategies.
 *
 * @warning This routine does not free the memory it allocates, so make sure to
 * reset after calling it.
 */
void test_basic_free(void) {
  void *p1 = (void*)0x01;
  void *p2 = (void*)0x02;
  void *p3 = (void*)0x03;
  void *p4 = (void*)0x04;
  void *p5 = (void*)0x05;

  reset_if_needed();

  p1 = rhmalloc(4);
  rhassert(p1 != 0);
  rhfree(p1);
  p2 = rhmalloc(4);
  rhassert_ptr_equals(p1, p2);

  // this should be allocated after the p1 p2 block
  p3 = rhmalloc(33);
  rhassert_int_equals(p3 - p1, 64);

  rhfree(p2);
  // we should coalesce p1 and p2 and reuse them
  p4 = rhmalloc(33);
  rhassert_ptr_equals(p4, p1);

  rhfree(p3);
  rhfree(p4);

  // make sure we coalesce everything
  p5 = rhmalloc(MAX_HEAP_SIZE-2*sizeof(struct metadata_t));
  rhassert_ptr_equals(p5, p1);
}

void test_wrong_size_coalesce(void)
{
  void *p1, *p4;

  reset_if_needed();

  p1 = rhmalloc(33);
  rhmalloc(4);
  rhmalloc(4);

  // when we free p1, it seems like it might be safe to coalesce p1 with p2,
  // they are buddies and they are both free. However, they are not the same
  // size, so allowing them to coalesce will inadvertantly free p3
  rhfree(p1);

  p4 = rhmalloc(65);

  rhassert_int_equals(p4 - p1, 128);
}
void print_free_blocks();
void test_gracefully_run_out_of_memory(void)
{
  reset_if_needed();

  // make sure that things happen at least once.
  rhassert(rhmalloc(800) != 0);

  // allocate a lot until we fill up all memory and we have no more room for
  // anything.
  while(rhmalloc(800));

  // check that no further allocation can happen.
  rhassert(!rhmalloc(800));
}

void test_stress_big_heap_chunks(void)
{
  void *ptr[STEP];
  int size, i;

  reset_if_needed();

  // each of our chunks here are 1/8th of the total heap memory hence "big" but
  // a little less, because otherwise the metadata and the fact that we're
  // required to allocate powers of two is gonna mean we can't allocate as much
  // as you'd expect
  size = (MAX_HEAP_SIZE / STEP) - 64;

  // allocate size bytes (STEP-1) times because we allocated a little less,
  // these should all succeed
  for(i = 0; i < STEP; i++) {
    ptr[i] = rhmalloc(size);
    rhassert(ptr[i] != 0);
  }

  // free allocated memory by permutation
  for(i = 0; i < STEP; i++) {
    rhfree(ptr[step_perm[i]]);
  }

  // finally try to allocate big chunk (90%) of memory to check functionality of
  // free
  size = MAX_HEAP_SIZE * 0.9;
  ptr[0] = rhmalloc(size);
  // fails when we can't allocate a big chunk, probably a coalescing failure.
  rhassert(ptr[0] != 0);
}

// cannot grow larger than these number or we risk running out of stack room
#define BUFLEN 10
#define LOOPCNT 50
#define ALLOC_SIZE MAX_HEAP_SIZE/100
#define ALLOC_CONST 1 << 16 

void test_stress_overlapping(void)
{
  int size;
  int itr;
  void *ptr[BUFLEN];
  int i, j;
  int randvar;
  long int global[LOOPCNT][2];
  void *bluff;

  reset_if_needed();

  // make sure we can put in some data, i..e, malloc does something
  bluff = rhmalloc(4);
  rhassert(bluff != 0);
  rhfree(bluff);

  for(j = 0; j < LOOPCNT; j++) {
    global[j][0] = -1;
    global[j][1] = -1;
  }

  for(i = 0; i < BUFLEN; i++) {
    ptr[i] = 0;
  }

  for(i = 0; i < LOOPCNT; i++) {
    itr = rand() % BUFLEN;

    randvar = rand(); // flip a coin to decide alloc/free

    if(randvar < ALLOC_CONST && ptr[itr] == 0) {
      // if the index is not already allocated allocate random size memory
      size = (rand() % ALLOC_SIZE) + 1;
      ptr[itr] = rhmalloc(size);
      // its possible the allocation could fail because we've allocated the max
      // already
      if(!ptr[itr]) // normal
        continue;

      // If we did allocate, check every other existing allocation to ensure the
      // region we were given does not overlap with other regions
      for(j = 0; j < i; j++) {
        if(global[j][0] == -1) {
          continue;
        }

        if((((long int)ptr[itr] >= global[j][0]) &&
            ((long int)ptr[itr] <= global[j][1])) ||
           (((long int)ptr[itr] + size >= global[j][0]) &&
            ((long int)ptr[itr] + size <= global[j][1]))) {
          printf("new region %p-%p\n", (void *)ptr[itr],
                 (void *)ptr[itr] + size);
          printf("old region %p-%p\n", (void *)global[j][0],
                 (void *)global[j][1]);
          printf("****allocation regions overlap\n****");
          rhassert(0);
        }
      }
      global[i][0] = (long int)ptr[itr];
      global[i][1] = (long int)ptr[itr] + size;

    } else if(randvar >= ALLOC_CONST && ptr[itr] != 0) { // free memory
      for(j = 0; j < i; j++) {
        if(global[j][0] == (long int)ptr[itr]) {
          global[j][0] = -1;
          global[j][1] = -1;
        }
      }
      rhfree(ptr[itr]);
      ptr[itr] = 0;
    }
  }

  // if we get here, all should be good, so assert true to get it to print out.
  rhassert(1);
}

int main(int argc, char **argv)
{
  int opt;

  if(argc < 2) {
    /* Run all tests */
    test_get_buddy();                         // 0
    test_basic_alloc();                       // 1
    test_basic_free();                        // 2
    test_wrong_size_coalesce();               // 3
    test_gracefully_run_out_of_memory();      // 4
    test_stress_big_heap_chunks();            // 5
    test_stress_overlapping();                // 6
    exit(0);
  }

  opt = atoi(argv[1]);
  switch(opt) {
    case 0:
    test_get_buddy();
    break;
    case 1:
    test_basic_alloc();
    break;
    case 2:
    test_basic_free();
    break;
    case 3:
    test_wrong_size_coalesce();
    break;
    case 4:
    test_gracefully_run_out_of_memory();
    break;
    case 5:
    test_stress_big_heap_chunks();
    break;
    case 6:
    test_stress_overlapping();
    break;
    default:
    printf("Unknown option %s\n", argv[1]);
    exit(1);
    break;
  }

  exit(0);
}

static int do_rand(unsigned long *ctx)
{
  /*
   * Compute x = (7^5 * x) mod (2^31 - 1)
   * without overflowing 31 bits:
   *      (2^31 - 1) = 127773 * (7^5) + 2836
   * From "Random number generators: good ones are hard to find",
   * Park and Miller, Communications of the ACM, vol. 31, no. 10,
   * October 1988, p. 1195.
   */
  long hi, lo, x;

  /* Transform to [1, 0x7ffffffe] range. */
  x = (*ctx % 0x7ffffffe) + 1;
  hi = x / 127773;
  lo = x % 127773;
  x = 16807 * lo - 2836 * hi;
  if(x < 0) x += 0x7fffffff;
  /* Transform to [0, 0x7ffffffd] range. */
  x--;
  *ctx = x;
  return (x);
}

int rand(void)
{
  return do_rand(&rand_next);
}
