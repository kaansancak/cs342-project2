#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "utils.h"
#include <signal.h>

char shm_name[MAX_SHM_NAME];
char input_file_name[MAX_FILENAME];
char sem_prefix[MAX_SEM_PREFIX];
struct sharedData *shared_data;

// SEMAPHORES
char SEMNAME_MUTEX[MAX_SHM_NAME];
char SEMNAME_FULL[MAX_SHM_NAME];
char SEMNAME_EMPTY[MAX_SHM_NAME];

char SEMNAME_STATUS_MUTEX[MAX_SHM_NAME];
char SEMNAME_STATUS_FULL[MAX_SHM_NAME];
char SEMNAME_STATUS_EMPTY[MAX_SHM_NAME];

char SEMNAME_REQUEST_MUTEX[MAX_SHM_NAME];
char SEMNAME_REQUEST_FULL[MAX_SHM_NAME];
char SEMNAME_REQUEST_EMPTY[MAX_SHM_NAME];

sem_t* result_queue_mutex_array[N];
sem_t* result_queue_full_array[N];
sem_t* result_queue_empty_array[N];
sem_t* status_mutex;
sem_t* status_full;
sem_t* status_empty;
sem_t* request_queue_mutex;
sem_t* request_queue_full;
sem_t* request_queue_empty;

static void cleanUp() {
  for (int i = 0; i < N; i++) {
    sem_close(result_queue_mutex_array[i]);
    sem_close(result_queue_full_array[i]);
    sem_close(result_queue_empty_array[i]);

    sprintf(SEMNAME_MUTEX, "%s%s%d", sem_prefix, "result_queue_mutex_", i);
    sprintf(SEMNAME_FULL, "%s%s%d", sem_prefix, "result_queue_full_", i);
    sprintf(SEMNAME_EMPTY, "%s%s%d", sem_prefix, "result_queue_empty_", i);

    sem_unlink(SEMNAME_MUTEX);
    sem_unlink(SEMNAME_FULL);
    sem_unlink(SEMNAME_EMPTY);
  }

  sem_close(status_mutex);
  sem_close(status_full);
  sem_close(status_empty);

  sem_unlink(SEMNAME_STATUS_MUTEX);
  sem_unlink(SEMNAME_STATUS_FULL);
  sem_unlink(SEMNAME_STATUS_EMPTY);

  sem_close(request_queue_mutex);
  sem_close(request_queue_full);
  sem_close(request_queue_empty);

  sem_unlink(SEMNAME_REQUEST_MUTEX);
  sem_unlink(SEMNAME_REQUEST_FULL);
  sem_unlink(SEMNAME_REQUEST_EMPTY);

  shm_unlink(shm_name);
}

static void sigint_handler() {
  cleanUp();
  exit(0);
}

