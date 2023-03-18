#ifndef THREADS_MSGQ_H
#define THREADS_MSGQ_H

struct msgq *msgq_init(int num_msgs);
char msgq_send(struct msgq *mq, char *msg);
char *msgq_recv(struct msgq *mq);
int msgq_len(struct msgq *mq);

#endif //THREADS_MSGQ_H
