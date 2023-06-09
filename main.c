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
static int msg_num = 0; // Used to assign message numbers for test 3 to show that the messages were produced and consumed in order
char *namearray[3] = { "Alpha", "Bravo", "Charlie"}; // Array of consumer names for test 3
char *consumerMsgs[3][BUFFER_SIZE]; // Array of arrays to hold messages for each consumer

// Define the semaphores
sem_t empty;
sem_t full;

char *messages[] = { "msg1", "msg2", "hellomsg", "gustymsg" };

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Producer function that produces messages and sends them to the message queue
 */
void *producer() {
    char message[20];
    for (int i = 0; i < NUM_MESSAGES; i++) {
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);

        sprintf(message, "%d", msg_num++);
        msgq_send(mq, message);

        pthread_mutex_unlock(&mutex);
        sem_post(&full);
    }
    return NULL;
}

/*
 * Consumer function that consumes messages from the message queue and inserts them into a static 2D array for later
 * use to show which consumers consumed which messages
 */
void *consumer(void *arg) {
    pthread_mutex_lock(&mutex);
    int i = (int)arg; // Consumer number used to insert messages into array correct index of consumerMsgs
    int currIndex = 0; // Index of array for the message to be inserted into
    pthread_mutex_unlock(&mutex);

    while (1) {
        // Wait for a message to be available
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            perror("clock_gettime");
            exit(EXIT_FAILURE);
        }
        ts.tv_sec += 1; // This controls how long to wait for a message
        int waitstat = sem_timedwait(&full, &ts);
        if (waitstat == -1) {
            break;
        }

        // When a message is available, consume it
        pthread_mutex_lock(&mutex);
        char *message = msgq_recv(mq);
        consumerMsgs[i][currIndex] = strdup(message); // Add message to array
        currIndex += 1; // Increment index for next message
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
    }
    return NULL;
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
        free(m);
        if (msgq_len(mq) == 0) {
            printf("\n");
            printf("Message queue is now empty\n");
            printf("Blocking msgq_recv until a message is sent\n");
        }
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
            mq = msgq_init(100); // Initialize the message queue with a size of 100

            // Initialize semaphores
            sem_init(&empty, 0, BUFFER_SIZE);
            sem_init(&full, 0, 0);

            // Create producer threads
            printf("Creating producers\n");
            printf("Filling up message queue\n");
            pthread_t prod1, prod2;
            pthread_create(&prod1, NULL, producer, NULL);
            pthread_create(&prod2, NULL, producer, NULL);

            // Create consumer threads
            printf("Creating consumers\n");
            pthread_t cons1, cons2, cons3;
            pthread_create(&cons1, NULL, consumer, (void *)0);
            pthread_create(&cons2, NULL, consumer, (void *)1);
            pthread_create(&cons3, NULL, consumer, (void *)2);

            // Join the producers
            pthread_join(prod1, NULL);
            pthread_join(prod2, NULL);
            printf("Producers finished\n");

            // Join the consumers
            pthread_join(cons1, NULL);
            pthread_join(cons2, NULL);
            pthread_join(cons3, NULL);
            printf("Consumers finished\n");

            if (msgq_len(mq) == 0) {
                printf("Message queue is now empty\n");
                printf("Blocking msgq_recv until a message is sent\n");
                printf("\n");
            }

            // Print the messages consumed by each consumer
            for (int i = 0; i < 3; i++) {
                int length = 0;
                printf("%s: ", namearray[i]);
                while (consumerMsgs[i][length] != NULL) {
                    printf("%s, ", consumerMsgs[i][length]);
                    length += 1;
                }
                printf("\n");
            }

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