void initSemaphores() {
  for (int i = 0; i < N; i++) {
    sprintf(SEMNAME_MUTEX, "%s%s%d", sem_prefix, "result_queue_mutex_", i);
    sprintf(SEMNAME_FULL, "%s%s%d", sem_prefix, "result_queue_full_", i);
    sprintf(SEMNAME_EMPTY, "%s%s%d", sem_prefix, "result_queue_empty_", i);

    result_queue_mutex_array[i] = sem_open(SEMNAME_MUTEX, O_RDWR | O_CREAT, 0660, 1);
    if (result_queue_mutex_array[i] < 0) {
      perror("can not create semaphore\n");
      exit (1);
    }

    result_queue_full_array[i] = sem_open(SEMNAME_FULL, O_RDWR | O_CREAT, 0660, 0);
    if (result_queue_full_array[i] < 0) {
      perror("can not create semaphore\n");
      exit (1);
    }

    result_queue_empty_array[i] = sem_open(SEMNAME_EMPTY, O_RDWR | O_CREAT, 0660, BUFFER_SIZE);
    if (result_queue_empty_array[i] < 0) {
      perror("can not create semaphore\n");
      exit (1);
    }
  }

  sprintf(SEMNAME_STATUS_MUTEX, "%s%s", sem_prefix, "status_mutex");
  sprintf(SEMNAME_STATUS_FULL, "%s%s", sem_prefix, "status_full");
  sprintf(SEMNAME_STATUS_EMPTY, "%s%s", sem_prefix, "status_empty");

  status_mutex = sem_open(SEMNAME_STATUS_MUTEX, O_RDWR | O_CREAT, 0660, 1);
  if (status_mutex < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  status_full = sem_open(SEMNAME_STATUS_FULL, O_RDWR | O_CREAT, 0660, 0);
  if (status_full < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  status_empty = sem_open(SEMNAME_STATUS_EMPTY, O_RDWR | O_CREAT, 0660, BUFFER_SIZE);
  if (status_empty < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  sprintf(SEMNAME_REQUEST_MUTEX, "%s%s", sem_prefix, "request_mutex");
  sprintf(SEMNAME_REQUEST_FULL, "%s%s", sem_prefix, "request_queue_full");
  sprintf(SEMNAME_REQUEST_EMPTY, "%s%s", sem_prefix, "request_queue_empty");

  request_queue_mutex = sem_open(SEMNAME_REQUEST_MUTEX, O_RDWR | O_CREAT, 0660, 1);
  if (request_queue_mutex < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  request_queue_full = sem_open(SEMNAME_REQUEST_FULL, O_RDWR | O_CREAT, 0660, 0);
  if (request_queue_full < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }

  request_queue_empty = sem_open(SEMNAME_REQUEST_EMPTY, O_RDWR | O_CREAT, 0660, BUFFER_SIZE);
  if (request_queue_empty < 0) {
    perror("can not create semaphore\n");
    exit (1);
  }
}

void *handleRequest(void *arg){
  struct request *req;
  req = (struct request *) arg;

  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char fileDir[MAX_FILENAME + 1];
  sprintf( fileDir, "./%s", input_file_name);
  int lineno = 1;

  fp = fopen( fileDir, "r");
  char keyword[MAX_KEYWORD];
  strcpy(keyword, req->keyword);

  if( fp == NULL){
    exit(1);
  }


  while( (read = getline( &line, &len, fp)) != -1){
    if(strstr(line, keyword) != NULL) {
      sem_wait(result_queue_empty_array[req->index]);
      sem_wait(result_queue_mutex_array[req->index]);
      shared_data->result_queues[req->index].buffer[shared_data->result_queues[req->index].in] = lineno;
      shared_data->result_queues[req->index].in = (shared_data->result_queues[req->index].in + 1) % BUFFER_SIZE;
      sem_post(result_queue_mutex_array[req->index]);
      sem_post(result_queue_full_array[req->index]);
    }
    lineno++;
  }

  fclose(fp);

  sem_wait(result_queue_empty_array[req->index]);
  sem_wait(result_queue_mutex_array[req->index]);
  shared_data->result_queues[req->index].buffer[shared_data->result_queues[req->index].in] = -1;
  shared_data->result_queues[req->index].in = (shared_data->result_queues[req->index].in + 1) % BUFFER_SIZE;
  sem_post(result_queue_mutex_array[req->index]);
  sem_post(result_queue_full_array[req->index]);

  if(line) {
    free(line);
  }
  pthread_exit(NULL);
}

int main(int argc, char **argv) {

  signal(SIGINT, sigint_handler);

  int fd;
  void *shm_start;
  struct stat sbuf;

  pthread_t tids[N];
  struct request t_args[N];

  if (argc != 4) {
    printf("Wrong number of arguments for server\n");
    exit(-1);
  }

  strcpy(shm_name, argv[1]);
  strcpy(input_file_name, argv[2]);
  strcpy(sem_prefix, argv[3]);

  // Clean up before start
  cleanUp();

  // Open new shared memory
  fd = shm_open(shm_name, O_RDWR | O_CREAT, 0660);
  ftruncate(fd, SHM_SIZE);
  fstat(fd, &sbuf);
  shm_start = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  shared_data = (struct sharedData *) shm_start;
  shared_data->request_queue.in = 0;
  shared_data->request_queue.out = 0;
  for (int i = 0; i < N; i++) {
    shared_data->resultQueueStatus[i] = 0;
  }

  initSemaphores();

  int ret;
  while (1) {

    // Queue is not empty, take the request and create a new thread to handle
    if (shared_data->request_queue.in != shared_data->request_queue.out) {
      // Take the request from request queue

      sem_wait(request_queue_full);
      sem_wait(request_queue_mutex);
      struct request req = shared_data->request_queue.requests[shared_data->request_queue.out];
      shared_data->request_queue.out = (shared_data->request_queue.out + 1) % BUFFER_SIZE;
      sem_post(request_queue_mutex);
      sem_post(request_queue_empty);

      // Create a new thread to handle the request
      t_args[req.index] = req;
      ret = pthread_create(&(tids[req.index]), NULL, handleRequest,(void *) &(t_args[req.index]));

      if (ret != 0) {
        printf("thread create failed\n");
        exit(1);
      }
    }
  }

  cleanUp();
  exit(0);
}
