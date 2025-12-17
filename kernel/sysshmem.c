#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "shm.h"

// System call: shmget(key, size)
// Returns shared memory ID or -1 on error
uint64
sys_shmget(void)
{
  int key;
  int size;
  
  argint(0, &key);
  argint(1, &size);
  
  if(size <= 0 || size > PGSIZE){
    return -1;
  }
  
  int shmid = shmalloc(key, size);
  return shmid;
}

// System call: shmat(shmid)
// Returns virtual address where shared memory is mapped, or -1 on error
uint64
sys_shmat(void)
{
  int shmid;
  
  argint(0, &shmid);
  
  struct proc *p = myproc();
  struct shm_segment *seg = shmget_segment(shmid);
  
  if(seg == 0){
    return -1;
  }
  
  // Find free slot in process's shm array
  int slot = -1;
  for(int i = 0; i < MAX_SHM_SEGMENTS; i++){
    if(p->shm_ids[i] == -1){
      slot = i;
      break;
    }
  }
  
  if(slot < 0){
    return -1; // No free slots
  }
  
  // Find a free virtual address in user space
  // We'll map it just after the process's heap
  uint64 va = PGROUNDUP(p->sz);
  
  // Map the physical page to virtual address
  if(mappages(p->pagetable, va, PGSIZE, (uint64)seg->addr, 
              PTE_R | PTE_W | PTE_U) != 0){
    return -1;
  }
  
  // Update process size
  p->sz = va + PGSIZE;
  
  // Record the mapping
  p->shm_addr[slot] = (void*)va;
  p->shm_ids[slot] = shmid;
  
  shm_incref(shmid);
  
  return va;
}

// System call: shmdt(addr)
// Detach shared memory at given virtual address
uint64
sys_shmdt(void)
{
  uint64 addr;
  
  argaddr(0, &addr);
  
  struct proc *p = myproc();
  
  // Find the shared memory segment
  int slot = -1;
  for(int i = 0; i < MAX_SHM_SEGMENTS; i++){
    if(p->shm_addr[i] == (void*)addr){
      slot = i;
      break;
    }
  }
  
  if(slot < 0){
    return -1; // Not found
  }
  
  // Unmap the page
  uvmunmap(p->pagetable, addr, 1, 0);
  
  // Decrement reference count
  shm_decref(p->shm_ids[slot]);
  
  // Clear the slot
  p->shm_addr[slot] = 0;
  p->shm_ids[slot] = -1;
  
  return 0;
}