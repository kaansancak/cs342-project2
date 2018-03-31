#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define N (10)
#define MAX_LINE (1024)
#define MAX_FILENAME (128)
#define MAX_KEYWORD (128)
#define MAX_SHM_NAME (128)
#define MAX_SEM_PREFIX (128)
#define BUFFER_SIZE (100)
#define SHM_SIZE (8192)

struct resultQueue {
  int in;
  int out;
  int buffer[BUFFER_SIZE];
};

struct request {
  char keyword[MAX_KEYWORD];
  int index;
};

struct requestQueue {
  int in;
  int out;
  struct request requests[BUFFER_SIZE];
};

struct sharedData {
  int number;
  struct resultQueue result_queues[N];
  int resultQueueStatus[N];
  struct requestQueue request_queue;
};

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

  printf("Test 1\n");
  shared_data = (struct sharedData *) sptr;
  printf("Test 2\n");
  int anumber = shared_data->number;
  printf("Test 3\n");

  printf("%d\n", anumber);

  int index = -1;
  for (int i = 0; i < N; i++) {
    if (shared_data->resultQueueStatus[i] == 0) {
      index = i;
      shared_data->resultQueueStatus[i] = 1;
      break;
    }
  }
  printf("Test2");

  if (index == -1) {
    return 0;
  }

  printf("Found empty queue at index %d: ", index);
  struct request req;
}
