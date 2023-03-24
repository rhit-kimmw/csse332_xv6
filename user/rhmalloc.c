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
#include "user/rhmalloc.h"

/**
 * For testing purposes, we need to record where our memory starts. Generally
 * this is hidden from the users of the library but we're just using it here to
 * make our tests more meaningful.
 */
static void *heap_mem_start = 0;

/**
 * Head of the free list. It is actually a pointer to the header of the first
 * free block.
 *
 * @warning In this assignment, "freelist" is somewhat of a misnomer, because
 * this list contains both free and unfree nodes.
 */
static metadata_t *freelist = 0;

/**
 * Return a pointer to the metadata of the head of the free list.
 *
 * @return The freelist pointer.
 */
metadata_t *freelist_head(void) { return freelist; }

/**
 * Return the pointer to the start of the heap memory.
 * 
 * @return The heam_mem_start ptr.
 */
void *heap_start(void) { return heap_mem_start; }


void *initialize_freelist(void *heap_start, unsigned heap_size)
{
  //create a metadata_t structure 
  metadata_t *block = (metadata_t *)heap_start;
  block->s.size = ALIGN(heap_size - sizeof(metadata_t));
  block->s.in_use = 0;
  block->s.next = 0;
  block->s.prev = 0;

  freelist = block;
  return 0;
}

/**
 * Initialize the rh memroy allocator system.
 *
 * @return 0 on success, non-zero number on failure.
 */
uint8 rhmalloc_init(void)
{
  char *p;

  /* Grab the start of the memory where we are allocating. */
  heap_mem_start = sbrk(0);

  /* grow the memory area by MAX_HEAP_SIZE bytes */
  p = sbrk(MAX_HEAP_SIZE);
  if(p == (char *)-1) {
    fprintf(2, "sbrk failed:exiting....\n");
    exit(1);
  }

  /* TODO: Add code here to initialize freelist and its content. */
  initialize_freelist(heap_mem_start, MAX_HEAP_SIZE);

  return 0;
}

/**
 * Deallocates everything and frees back all the memory to the operating system.
 *
 * This routine is useful to do between tests so that we can reset everything.
 * You should not need to modify this routine though if you use global
 * variables, it might be useful to reset their values here.
 */
void rhfree_all(void)
{
  /* Imagine what would happen on a double free, yikes! */
  sbrk(-MAX_HEAP_SIZE);

  freelist = 0;
  heap_mem_start = 0;
}

/**
 * Allocate size bytes and return a pointer to start of the region. 
 * 
 * @return A valid void ptr if there is enough room, 0 on error. 
 */
void *rhmalloc(uint32 size)
{
  /* Check if we need to call rhmalloc_init and call it if needed. */
  if(!freelist)
    if(rhmalloc_init()) return 0;

  /* TODO: Add you malloc code here. */
  size = ALIGN(size);
  //round up to multiple of alignment
  metadata_t *current_block = freelist;
  //search freelist which fit the requrest size
  while(current_block != 0)
  {
    //find the block
    if (!current_block->s.in_use && current_block->s.size >= size)
    {
      //the size is greater than the requested size
      if(current_block->s.size >= size + sizeof(metadata_t) + ALIGNMENT)
      {
        metadata_t *new_block = (metadata_t *)((char*)current_block + sizeof(metadata_t) + size);
        new_block->s.size = current_block->s.size - size - sizeof(metadata_t);
        new_block->s.in_use = 0;
        new_block->s.prev = current_block;
        new_block->s.next = current_block->s.next;
        if (new_block->s.next != 0)
        {
          new_block->s.next->s.prev = new_block;
        }
        current_block->s.next = new_block;
        current_block->s.size = size;
      }

      //current block in use
      current_block->s.in_use = 1;
      return (void *)((char *)current_block + sizeof(metadata_t));
    }
  
    current_block = current_block->s.next;
  }

  //if no block found, return error
  return 0;
}

//merge block function
metadata_t *merge_blocks(metadata_t *block_first, metadata_t *block_second)
{
  block_first->s.size +=(block_second->s.size + sizeof(metadata_t));
  block_first->s.next = block_second->s.next;
  if(block_second->s.next != 0)
    block_second->s.next->s.prev = block_first;

  return block_first;
}

/**
 * Free a memory region and return it to the memory allocator.
 *
 * @param ptr The pointer to free.
 *
 * @warning This routine is not responsible for making sure that the free
 * operation will not result in an error. If freeing a pointer that has already
 * been freed, undefined behavior may occur.
 */
void rhfree(void *ptr)
{
  /* TODO: Add your free code here. */

  //assuming a pointer is pointing to the metadata of the block
  if (ptr == 0)
    return;

  metadata_t *current_block = (metadata_t *)((char *)ptr - sizeof(metadata_t));
  current_block->s.in_use = 0;
  
  //coalescing
  if(current_block->s.prev != 0 && !current_block->s.prev->s.in_use)
  {
    current_block = merge_blocks(current_block->s.prev, current_block);
  }

  if(current_block->s.next != 0 && !current_block->s.next->s.in_use)
  {
    current_block = merge_blocks(current_block, current_block->s.next);
  }

  

  
}
