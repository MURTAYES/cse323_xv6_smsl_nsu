#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "shm.h"

struct {
  struct spinlock lock;
  struct shm_segment segments[MAX_SHM_SEGMENTS];
} shm_table;

void
shminit(void)
{
  initlock(&shm_table.lock, "shm_table");
  
  for(int i = 0; i < MAX_SHM_SEGMENTS; i++){
    shm_table.segments[i].valid = 0;
    shm_table.segments[i].key = 0;
    shm_table.segments[i].shmid = 0;
    shm_table.segments[i].addr = 0;
    shm_table.segments[i].size = 0;
    shm_table.segments[i].refcount = 0;
  }
}

// Find a shared memory segment by key
int
shmfind(int key)
{
  for(int i = 0; i < MAX_SHM_SEGMENTS; i++){
    if(shm_table.segments[i].valid && shm_table.segments[i].key == key){
      return i;
    }
  }
  return -1;
}

// Allocate a new shared memory segment
int
shmalloc(int key, uint size)
{
  // Round up size to page boundary
  size = PGROUNDUP(size);
  
  acquire(&shm_table.lock);
  
  // Check if segment with this key already exists
  if(key != SHM_KEY_PRIVATE){
    int idx = shmfind(key);
    if(idx >= 0){
      // Segment exists, check size matches
      if(shm_table.segments[idx].size == size){
        shm_table.segments[idx].refcount++;
        release(&shm_table.lock);
        return shm_table.segments[idx].shmid;
      } else {
        release(&shm_table.lock);
        return -1; // Size mismatch
      }
    }
  }
  
  // Find free slot
  int free_idx = -1;
  for(int i = 0; i < MAX_SHM_SEGMENTS; i++){
    if(!shm_table.segments[i].valid){
      free_idx = i;
      break;
    }
  }
  
  if(free_idx < 0){
    release(&shm_table.lock);
    return -1; // No free slots
  }
  
  // Allocate physical memory
  void *pa = kalloc();
  if(pa == 0){
    release(&shm_table.lock);
    return -1; // Out of memory
  }
  
  // If size > PGSIZE, allocate more pages
  if(size > PGSIZE){
    // For simplicity, we only support single page for now
    kfree(pa);
    release(&shm_table.lock);
    return -1;
  }
  
  // Initialize segment
  memset(pa, 0, PGSIZE);
  shm_table.segments[free_idx].valid = 1;
  shm_table.segments[free_idx].key = key;
  shm_table.segments[free_idx].shmid = free_idx;
  shm_table.segments[free_idx].addr = pa;
  shm_table.segments[free_idx].size = size;
  shm_table.segments[free_idx].refcount = 1;
  
  release(&shm_table.lock);
  return free_idx;
}

// Get shared memory segment info
struct shm_segment*
shmget_segment(int shmid)
{
  if(shmid < 0 || shmid >= MAX_SHM_SEGMENTS)
    return 0;
  
  if(!shm_table.segments[shmid].valid)
    return 0;
  
  return &shm_table.segments[shmid];
}

// Increment reference count
void
shm_incref(int shmid)
{
  acquire(&shm_table.lock);
  if(shmid >= 0 && shmid < MAX_SHM_SEGMENTS && shm_table.segments[shmid].valid){
    shm_table.segments[shmid].refcount++;
  }
  release(&shm_table.lock);
}

// Decrement reference count and free if zero
void
shm_decref(int shmid)
{
  acquire(&shm_table.lock);
  
  if(shmid >= 0 && shmid < MAX_SHM_SEGMENTS && shm_table.segments[shmid].valid){
    shm_table.segments[shmid].refcount--;
    
    if(shm_table.segments[shmid].refcount <= 0){
      // Free the physical memory
      kfree(shm_table.segments[shmid].addr);
      shm_table.segments[shmid].valid = 0;
      shm_table.segments[shmid].addr = 0;
    }
  }
  
  release(&shm_table.lock);
}