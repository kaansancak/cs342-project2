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


int main(int argc, char **argv) {

  int fd;
  void *sptr;
  struct sharedData *shared_data;
  struct stat sbuf;

  if (argc != 4) {
    printf("Wrong number of arguments for server\n");
    exit(-1);
  }

  char shm_name[MAX_SHM_NAME];
  char keyword[MAX_KEYWORD];
  char sem_prefix[MAX_SEM_PREFIX];

  strcpy(shm_name, argv[1]);
  strcpy(keyword, argv[2]);
  strcpy(sem_prefix, argv[3]);

  printf("%s\n", shm_name);
  printf("%s\n", keyword);
  printf("%s\n", sem_prefix);

  fd = shm_open(shm_name, O_RDWR, 0660);
  fstat(fd, &sbuf);
  sptr = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  close(fd);

  shared_data = (struct sharedData *) sptr;
  int index = -1;
  for (int i = 0; i < N; i++) {
    if (shared_data->resultQueueStatus[i] == 0) {
      index = i;
      shared_data->resultQueueStatus[i] = 1;
      break;
    }
  }

  if (index == -1) {
    perror("too many clients started");
    exit(1);
  }

  printf("Found empty queue at index: %d\n", index);

  struct request req;
  req.index = index;
  strcpy(req.keyword, keyword);

  shared_data->request_queue.requests[shared_data->request_queue.in] = req;
  shared_data->request_queue.in = (shared_data->request_queue.in + 1) % BUFFER_SIZE;
  printf("Request sent\n");


}
