
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
#include <semaphore.h>

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


sem_t semEmpty;
sem_t semFull;
sem_t sem_p;
sem_t sem_c;

pthread_mutex_t mutexBuffer;

pthread_mutex_t messagequeue;

static long number_of_Consumer_Producer;
static unsigned int g_msg_count = MSG_COUNT_DEFAULT;

void* consume(void* threadid) {
    long tid;
    tid = (long)threadid;
    mqd_t q;
    char buf[Q_ATTR_MSG_SIZE + 1];
    //int i;
    
    
    while ((q = mq_open(Q_NAME, Q_OFLAGS_CONSUMER)) == (mqd_t) -1) {
        if (errno == ENOENT) {
            printf("consume: Waiting for producer to create message queue...\n");
            usleep(Q_CREATE_WAIT_US);
            continue;
        }
        sleep(4);
        perror("consume: mq_open");
        return NULL;
    }
    
    
    
    
    while(1) {
        
        printf("[Consumer] : %ld waiting for message from Producer \n", tid);
        memset(buf, 0, sizeof(buf));
        
        //pthread_mutex_lock(&messagequeue);
        //sem_wait(&sem_c);
        if (mq_receive(q, buf, Q_ATTR_MSG_SIZE, NULL) < 0) {
            perror("consume: mq_receive");
        } else {
            printf("[Consumer] : %ld messege recieved --> %s\n", tid, buf);
        }
        //sem_post(&sem_c);
    }
    //pthread_mutex_unlock(&messagequeue);
    
    if (mq_close(q)) {
        perror("consume: mq_close");
    }
    // consumer destroys the queue
    if (mq_unlink(Q_NAME)) {
        perror("consume: mq_unlink");
    }
    return NULL;
}

void* produce(void* threadid) {
    mqd_t q;
    long tid;
    tid = (long)threadid;
    char buf[Q_ATTR_MSG_SIZE];
    struct mq_attr q_attr = {
        .mq_flags = Q_ATTR_FLAGS,       /* Flags: 0 or O_NONBLOCK */
        .mq_maxmsg = Q_ATTR_MAX_MSG,    /* Max. # of messages on queue */
        .mq_msgsize = Q_ATTR_MSG_SIZE,  /* Max. message size (bytes) */
        .mq_curmsgs = Q_ATTR_CURMSGS,   /* # of messages currently in queue */
    };
    
    //int i;
    // producer creates the queue
    
    if ((q = mq_open(Q_NAME, Q_OFLAGS_PRODUCER, Q_MODE, &q_attr)) == (mqd_t) -1) {
        perror("produce: mq_open");
        return NULL;
    }
    
    while(1) {
        int x = rand() % 100;
        
        sem_wait(&sem_p);
        //pthread_mutex_lock(&messagequeue);
        snprintf(buf, sizeof(buf), "Message %d", x);
        printf("[Producer] : %ld is ready to send \n", tid);
        
        if (mq_send(q, buf, strlen(buf) + 1, 0)) {
            perror("produce: mq_send");
        }
        printf("[Producer] : %ld message Sent --> %s\n",tid, buf);
        usleep(MSG_PERIOD_US);
        sem_post (&sem_p);
        sleep(4);
    }
    
    //pthread_mutex_unlock(&messagequeue);
    
    if (mq_close(q)) {
        perror("produce: mq_close");
    }
    return NULL;
}



int main(int argc, char** argv) {
    pthread_t th[THREAD_NUM];
    sem_init(&semEmpty, 0, 10);
    sem_init(&semFull, 0, 0);
    sem_init (&sem_c, 0, 1);
    sem_init (&sem_p, 0, 1);
    pthread_mutex_init(&messagequeue, NULL);
    srand(time(NULL));
    //int is_consumer;
    //      if (argc < 2) {
    //        fprintf(stderr, "Usage: %s <0|1> [msg_count]\n", argv[0]);
    //        fprintf(stderr, "  (0 = producer, 1 = consumer)\n");
    //        return EINVAL;
    //      }
    
    
    
//    is_consumer = atoi(argv[1]);
//    if (argc > 2) {
//        g_msg_count = atoi(argv[2]);
//    }
    
    //---------------------------------------------------------------
    
    //            for (number_of_Consumer_Producer = 0; number_of_Consumer_Producer < THREAD_NUM; number_of_Consumer_Producer ++) {
    //                if (number_of_Consumer_Producer % 2 == 0 && is_consumer == 0) {
    //                    printf("Producer thread %ld ID %lu\n",number_of_Consumer_Producer,pthread_self());
    //                    if (pthread_create(&th[number_of_Consumer_Producer], NULL, &produce, (void *)number_of_Consumer_Producer) != 0) {
    //                        perror("Failed to create thread");
    //                    }
    //            } else {
    //                printf("Consumer thread %ld ID %lu\n",number_of_Consumer_Producer,pthread_self());
    //                    if (pthread_create(&th[number_of_Consumer_Producer], NULL, is_consumer ? &consume : &produce, (void *)number_of_Consumer_Producer) != 0) {
    //                        perror("Failed to create thread");
    //                    }
    //            }
    //        }
    //
    //            for (number_of_Consumer_Producer = 0; number_of_Consumer_Producer < THREAD_NUM; number_of_Consumer_Producer ++) {
    //                if (pthread_join(th[number_of_Consumer_Producer], (void *) number_of_Consumer_Producer) != 0) {
    //                    perror("Failed to join thread");
    //                }
    //            }
    
    // if ((errno = pthread_join(thread, NULL))) {
    // perror("pthread_join");
    // return errno;
    // }
    
    //---------------------------------------------------------------
    long i;
    for (i = 0; i < THREAD_NUM; i++) {
        if (i % 2 == 0) {
            if (pthread_create(&th[i], NULL, &consume, (void *)i) != 0) {
                perror("Failed to create thread");
            }
            
        } else {
            if (pthread_create(&th[i], NULL, &produce, (void *)i) != 0) {
                perror("Failed to create thread");
            }
        }
    }
    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }
    sem_destroy(&sem_p);
    sem_destroy(&sem_c);
    sem_destroy(&semEmpty);
    sem_destroy(&semFull);
    pthread_mutex_destroy(&messagequeue);
    return 0;
}


