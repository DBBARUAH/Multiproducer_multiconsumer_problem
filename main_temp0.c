/**
 * In this POSIX MQ example, a producer sends messages to a consumer.
 * The producer creates the message queue, then starts sending messages.
 * The producer will block if/while the message queue is full.
 * The consumer waits for the producer to create the message queue, then blocks waiting for messages.
 * The consumer destroys the queue after it finishes reading messages.
 *
 * @author Connor Imes
 * @date 2018-08-31
 */
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define Q_NAME              "/example"
#define Q_OFLAGS_CONSUMER   (O_RDONLY)
#define Q_OFLAGS_PRODUCER   (O_CREAT | O_WRONLY)
#define Q_MODE              (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define Q_ATTR_FLAGS        0
#define Q_ATTR_MSG_SIZE     1024
#define Q_ATTR_MAX_MSG      3
#define Q_ATTR_CURMSGS      0

#define MSG_COUNT_DEFAULT   10
#define MSG_PERIOD_US       100000

#define Q_CREATE_WAIT_US    2000000
#define THREAD_NUM          8

pthread_mutex_t messagequeue;

static unsigned int g_msg_count = MSG_COUNT_DEFAULT;

void* consume(void* args) {
  mqd_t q;
  char buf[Q_ATTR_MSG_SIZE + 1];
  int i;
  pthread_mutex_lock(&messagequeue);
  
  while ((q = mq_open(Q_NAME, Q_OFLAGS_CONSUMER)) == (mqd_t) -1) {
    if (errno == ENOENT) {
      printf("consume: Waiting for producer to create message queue...\n");
      usleep(Q_CREATE_WAIT_US);
      continue;
    }
    perror("consume: mq_open");
    return NULL;
  }
  printf("Message_queue is open by Producer");

  while(1) {
    memset(buf, 0, sizeof(buf));
    if (mq_receive(q, buf, Q_ATTR_MSG_SIZE, NULL) < 0) {
      perror("consume: mq_receive");
    } else {
      printf("consume: recv: %s\n", buf);
    }
  }
  pthread_mutex_unlock(&messagequeue);

  if (mq_close(q)) {
    perror("consume: mq_close");
  }
  // consumer destroys the queue
  if (mq_unlink(Q_NAME)) {
    perror("consume: mq_unlink");
  }
  return NULL;
}

void* produce(void* args) {
  mqd_t q;
  char buf[Q_ATTR_MSG_SIZE];
  struct mq_attr q_attr = {
    .mq_flags = Q_ATTR_FLAGS,       /* Flags: 0 or O_NONBLOCK */
    .mq_maxmsg = Q_ATTR_MAX_MSG,    /* Max. # of messages on queue */
    .mq_msgsize = Q_ATTR_MSG_SIZE,  /* Max. message size (bytes) */
    .mq_curmsgs = Q_ATTR_CURMSGS,   /* # of messages currently in queue */
  };

  int i;

  // producer creates the queue
  if ((q = mq_open(Q_NAME, Q_OFLAGS_PRODUCER, Q_MODE, &q_attr)) == (mqd_t) -1) {
    perror("produce: mq_open");
    return NULL;
  }

  while(1) {
    snprintf(buf, sizeof(buf), "Message");
    printf("produce: send: %s\n", buf);
    if (mq_send(q, buf, strlen(buf) + 1, 0)) {
      perror("produce: mq_send");
    }
    usleep(MSG_PERIOD_US);
  }

  if (mq_close(q)) {
    perror("produce: mq_close");
  }
  return NULL;
}

int main(int argc, char** argv) {
  pthread_t th[THREAD_NUM]; 
 int is_consumer;
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <0|1> [msg_count]\n", argv[0]);
    fprintf(stderr, "  (0 = producer, 1 = consumer)\n");
    return EINVAL;
  }
  is_consumer = atoi(argv[1]);
  if (argc > 2) {
    g_msg_count = atoi(argv[2]);
  }

int i;
    for (i = 0; i < THREAD_NUM; i++) {
        if (i % 2 == 0) {
            if (pthread_create(&th[i], NULL,is_consumer ? &consume : &produce, NULL) != 0) {
                perror("Failed to create thread");
            }
        } else {
            if (pthread_create(&th[i], NULL, is_consumer ? &consume : &produce, NULL) != 0) {
                perror("Failed to create thread");
            }
        }
}

    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }

 // if ((errno = pthread_join(thread, NULL))) {
   // perror("pthread_join");
   // return errno;
 // }
  return 0;
}

