#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N (10)
#define MAX_LINE (1024)
#define MAX_FILENAME (128)
#define MAX_KEYWORD (128)
#define MAX_SHM_NAME (128)
#define MAX_SEM_PREFIX (128)
#define BUFSIZE (100)

int main(int argc, char **argv) {

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

  printf("%s", shm_name);
  printf("%s", input_file_name);
  printf("%s", sem_prefix);

  exit(0);
}
