#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include "msgq.h"

#define BUFFER_SIZE 100
#define NUM_MESSAGES 50

// SEE Labs/GdbLldbLab for more information on lldb - lowlevel debugger

struct msgq *mq;

// Define the semaphores
sem_t empty;
sem_t full;

//initialize buffer
int buffer[BUFFER_SIZE];

char *messages[] = { "msg1", "msg2", "hellomsg", "gustymsg" };

// Define the shared buffer and the mutex
int buffer[BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Define the producer function
void *producer(void *arg) {
    int prod_index = 0;
    char *msg_buffer = malloc(sizeof(char) * 80 * NUM_MESSAGES);
    int i = 0;
    while (i < NUM_MESSAGES) {
        // Wait for an empty slot in the buffer
        sem_wait(&empty);
        // Acquire the mutex to protect the buffer
        pthread_mutex_lock(&mutex);
        // Add the message to the buffer
        sprintf(&msg_buffer[prod_index * 80], "msg #%d", i);
        prod_index = (prod_index + 1) % BUFFER_SIZE;
        // Release the mutex
        pthread_mutex_unlock(&mutex);
        // Signal that a slot is full in the buffer
        sem_post(&full);
        i++;
    }
    free(msg_buffer);
    pthread_exit(NULL);
}

// Define the consumer function
void *consumer(void *arg) {
    int cons_index = 0;
    char *msg_buffer = malloc(sizeof(char) * 80 * NUM_MESSAGES);
    char *array = malloc(sizeof(char) * 80 * NUM_MESSAGES);
    int i = 0;
    while (i < NUM_MESSAGES) {
        // Wait for a full slot in the buffer
        sem_wait(&full);
        // Acquire the mutex to protect the buffer
        pthread_mutex_lock(&mutex);
        // Remove an item from the buffer
        char *msg = &msg_buffer[cons_index * 80];
        cons_index = (cons_index + 1) % BUFFER_SIZE;
        printf("Consumer %d consumed item %s\n", i, msg);
        // Copy the message to the array
        memcpy(&array[i * 80], msg, 80);
        // Release the mutex
        pthread_mutex_unlock(&mutex);
        // Signal that a slot is empty in the buffer
        sem_post(&empty);
        i++;
    }
    printf("Resulting array: %s\n", array);
    free(msg_buffer);
    free(array);
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
    pthread_t p1, p2, p3, p4, p5;
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
            //    1. Create a shared bounded buffer that can hold a fixed number of items. Let's say the buffer can hold up to 5 items.
            //
            //    2. Create two producer threads and three consumer threads. Each producer thread generates a data item and adds it to the buffer, while each consumer thread removes an item from the buffer and processes it.
            //
            //    3. Create a mutex to protect the buffer from concurrent access.
            //
            //    4. Create two semaphores:
            //          a. A semaphore empty initialized to the buffer size (5 in this case) to keep track of the number of empty slots in the buffer.
            //          b. A semaphore full initialized to 0 to keep track of the number of full slots in the buffer.
            //
            //    5. In each producer thread, repeat the following steps:
            //          a. Wait on the empty semaphore to decrement the number of empty slots in the buffer.
            //          b. Acquire the mutex to protect the buffer.
            //          c. Generate a data item and add it to the buffer.
            //          d. Release the mutex to allow other threads to access the buffer.
            //          e. Signal the full semaphore to increment the number of full slots in the buffer.
            //          f. Repeat the above steps indefinitely.
            //
            //    6. In each consumer thread, repeat the following steps:
            //          a. Wait on the full semaphore to decrement the number of full slots in the buffer.
            //          b. Acquire the mutex to protect the buffer.
            //          c. Remove an item from the buffer and process it.
            //          d. Release the mutex to allow other threads to access the buffer.
            //          e. Signal the empty semaphore to increment the number of empty slots in the buffer.
            //          f. Repeat the above steps indefinitely.

            //create more pthreads 3, 4,and 5
            //create a shared bounded buffer that can hold a fixed number of items
            //create a mutex to protect the buffer from concurrent access
            /*pthread_create(&p1, NULL, promtAndSend, NULL);
            pthread_create(&p2, NULL, recvMsgs, NULL);
            pthread_create(&p3, NULL, passiton, (void *)1);
            pthread_create(&p4, NULL, passiton, (void *)2);
            pthread_create(&p5, NULL, passiton, (void *)3);
            pthread_join(p1, NULL);
            pthread_join(p2, NULL);
            pthread_join(p3, NULL);
            pthread_join(p4, NULL);
            pthread_join(p5, NULL);*/

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