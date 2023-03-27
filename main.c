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

// SEE Labs/GdbLldbLab for more information on lldb - lowlevel debugger

struct msgq *mq;

static int msg_num = 0;

// Define the semaphores
sem_t empty;
sem_t full;

//initialize buffer
char *buffer[BUFFER_SIZE];

int in = 0, out = 0; //buffer indices

char *messages[] = { "msg1", "msg2", "hellomsg", "gustymsg" };

// Define the shared buffer and the mutex
//int buffer[BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void produce_message(char* message, int num) {
    sprintf(message, "Message %d", num);
    usleep(500000);
}

void insert_buffer(char* message) {
    // Allocate memory for the new message
    char* new_message = (char*) malloc(sizeof(char) * (strlen(message) + 1));
    // Copy the message to the new memory
    strcpy(new_message, message);
    // Insert the new message into the buffer
    buffer[in] = new_message; // ***** POSSIBLE ERROR HERE *****
    // Increment the index
    in = (in + 1) % BUFFER_SIZE;
}

// Define the producer function
void *producer() {
    char message[20];
    for (int i = 0; i < NUM_MESSAGES; i++) {
        produce_message(message, msg_num++);

        sem_wait(&empty);
        pthread_mutex_lock(&mutex);
        //printf("Producer Lock\n");

        insert_buffer(message);

        //printf("producer unlock\n");
        pthread_mutex_unlock(&mutex);
        sem_post(&full);
    }
    pthread_exit(NULL);
}

void consume_message(char* message) {
    printf("Consumed message: %s\n", message);
    free(message);
    //sleep for 500ms
    usleep(500000);
}

char* remove_buffer() {
    char* message = buffer[out];
    out = (out + 1) % BUFFER_SIZE;
    return message;
}

// Define the consumer function
void *consumer(void *arg) {
    char* message;
    //i is not reaching NUM_MESSAGES when there are more consumers than producers due to i's in threads being independent of each other
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
        //printf("Consumer Lock\n");

        message = remove_buffer();
        consume_message(message);

        //printf("Consumer unlock %d\n",waitstat);
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

#define MSGQLEN 4

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    mq = msgq_init(MSGQLEN);

    //struct timespec ts;

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
            // Initialize the semaphores
            sem_init(&empty, 0, BUFFER_SIZE);
            sem_init(&full, 0, 0);

            // Create the producer threads
            pthread_t prod1, prod2;
            int prod_id1 = 1, prod_id2 = 2;
            pthread_create(&prod1, NULL, producer, &prod_id1);
            pthread_create(&prod2, NULL, producer, &prod_id2);

            // Create the consumer threads
            pthread_t cons1, cons2, cons3;
            int cons_id1 = 1, cons_id2 = 2, cons_id3 = 3;
            pthread_create(&cons1, NULL, consumer, &cons_id1);
            pthread_create(&cons2, NULL, consumer, &cons_id2);
            pthread_create(&cons3, NULL, consumer, &cons_id3);

            // Wait for all threads to finish (which will never happen in this case)
            pthread_join(prod1, NULL);
            pthread_join(prod2, NULL);
            pthread_join(cons1, NULL);
            pthread_join(cons2, NULL);
            pthread_join(cons3, NULL);

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