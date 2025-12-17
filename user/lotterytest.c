#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_PROCESSES 7
#define WORK_AMOUNT 5000000

struct result {
  int process_num;
  int pid;
  int tickets;
  int start_tick;
  int end_tick;
  int duration;
};

int
main(int argc, char *argv[])
{
  int tickets[NUM_PROCESSES] = {10, 20, 30, 40, 50, 60, 70};
  int i, pid;
  int pipe_fd[2];
  struct result results[NUM_PROCESSES];
  
  if(pipe(pipe_fd) < 0){
    printf("pipe failed\n");
    exit(1);
  }
  
  printf("Starting lottery scheduler test\n");
  printf("Tickets: 10, 20, 30, 40, 50, 60, 70\n\n");
  
  int test_start = uptime();
  
  // Create all processes
  for(i = 0; i < NUM_PROCESSES; i++){
    pid = fork();
    if(pid == 0){
      close(pipe_fd[0]);
      
      settickets(tickets[i]);
      
      int my_process_num = i + 1;
      int my_pid = getpid();
      int my_tickets = tickets[i];
      int work = 0;
      int start = uptime();
      
      // Do work silently
      while(work < WORK_AMOUNT){
        work++;
        if(work % 1000 == 0){
          yield();
        }
      }
      
      int end = uptime();
      
      // Send results through pipe
      struct result my_result;
      my_result.process_num = my_process_num;
      my_result.pid = my_pid;
      my_result.tickets = my_tickets;
      my_result.start_tick = start;
      my_result.end_tick = end;
      my_result.duration = end - start;
      
      write(pipe_fd[1], &my_result, sizeof(my_result));
      close(pipe_fd[1]);
      exit(0);
    }
  }
  
  // Parent collects results
  close(pipe_fd[1]);
  
  for(i = 0; i < NUM_PROCESSES; i++){
    read(pipe_fd[0], &results[i], sizeof(struct result));
    wait(0);
  }
  
  close(pipe_fd[0]);
  
  int test_end = uptime();
  
  // Print clean results
  printf("Test completed in %d ticks\n\n", test_end - test_start);
  
  printf("=== GANTT CHART DATA ===\n\n");
  printf("Process\tTickets\tPID\tStart\tEnd\tDuration\n");
  
  for(i = 0; i < NUM_PROCESSES; i++){
    printf("P%d\t%d\t%d\t%d\t%d\t%d\n",
           results[i].process_num,
           results[i].tickets,
           results[i].pid,
           results[i].start_tick,
           results[i].end_tick,
           results[i].duration);
  }
  
  
  
  // Sort by end time to see finish order
  printf("\n=== COMPLETION ORDER ===\n\n");
  printf("Order\tProcess\tTickets\tDuration\n");
  
  // Simple bubble sort by end_tick
  for(i = 0; i < NUM_PROCESSES - 1; i++){
    for(int j = 0; j < NUM_PROCESSES - i - 1; j++){
      if(results[j].end_tick > results[j+1].end_tick){
        // Swap
        struct result temp = results[j];
        results[j] = results[j+1];
        results[j+1] = temp;
      }
    }
  }
  
  for(i = 0; i < NUM_PROCESSES; i++){
    printf("%d\tP%d\t%d\t%d\n",
           i+1,
           results[i].process_num,
           results[i].tickets,
           results[i].duration);
  }

  exit(0);
}