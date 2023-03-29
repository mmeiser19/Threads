#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "msgq.h"

/*
 * Struct msgq is a message queue consisting the current number of messages, the maximum number of messages, an array
 * to hold said messages, and a lock to protect the message queue
 */
struct msgq {
    int num_msgs; // Current number of messages
    int max_msgs; // Maximum number of messages
    char **messages; // Array of messages
    pthread_mutex_t lock;
};

/*
 * Initializes a message queue and returns a pointer to a struct msgq. The parameter num_msgs is the maximum number
 * of messages that may be in the message queue. The returned pointer is used in the other API functions
 */
struct msgq *msgq_init(int num_msgs) {
    struct msgq *mq = malloc(sizeof(struct msgq));
    mq->num_msgs = 0; // Initializes an empty message queue
    mq->max_msgs = num_msgs; // Sets the maximum number of messages to the number of current messages in the queue
    return mq;
}

/*
 * Places msg on message queue mq. mq is returned from msgq_init. msgq_send must make a copy of msg on the heap.
 * If mq has num_msgs in it; then msgq_send blocks until there is room for msg. msgq_send returns 1 for success and
 * -1 for failure
 */
int msgq_send(struct msgq *mq, char *msg) {
    pthread_mutex_lock(&mq->lock);
    char *msg_copy = strdup(msg);
    if (mq->num_msgs == 0) { // If there are no messages in the queue, add the message
        mq->messages = malloc(sizeof(char*));
        mq->messages[0] = msg_copy;
        mq->num_msgs++;
    }
    else if (mq->num_msgs == mq->max_msgs) { // If the queue is full, print error message
        printf("Message queue is full\n");
        pthread_mutex_unlock(&mq->lock);
        return -1; // Msg was not sent
    }
    else { // If there are messages in the queue but th queue is not full, add the message
        mq->messages = realloc(mq->messages, sizeof(char*) * (mq->num_msgs + 1));
        mq->messages[mq->num_msgs] = msg_copy;
        mq->num_msgs++;
        if (mq->num_msgs == mq->max_msgs) {
            printf("\n");
            printf("Message queue is now full\n");
            printf("Blocking msgq_send until a message is received\n");
            printf("\n");
        }
    }
    // Check if msg was sent
    for (int i = 0; i < mq->max_msgs; i++) {
        if (mq->messages[i] == msg_copy) {
            pthread_mutex_unlock(&mq->lock);
            return 1; // Msg was sent
        }
    }
    printf("Message was not sent\n");
    pthread_mutex_unlock(&mq->lock);
    return -1; // Msg was not sent
}

/*
 * Returns a message from mq. mq is returned from msgq_init. The returned message is on the heap. The function that
 * receives the message can free it when it is no longer needed
 */
char *msgq_recv(struct msgq *mq) {
    if (mq->num_msgs == 0) { // If there are no messages in the queue, print error message
        printf("No messages to receive\n");
        return NULL;
    }
    else { // If there are messages in the queue, return the first message
        pthread_mutex_lock(&mq->lock);
        char *msg = mq->messages[0];
        for (int i = 0; i < mq->num_msgs; i++) { // Shift all messages down
            mq->messages[i] = mq->messages[i+1];
        }
        mq->num_msgs--;
        pthread_mutex_unlock(&mq->lock);
        return msg;
    }
}

/*
 * Returns the number of messages on mq. mq is returned from msgq_init
 */
int msgq_len(struct msgq *mq) {
    pthread_mutex_lock(&mq->lock);
    int len = mq->num_msgs;
    pthread_mutex_unlock(&mq->lock);
    return len;
}

/*
 * Displays all the messages in mq to stdout. mq is returned from msgq_init
 */
void msgq_show(struct msgq *mq) {
    pthread_mutex_lock(&mq->lock);
    int num_msgs = mq->num_msgs;
    for (int i = 0; i < num_msgs; i++) {
        printf("message #%d: %s\n", i, mq->messages[i]);
    }
    pthread_mutex_unlock(&mq->lock);
}