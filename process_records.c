#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "report_record_formats.h"
#include "queue_ids.h"

#define bool int
#define true 1
#define false 0

void processRecordsError(char* output)
{
    fprintf(stderr, "ProcessRecords.c: ");
    fprintf(stderr, "%s", output);
    fprintf(stderr, "\n");
}

void printStatusReport(int numRecords, int numReports, int* recordsSentPerReport)
{
    fprintf(stdout, "***REPORT***\n");
    fprintf(stdout, "%d records read for %d reports\n", numRecords, numReports);

    for(int i = 0; i < numReports; i++)
    {
        fprintf(stdout, "Records sent for report index %d: %d\n", i, recordsSentPerReport[i]);
    }
};

int main(int argc, char**argv)
{
    // Local variables
    // Queue-related
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;

    // Send / Receive
    report_request_buf rbuf;
    report_record_buf sbuf;
    size_t buf_length;

    // File reading
    struct reportrecordbuf* records;
    int numRecords = 0;
    char line[RECORD_FIELD_LENGTH+1];
    
    // Main routine
    bool firstRequestRecieved = false;
    int numReports = 1;
    int messagesRecieved = 0;
    int* responsesPerID;

    // Mutexes
    pthread_mutex_t responsesPerIDMutex;
    pthread_mutex_t numReportsMutex;
    pthread_mutex_t numRecordsMutex;

    // Initialize mutexes
    int init_ret0 = pthread_mutex_init(&responsesPerIDMutex, NULL);
    int init_ret1 = pthread_mutex_init(&numReportsMutex, NULL);
    int init_ret2 = pthread_mutex_init(&numRecordsMutex, NULL);

    // region // Provided Setup
    // Sanity check
    key = ftok(FILE_IN_HOME_DIR, QUEUE_NUMBER);
    if(key == 0xffffffff) {
        fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
        return 1;
    }

    // Error handling
    if((msqid = msgget(key, msgflg)) < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }
    else
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);
    // endregion

    //region // Read from stdin
    while(fgets(line, sizeof line, stdin))
    {
        // Check length is within bounds
        size_t eol = strcspn(line, "\n");
        line[eol] = '\0';
        if(eol >= RECORD_FIELD_LENGTH)
        {
            processRecordsError("Line too long!");
            break;
        }

        // Add line to dictionary
        struct reportrecordbuf newRecord;
        newRecord.mtype = 1;
        strcpy(line, newRecord.record);

        pthread_mutex_lock(&numRecordsMutex);
        numRecords++;
        pthread_mutex_unlock(&numRecordsMutex);
    }
    // endregion

    //region // Create thread
    pthread_t statusThread;
//    pthread_create(&statusThread, NULL, , NULL);

    //region // Main procedure
    while(true)
    {
        // Receive message
        int ret;
        do
        {
            ret = msgrcv(msqid, &rbuf, sizeof(report_request_buf), 1, 0); //receive type 1 message
            int errnum = errno;
            if (ret < 0 && errno !=EINTR)
            {
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
                fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
            }
        } while((ret < 0 ) && (errno == 4));
        messagesRecieved++;

        // Set number of records if not already set
        if(firstRequestRecieved == false)
        {
            processRecordsError("First request received");

            pthread_mutex_lock(&numReportsMutex);
            numReports = rbuf.report_count;
            pthread_mutex_unlock(&numReportsMutex);

            firstRequestRecieved = true;

            for(int i = 0; i < numReports; i++)
            {
                responsesPerID[i] = 0;
            }
        }

        // Load variables from message
        int id = rbuf.report_idx;
        char search[11];
        strcpy(rbuf.search_string, search);

        // Search for string in list read
        for(int i = 0; i < numRecords; i++)
        {
            // Load struct
            const struct reportrecordbuf currentReport = records[i];

            // If contains search, send it
            msqid = id;
            if(strstr(currentReport.record, search))
            {
                // Increment number of responses
                responsesPerID[id]++;

                // Send report on correct queue
                sbuf.mtype = 1;
                strcpy(sbuf.record,currentReport.record);
                buf_length = strlen(sbuf.record) + sizeof(int) + 1;//struct size without mtype

                // Send a message.
                if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {
                    int errnum = errno;
                    fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.record, (int)buf_length);
                    perror("(msgsnd)");
                    fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
                    exit(1);
                }
                else
                    fprintf(stderr,"msgsnd-report_record: record\"%s\" Sent (%d bytes)\n", sbuf.record,(int)buf_length);
            }
        }

        // Send report on correct queue
        sbuf.mtype = 1;
        strcpy(sbuf.record,"");
        buf_length = strlen(sbuf.record) + sizeof(int) + 1;//struct size without mtype

        // Send message with no payload to let Java program know to complete
        if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {
            int errnum = errno;
            fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.record, (int)buf_length);
            perror("(msgsnd)");
            fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
            exit(1);
        }
        else
            fprintf(stderr,"msgsnd-report_record: record\"%s\" Sent (%d bytes)\n", sbuf.record,(int)buf_length);

        // Escape loop if complete
        pthread_mutex_lock(&numReportsMutex);
        if(messagesRecieved != numReports)
        {
            pthread_mutex_unlock(&numReportsMutex);
            break;
        }
        pthread_mutex_unlock(&numReportsMutex);
    }
    // endregion

    exit(0);
}
