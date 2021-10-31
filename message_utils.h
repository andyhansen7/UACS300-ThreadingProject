//
// Created by andyh on 10/30/21.
//

#ifndef CS300_MESSAGE_UTILS_H
#define CS300_MESSAGE_UTILS_H

#include "report_record_formats.h"

void sendMessage(report_record_buf* record, size_t bufferLength, int queueID)
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;

    key = ftok(FILE_IN_HOME_DIR, queueID);
    if(key == 0xffffffff)
    {
        fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
        return;
    }
    if((msqid = msgget(key, msgflg)) < 0)
    {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }

    // Send message
    if((msgsnd(msqid, record, bufferLength, IPC_NOWAIT)) < 0) {
        int errnum = errno;
        fprintf(stderr,"%d, %ld, %s, %d\n", msqid, record->mtype, record->record, (int)bufferLength);
        perror("(msgsnd)");
        fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
        exit(1);
    }
}

report_request_buf* getMessage()
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    report_request_buf* buf = malloc(sizeof(report_request_buf));
    size_t buf_length;

    key = ftok(FILE_IN_HOME_DIR,QUEUE_NUMBER);
    if (key == 0xffffffff) {
        fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
        return NULL;
    }

    if ((msqid = msgget(key, msgflg)) < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }
    else
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

    // Recieve message
    int ret;
    do {
        ret = msgrcv(msqid, buf, sizeof(report_request_buf), 1, 0);
        int errnum = errno;
        if (ret < 0 && errno !=EINTR){
            fprintf(stderr, "Value of errno: %d\n", errno);
            perror("Error printed by perror");
            fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
        }
    } while ((ret < 0 ) && (errno == 4));

    return buf;
}

#endif //CS300_MESSAGE_UTILS_H
