#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "utils.h"

void handleRequest(struct request req){
  // TODO
}

int main(int argc, char **argv) {

  int fd;
  void *shm_start;
  struct sharedData *shared_data;
  struct stat sbuf;

  if (argc != 4) {
    printf("Wrong number of arguments for server\n");
    exit(-1);
  }

  char shm_name[MAX_SHM_NAME];
  char input_file_name[MAX_FILENAME];
  char sem_prefix[MAX_SEM_PREFIX];

  strcpy(shm_name, argv[1]);
  strcpy(input_file_name, argv[2]);
  strcpy(sem_prefix, argv[3]);

  printf("%s\n", shm_name);
  printf("%s\n", input_file_name);
  printf("%s\n", sem_prefix);

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

  while (1) {

    // Queue is not empty, take the request and create a new thread to handle
    if (shared_data->request_queue.in != shared_data->request_queue.out) {
      struct request req = shared_data->request_queue.requests[shared_data->request_queue.out];
      shared_data->request_queue.out = (shared_data->request_queue.out + 1) % BUFFER_SIZE;
    }
  }

exit(0);
}
