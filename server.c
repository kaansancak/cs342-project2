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
#include "utils.h"

char shm_name[MAX_SHM_NAME];
char input_file_name[MAX_FILENAME];
char sem_prefix[MAX_SEM_PREFIX];
struct sharedData *shared_data;

void *handleRequest(void *arg){
  struct request *req;
  req = (struct request *) arg;

  printf("Thread is active for child: %d\n", req->index);
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
      shared_data->result_queues[req->index].buffer[shared_data->result_queues[req->index].in] = lineno;
      shared_data->result_queues[req->index].in = (shared_data->result_queues[req->index].in + 1) % BUFFER_SIZE;
    }
    lineno++;
  }

  fclose(fp);

  shared_data->result_queues[req->index].buffer[shared_data->result_queues[req->index].in] = -1;
  shared_data->result_queues[req->index].in = (shared_data->result_queues[req->index].in + 1) % BUFFER_SIZE;

  if(line) {
    free(line);
  }
  pthread_exit(NULL);
}

int main(int argc, char **argv) {

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

  int ret;
  while (1) {

    // Queue is not empty, take the request and create a new thread to handle
    if (shared_data->request_queue.in != shared_data->request_queue.out) {

      // Take the request from request queue
      struct request req = shared_data->request_queue.requests[shared_data->request_queue.out];
      shared_data->request_queue.out = (shared_data->request_queue.out + 1) % BUFFER_SIZE;

      // Create a new thread to handle the request
      t_args[req.index] = req;
      ret = pthread_create(&(tids[req.index]), NULL, handleRequest,(void *) &(t_args[req.index]));

      if (ret != 0) {
        printf("thread create failed\n");
        exit(1);
      }

      printf("thread created for child: %d\n", req.index);
    }
  }

exit(0);
}
