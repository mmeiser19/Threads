#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include "msgq.h"
#include <time.h>

#define BUFFER_SIZE 100
#define NUM_MESSAGES 50
#define MSGQLEN 4

struct msgq *mq;
static int msg_num = 0; //used to assign message numbers for test 3
static int nameassign = 0; //used to assign names for test 3
//consumer name generator
char *namearray[3] = { "Alpha", "Bravo", "Charlie"};
//array of 3 arrays to hold messages
char *threadMsgs[3][NUM_MESSAGES * 2];

//create an array that can hold 3 nested arrays that hold up to 100 elements
//char *threadMsgs[3][BUFFER_SIZE];

// Define the semaphores
sem_t empty;
sem_t full;

//initialize buffer
char *buffer[BUFFER_SIZE];

int in = 0, out = 0; //buffer indices

char *messages[] = { "msg1", "msg2", "hellomsg", "gustymsg" };

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void produce_message(char* message, int num) {
    sprintf(message, "%d", num);
}

// Define the producer function
void *producer() {
    char message[20];
    for (int i = 0; i < NUM_MESSAGES; i++) {
        produce_message(message, msg_num++);

        sem_wait(&empty);
        pthread_mutex_lock(&mutex);
        msgq_send(mq, message);

        //insert_buffer(message);

        pthread_mutex_unlock(&mutex);
        sem_post(&full);
    }
    pthread_exit(NULL);
}

// Define the consumer function
void *consumer(void *arg) {
    pthread_mutex_lock(&mutex);
    //get name from namearray
    char *name = namearray[nameassign];
    nameassign += 1;
    pthread_mutex_unlock(&mutex);
    char* message;
    //for(int i = 0; i < NUM_MESSAGES; i++) {
    while (1) {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }
        ts.tv_sec += 1; //this controls how long to wait, change int if needed
        int waitstat = sem_timedwait(&full, &ts);
        if (waitstat == -1) {
            break;
        }
        pthread_mutex_lock(&mutex);

        message = msgq_recv(mq);
        printf("%s: %s\n", name, message);

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
    }
    pthread_exit(NULL);
}

// sends msgs in messages[]
void *promtAndSend(void *arg) {
    for (int i = 0; i < sizeof(messages)/sizeof(char*); i++) {
        char response[80];
        printf("Send? ");
        scanf("%s", response);
        if (response[0] == 'y' || response[0] == 'Y') {
            printf("sending: %s\n", messages[i]);
            msgq_send(mq, messages[i]);
        }
    }
    return NULL;
}

// consume messges in msgq
void *recvMsgs(void *arg) {
    sleep(5);
    int msg_count = msgq_len(mq);
    printf("mq msg_count: %d\n", msg_count);
    for (int i = 0; i < msg_count; i++) {
        char *m = msgq_recv(mq);
        printf("recvMsgs: %s\n", m);
        //free(m);
    }
    return NULL;
}

void *passiton(void *arg) {
    int me = (int) arg;
    while (1) {
        sleep(1);
        printf("passiton%d initial msgq_len: %d\n", me, msgq_len(mq));
        char *m = msgq_recv(mq);
        printf("passiton%d: %p %p %s\n", me, &m, m, m);
        printf("passiton%d after recv msgq_len: %d\n", me, msgq_len(mq));
        msgq_send(mq, m);
        printf("passiton%d after send msgq_len: %d\n", me, msgq_len(mq));
        free(m);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    mq = msgq_init(MSGQLEN);

    char test = '1';
    if (argc == 2)
        test = argv[1][0];
    switch (test) {
        case '1':
            printf("test fill and empty msgq\n");
            pthread_create(&p1, NULL, promtAndSend, NULL);
            pthread_join(p1, NULL);
            printf("msgq_show() after filling for test 1:\n");
            msgq_show(mq);
            pthread_create(&p2, NULL, recvMsgs, NULL);
            pthread_join(p2, NULL);
            printf("msgq_show() after all consumed by test 1:\n");
            msgq_show(mq);
            break;
        case '2': //has a bug with passition
            printf("test fill msgs and pass it on\n");
            pthread_create(&p1, NULL, promtAndSend, NULL);
            pthread_join(p1, NULL);
            printf("msgq_show() after filling for test 2:\n");
            msgq_show(mq);
            pthread_create(&p1, NULL, passiton, (void *)1);
            pthread_create(&p2, NULL, passiton, (void *)2);
            pthread_join(p1, NULL);
            pthread_join(p2, NULL);
            break;
        case '3':
            //initialize a msgq of size 100
            mq = msgq_init(100);

            // Initialize the semaphores
            sem_init(&empty, 0, BUFFER_SIZE);
            sem_init(&full, 0, 0);

            // Create the producer threads
            printf("Creating producers\n");
            pthread_t prod1, prod2;
            int prod_id1 = 1, prod_id2 = 2;
            pthread_create(&prod1, NULL, producer, &prod_id1);
            pthread_create(&prod2, NULL, producer, &prod_id2);

            // Wait for the producer threads to finish
            pthread_join(prod1, NULL);
            pthread_join(prod2, NULL);
            printf("Producers finished\n");

            // Wait for 5 seconds
            printf("Waiting for 5 seconds\n");
            sleep(5);

            // Create the consumer threads
            printf("Creating consumers\n");
            pthread_t cons1, cons2, cons3;
            int cons_id1 = 1, cons_id2 = 2, cons_id3 = 3;
            char *thread1[100];
            char *thread2[100];
            char *thread3[100];
            pthread_create(&cons1, NULL, consumer, &cons_id1);
            pthread_create(&cons2, NULL, consumer, &cons_id2);
            pthread_create(&cons3, NULL, consumer, &cons_id3);

            // Wait for the consumer threads to finish
            pthread_join(cons1, NULL);
            pthread_join(cons2, NULL);
            pthread_join(cons3, NULL);
            printf("Consumers finished\n");
            printf("\n");

            //print the contents of each nested array of threadMsgs along with the name of the thread
            /*for (int i = 0; i < 3; i++) {
                //print the name of the thread
                printf("%s: ", namearray[i]);
                for (int j = 0; j < NUM_MESSAGES; j++) {
                    printf("%c", threadMsgs[i][j]); //not printing contents correctly
                }
                printf("\n");
            }*/

            // Clean up the semaphores and exit
            sem_destroy(&empty);
            sem_destroy(&full);
            break;
        default:
            printf("invalid test selection!\n");
            break;
    }
    return 0;
}