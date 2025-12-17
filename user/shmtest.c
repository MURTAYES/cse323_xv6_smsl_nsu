#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SHM_KEY 5000
#define SHM_SIZE 4096

struct shared_data {
  int counter;
  int process_writes[5];
};

int
main(int argc, char *argv[])
{
  printf("\n=== SHARED MEMORY TEST ===\n\n");
  
  // Create shared memory
  int shmid = shmget(SHM_KEY, SHM_SIZE);
  printf("Shared memory ID: %d\n\n", shmid);
  
  // Parent initializes
  struct shared_data *shm = (struct shared_data*)shmat(shmid);
  shm->counter = 0;
  for(int i = 0; i < 5; i++){
    shm->process_writes[i] = 0;
  }
  shmdt((void*)shm);
  
  printf("5 processes, each writes 10 times\n");
  printf("Expected total: 50\n\n");
  
  // Create 5 child processes
  for(int i = 0; i < 5; i++){
    int pid = fork();
    if(pid == 0){
      struct shared_data *child_shm = (struct shared_data*)shmat(shmid);
      
      for(int j = 0; j < 10; j++){
        child_shm->counter++;
        child_shm->process_writes[i]++;
      }
      
      shmdt((void*)child_shm);
      exit(0);
    }
  }
  
  // Wait for all
  for(int i = 0; i < 5; i++){
    wait(0);
  }
  
  // Read results
  shm = (struct shared_data*)shmat(shmid);
  
  printf("=== RESULTS ===\n\n");
  printf("Process\tWrites\n");
  for(int i = 0; i < 5; i++){
    printf("P%d\t%d\n", i, shm->process_writes[i]);
  }
  
  printf("\nExpected: 50\n");
  printf("Actual:   %d\n", shm->counter);
  printf("Lost:     %d\n\n", 50 - shm->counter);
  
  if(shm->counter == 50){
    printf("SUCCESS: No race condition!\n");
  } else {
    printf("Race condition detected\n");
    printf("Lost %d%% of writes\n\n", ((50 - shm->counter) * 100) / 50);
  }
  
  shmdt((void*)shm);
  exit(0);
}