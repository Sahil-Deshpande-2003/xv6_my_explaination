// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run { // This defines a structure named run, which represents a block of memory.
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist; // a pointer to the free list of memory blocks.
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem"); // This line initializes the spin lock kmem.lock using the initlock function
  kmem.use_lock = 0; // This flag indicates whether locking mechanisms should be used or not. By setting it to 0, it indicates that locking mechanisms are not yet activated.
  freerange(vstart, vend); //  The freerange function is responsible for freeing the memory within the specified range
}

void
kinit2(void *vstart, void *vend)
{
  // 
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  // list of free pages head is kmem free list
  char *p;
  p = (char*)PGROUNDUP((uint)vstart); // This line calculates the starting address for freeing memory.
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE) // This for loop iterates over each page within the memory range specified by vstart and vend. It starts with the page pointed to by p and increments p by the page size (PGSIZE) until it reaches or exceeds vend.
    kfree(p); // the kfree function is called to free the memory pointed to by p. This function deallocates a page of physical memory and updates the free list accordingly. The freelist contains a linked list of free memory blocks available for allocation
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  // adds a head ptr to the LL

  // kfree -> range me 4 KB ke pages banata hai range is 2049 -2052 MB bich ka 3 MB

  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs. 
  
  /*
  In this context, v is the starting address of the memory block to be filled.

1 is the byte pattern to be written into each byte of the memory block.

PGSIZE specifies the number of bytes to be filled, which in this case is the size of a page (4096 bytes)
  */
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock); // If locking is enabled (kmem.use_lock == 1), it acquires the lock associated with the memory allocator before proceeding with the deallocation. This ensures mutual exclusion when accessing shared data structures.
  r = (struct run*)v;
  r->next = kmem.freelist; // It sets the next pointer of the node r to the current head of the free list, effectively linking r to the rest of the free list.
  kmem.freelist = r; // This line updates the head of the free list to point to the newly deallocated memory block r. This effectively adds the deallocated block to the beginning of the free list
  if(kmem.use_lock)
    release(&kmem.lock); // If locking is enabled, it releases the lock associated with the memory allocator. This allows other threads to access the memory allocator concurrently.
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{ // This defines the function kalloc, which allocates one page (4096 bytes) of physical memory and returns a pointer to it. The function returns a char* pointer, which points to the allocated memory block.
  struct run *r; // This structure represents a memory block in the free list.

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist; // r points to head
  if(r)
    kmem.freelist = r->next; // If r is not NULL (meaning there was a free memory block available), this line updates the free list to point to the next node in the list after the one that was allocated. This effectively removes the allocated block from the free list.
    // head ko chin lena and head ptr updated here r is page
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}
