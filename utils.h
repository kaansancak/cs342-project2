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
  int buffer[BUFFER_SIZE + 1];
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
  struct resultQueue result_queues[N];
  int resultQueueStatus[N];
  struct requestQueue request_queue;
};
