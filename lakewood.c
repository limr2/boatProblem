#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

long global_data = 0;
pthread_mutex_t mutex1;
int avail = 10;
struct queue que;
int quecount;

struct node {
  int data;
  struct node *next;
};

struct queue {
  struct node *head;
  struct node *tail;
};

// initialize
void queue_init (struct queue *queue) {
  queue->head = NULL;
  queue->tail = NULL;
  quecount = 0;
}

// check if empty
_Bool queue_isEmpty (struct queue *queue) {
  return queue->head == NULL;
}

// insert value into queue
void queue_insert (struct queue* queue, int value) {
  struct node *tmp = malloc(sizeof(struct node));
  if (tmp == NULL) {
    fputs ("malloc failed\n", stderr);
    exit(1);
  }

  tmp->data = value;
  tmp->next = NULL;

  if (queue->head == NULL) {
    queue->head = tmp;
  } else {
    queue->tail->next = tmp;
  }
  queue->tail = tmp;
  quecount++;
}

// remove root value and return it
int queue_remove ( struct queue *queue ) {
  int retval = 0;
  struct node *tmp;

  if (!queue_isEmpty(queue)) {
    tmp = queue->head;
    retval = tmp->data;
    queue->head = tmp->next;
    free(tmp);
  }
  quecount--;
  return retval;
}

// ERROR WITH THREAD ID //
void fatal (long n) {
  printf ("Fatal error, lock or unlock error, thread %ld.\n", n);
  exit(n);
}

// MAKE THREAD //
void * thread_body ( void *arg ) {

  long threadn = (long) arg;
  int r = rand() % 3 + 1;
  int lj;

  // set life jackets
  if(r == 3){
    lj = 4;
  }
  else{
    lj = r;
  }

  // print water craft requested
  if(r == 1){
    printf("group number %ld\nwater craft requested: kayak\nlife jackets needed: %d\n",
    threadn+1, lj);
  }
  else if(r == 2){
    printf("group number %ld\nwater craft requested: canoe\nlife jackets needed: %d\n",
    threadn+1, lj);
  }
  else{
    printf("group number %ld\nwater craft requested: sailboat\nlife jackets needed: %d\n",
    threadn+1, lj);
  }

  if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
  // if not enough, group wait
  if(lj > avail){
    if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
    // less than 5 groups waiting
    if(quecount < 10){
      queue_insert(&que, threadn);
      queue_insert(&que, lj);
      printf("group %ld will wait\n", threadn + 1);
    }
    // 5 or more groups waiting
    else{
      printf("group %ld will not wait\n", threadn + 1);
      pthread_exit(0);
    }
  }
  // if enough life jackets
  else{
    if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
    // queue is empty
    if(queue_isEmpty(&que)){
      if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
      avail -= lj;
      printf("group %ld will use %d lifejacket(s)\n", threadn + 1, lj);
      printf("remaining life jackets: %d\n", avail);
      if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }

      sleep(rand() % 7 + 1);

      if (pthread_mutex_lock(&mutex1)) { fatal(threadn); }
      avail += lj;
      printf("returned %d life jacket(s)\n", lj);
      printf("remaining life jackets: %d\n", avail);
      if (pthread_mutex_unlock(&mutex1)) { fatal(threadn); }
    }
    // add to queue
    else{
      // less than 5 groups waiting
      if(quecount < 10){
        queue_insert(&que, threadn);
        queue_insert(&que, lj);
        printf("group %ld will wait\n", threadn + 1);
      }
      // 5 or more groups waiting
      else{
        printf("group %ld will not wait\n", threadn + 1);
        pthread_exit(0);
      }
    }
  }
  // go through queue
  if(!queue_isEmpty(&que)){
    int gothrough = quecount;
    int ix = 0;
    int currgroup;
    int currlifej;
    int reset = 0;
    while(ix < gothrough){
      currgroup = queue_remove(&que);
      currlifej = queue_remove(&que);
      ix += 2;
      if(reset == 0){
        if (pthread_mutex_lock(&mutex1)) { fatal(currgroup); }
        // enough life jackets avail
        if(avail >= currlifej){
          if (pthread_mutex_unlock(&mutex1)) { fatal(currgroup); }

          if (pthread_mutex_lock(&mutex1)) { fatal(currgroup); }
          avail -= currlifej;
          printf("group %d will use %d lifejacket(s)\n", currgroup + 1, currlifej);
          printf("remaining life jackets: %d\n", avail);
          if (pthread_mutex_unlock(&mutex1)) { fatal(currgroup); }

          sleep(rand() % 7 + 1);

          if (pthread_mutex_lock(&mutex1)) { fatal(currgroup); }
          avail += currlifej;
          printf("returned %d life jacket(s)\n", currlifej);
          printf("remaining life jackets: %d\n", avail);
          if (pthread_mutex_unlock(&mutex1)) { fatal(currgroup); }
        }
        // rebuild queue
        else{
          if (pthread_mutex_unlock(&mutex1)) { fatal(currgroup); }
          queue_insert(&que, currgroup);
          queue_insert(&que, currlifej);
          reset = 1;
        }
      }
      // rebuild queue
      else{
        queue_insert(&que, currgroup);
        queue_insert(&que, currlifej);
      }
    }
  }
  pthread_exit(0);
}

// MAIN //
int main (int argc, char **argv) {

  // set arguments
  int arg1 = 20;
  int arg2;
  int arg3;
  if(argc > 1){
    arg1 = atoi(argv[1]);
    if(argc > 2){
      arg2 = atoi(argv[2]) / 2;
      if(argc > 3){
        arg3 = time(NULL);
      }
      else{
        arg3 = 0;
      }
    }
    else{
      arg2 = 10;
    }
  }

  // initialize
  srandom(arg3);
  pthread_t ids[arg1];
  pthread_mutex_init(&mutex1, NULL);
  queue_init(&que);

  // create loop
  int err;
  long i;
  for (i = 0; i < arg1; i++) {
    sleep(rand() % arg2 + 1);
    err = pthread_create (&ids[i], NULL, thread_body, (void *)i);
    if (err) {
      fprintf (stderr, "Can't create thread %ld\n", i);
      exit (1);
    }
  }

  // get values from thread
  void *retval;
  for (i=0; i < arg1; i++) {
    pthread_join(ids[i], &retval);
  }

  pthread_mutex_destroy(&mutex1);
  queue_remove(&que);
  return 0;
}
