#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "msgq.h"

/*
 * You must design and implement struct msgq, which must involve dynamically allocated memory. My solution
 * includes a linked list of messages, where send_msg mallocs memory for the data structure and places it on the
 * end of the linked list. The data structure has a char* pointer to the message. The heap memory for the message
 * is allocated using strdup. recv_msg removes the head from the linked list and returns the char* pointer to the message
 */
struct msgq {
    int num_msgs; //current number of messages
    int max_msgs; //maximum number of messages
    char **messages; //array of messages
    pthread_mutex_t lock;
};

/*
 * initializes a message queue and returns a pointer to a struct msgq. The parameter num_msgs is the maximum number
 * of messages that may be in the message queue. The returned pointer is used in the other API functions
 */
struct msgq *msgq_init(int num_msgs) {
    struct msgq *mq = malloc(sizeof(struct msgq));
    mq->num_msgs = 0;
    mq->max_msgs = num_msgs;
    return mq;
}

/*
 * places msg on message queue mq. mq is returned from msgq_init. msgq_send must make a copy of msg on the heap.
 * If mq has num_msgs in it; then msgq_send blocks until there is room for msg. msgq_send returns 1 for success and
 * -1 for failure
 */
char msgq_send(struct msgq *mq, char *msg) {
    char *msg_copy = strdup(msg);
    if (mq->num_msgs == 0) {
        mq->messages = malloc(sizeof(char*));
        mq->messages[0] = msg_copy;
        mq->num_msgs++;
    }
    else if (mq->num_msgs == mq->max_msgs) {
        return -1; //msg was not sent
    }
    else {
        pthread_mutex_lock(&mq->lock);
        mq->messages = realloc(mq->messages, sizeof(char*) * (mq->num_msgs + 1));
        mq->messages[mq->num_msgs] = msg_copy;
        mq->num_msgs++;
        pthread_mutex_unlock(&mq->lock);
    }
    //check if msg was sent
    for (int i = 0; i < mq->max_msgs; i++) {
        if (mq->messages[i] == msg_copy) {
            return 1; //msg was sent
        }
    }
    return -1; //msg was not sent
}

/*
 * returns a message from mq. mq is returned from msgq_init. The returned message is on the heap. The function that
 * receives the message can free it when it is no longer needed
 */
//does there need to be some sort of yield here?
char *msgq_recv(struct msgq *mq) {
    //return a message from mq
    if (mq->num_msgs == 0) {
        printf("No messages to receive\n");
        return NULL;
    }
    else {
        pthread_mutex_lock(&mq->lock);
        char *msg = mq->messages[0];
        for (int i = 0; i < mq->num_msgs; i++) { //shift all messages down
            mq->messages[i] = mq->messages[i+1];
        }
        mq->num_msgs--;
        pthread_mutex_unlock(&mq->lock);
        return msg;
    }
}

/*
 * returns the number of messages on mq. mq is returned from msgq_init. Due to the nature of threads and interrupts,
 * the length returned may be incorrect by the time it is used
 */
int msgq_len(struct msgq *mq) {
    pthread_mutex_lock(&mq->lock);
    int len = mq->num_msgs;
    pthread_mutex_unlock(&mq->lock);
    return len;
}

/*
 * displays all of the messages in mq to stdout. mq is returned from msgq_init
 */
void msgq_show(struct msgq *mq) {
    pthread_mutex_lock(&mq->lock);
    int num_msgs = mq->num_msgs;
    for (int i = 0; i < num_msgs; i++) {
        printf("message #%d: %s\n", i, mq->messages[i]);
    }
    pthread_mutex_unlock(&mq->lock);
}