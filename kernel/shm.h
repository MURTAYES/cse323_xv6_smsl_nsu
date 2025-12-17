#ifndef SHM_H
#define SHM_H

// Use the same value as in proc.h
#ifndef MAX_SHM_SEGMENTS
#define MAX_SHM_SEGMENTS 16
#endif

#define SHM_KEY_PRIVATE 0

struct shm_segment {
  int key;              // Unique identifier
  int shmid;            // Shared memory ID
  void *addr;           // Physical address of shared memory
  uint size;            // Size in bytes
  int refcount;         // Number of processes attached
  int valid;            // Is this segment valid?
};

#endif