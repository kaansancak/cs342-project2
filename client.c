#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include "utils.h"

char shm_name[MAX_SHM_NAME];
char keyword[MAX_KEYWORD];
char sem_prefix[MAX_SEM_PREFIX];

// SEMAPHORES
sem_t* result_queue_mutex_array[N];
sem_t* result_queue_full_array[N];
sem_t* result_queue_empty_array[N];
sem_t* status_mutex;
sem_t* status_full;
sem_t* status_empty;
sem_t* request_queue_mutex;
sem_t* request_queue_full;
sem_t* request_queue_empty;

void initSemaphores() {
  for (int i = 0; i < N; i++) {
    char SEMNAME_MUTEX[MAX_SHM_NAME];
    char SEMNAME_FULL[MAX_SHM_NAME];
    char SEMNAME_EMPTY[MAX_SHM_NAME];
    sprintf(SEMNAME_MUTEX, "%s%s%d", sem_prefix, "result_queue_mutex_", i);
    sprintf(SEMNAME_FULL, "%s%s%d", sem_prefix, "result_queue_full_", i);
    sprintf(SEMNAME_EMPTY, "%s%s%d", sem_prefix, "result_queue_empty_", i);

    result_queue_mutex_array[i] = sem_open(SEMNAME_MUTEX, O_RDWR);
    if (result_queue_mutex_array[i] < 0) {
      perror("can not create semaphore\n");
      exit (1);
    }

    result_queue_full_array[i] = sem_open(SEMNAME_FULL, O_RDWR);
    if (result_queue_full_array[i] < 0) {
      perror("can not create semaphore\n");
      exit (1);
    }

    result_queue_empty_array[i] = sem_open(SEMNAME_EMPTY, O_RDWR);
    if (result_queue_empty_array[i] < 0) {
      perror("can not create semaphore\n");
      exit (1);
    }
  }

  char SEMNAME_STATUS_MUTEX[MAX_SHM_NAME];
  char SEMNAME_STATUS_FULL[MAX_SHM_NAME];
  char SEMNAME_STATUS_EMPTY[MAX_SHM_NAME];
  sprintf(SEMNAME_STATUS_MUTEX, "%s%s", sem_prefix, "status_mutex");
  sprintf(SEMNAME_STATUS_FULL, "%s%s", sem_prefix, "status_full");
  sprintf(SEMNAME_STATUS_EMPTY, "%s%s", sem_prefix, "status_empty");

  status_mutex = sem_open(SEMNAME_STATUS_MUTEX, O_RDWR);
  if (status_mutex < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  status_full = sem_open(SEMNAME_STATUS_FULL, O_RDWR);
  if (status_full < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  status_empty = sem_open(SEMNAME_STATUS_EMPTY, O_RDWR);
  if (status_empty < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  char SEMNAME_REQUEST_MUTEX[MAX_SHM_NAME];
  char SEMNAME_REQUEST_FULL[MAX_SHM_NAME];
  char SEMNAME_REQUEST_EMPTY[MAX_SHM_NAME];
  sprintf(SEMNAME_REQUEST_MUTEX, "%s%s", sem_prefix, "request_mutex");
  sprintf(SEMNAME_REQUEST_FULL, "%s%s", sem_prefix, "request_queue_full");
  sprintf(SEMNAME_REQUEST_EMPTY, "%s%s", sem_prefix, "request_queue_empty");

  request_queue_mutex = sem_open(SEMNAME_REQUEST_MUTEX, O_RDWR);
  if (request_queue_mutex < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  request_queue_full = sem_open(SEMNAME_REQUEST_FULL, O_RDWR);
  if (request_queue_full < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  request_queue_empty = sem_open(SEMNAME_REQUEST_EMPTY, O_RDWR);
  if (request_queue_empty < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }
}

int main(int argc, char **argv) {

  int fd;
  void *sptr;
  struct sharedData *shared_data;
  struct stat sbuf;

  if (argc != 4) {
    printf("Wrong number of arguments for server\n");
    exit(-1);
  }

  strcpy(shm_name, argv[1]);
  strcpy(keyword, argv[2]);
  strcpy(sem_prefix, argv[3]);

  fd = shm_open(shm_name, O_RDWR, 0660);
  fstat(fd, &sbuf);
  sptr = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  close(fd);
  shared_data = (struct sharedData *) sptr;

  initSemaphores();

  // Find an empty index in the result queues
  int index = -1;

  sem_wait(status_empty);
  sem_wait(status_mutex);
  for (int i = 0; i < N; i++) {
    if (shared_data->resultQueueStatus[i] == 0) {
      index = i;
      shared_data->resultQueueStatus[i] = 1;
      break;
    }
  }
  sem_post(status_mutex);
  sem_post(status_full);

  if (index == -1) {
    printf("too many clients started");
    exit(1);
  }

  struct request req;
  req.index = index;
  strcpy(req.keyword, keyword);

  // TODO SEMAPHORE HERE
  shared_data->request_queue.requests[shared_data->request_queue.in] = req;
  shared_data->request_queue.in = (shared_data->request_queue.in + 1) % BUFFER_SIZE;


  while (1) {
    if(shared_data->result_queues[index].in != shared_data->result_queues[index].out) {
      sem_wait(result_queue_full_array[index]);
      sem_wait(result_queue_mutex_array[index]);
      int lineNo = shared_data->result_queues[index].buffer[shared_data->result_queues[index].out];
      shared_data->result_queues[index].out = (shared_data->result_queues[index].out + 1) % BUFFER_SIZE;
      sem_post(result_queue_mutex_array[index]);
      sem_post(result_queue_empty_array[index]);

      if (lineNo == -1)
        break;

      printf("%d\n", lineNo);
    }
  }

  // CLEAN UP
  shared_data->result_queues[index].in = 0;
  shared_data->result_queues[index].out = 0;

  sem_wait(status_full);
  sem_wait(status_mutex);
  shared_data->resultQueueStatus[index] = 0;
  sem_post(status_mutex);
  sem_post(status_empty);
}
