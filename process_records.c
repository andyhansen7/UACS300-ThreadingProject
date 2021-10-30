#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "report_record_formats.h"
#include "queue_ids.h"

#define bool int
#define true 1
#define false 0
#define SIGINT  2

// Protected variables
int numReports = 1;
int* responsesPerID;
int numRecords = 0;
bool interrupted = false;

// Mutexes
pthread_mutex_t responsesPerIDMutex;
pthread_mutex_t numReportsMutex;
pthread_mutex_t numRecordsMutex;
pthread_mutex_t interruptedMutex;

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

void handleInterrupt(int signal)
{
    processRecordsError("Ctrl+C received");
    pthread_mutex_lock(&interruptedMutex);
    interrupted = true;
    pthread_mutex_unlock(&interruptedMutex);
};

void* getReportRecordBuf()
{
    return malloc(sizeof(report_record_buf));
}

void* getReportRequestBuf()
{
    return malloc(sizeof(report_request_buf));
}

void sendMessage(report_record_buf* record, size_t bufferLength)
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;

    key = ftok(FILE_IN_HOME_DIR,1);
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
    else
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

    // Send message
    if((msgsnd(msqid, record, bufferLength, IPC_NOWAIT)) < 0) {
        int errnum = errno;
        fprintf(stderr,"%d, %ld, %s, %d\n", msqid, record->mtype, record->record, (int)bufferLength);
        perror("(msgsnd)");
        fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
        exit(1);
    }
    else
        fprintf(stderr,"msgsnd-report_record: record\"%s\" Sent (%d bytes)\n", record->record,(int)bufferLength);
}

report_request_buf* getMessage()
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    report_request_buf* buf = getReportRequestBuf();
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
    //fprintf(stderr,"msgrcv error return code --%d:$d--",ret,errno);

    fprintf(stderr,"process-msgrcv-request: msg type-%ld, Record %d of %d: %s ret/bytes rcv'd=%d\n", buf->mtype, buf->report_idx, buf->report_count, buf->search_string, ret);

    return buf;
}

int main(int argc, char**argv)
{
    // Local variables
    // Queue-related
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;

    // File reading
    struct reportrecordbuf* records;
    char line[RECORD_FIELD_LENGTH+1];

    // Main routine
    bool firstRequestRecieved = false;
    int messagesRecieved = 0;

    // Signal handling
    signal(SIGINT, handleInterrupt);

    // Initialize mutexes
    int init_ret0 = pthread_mutex_init(&responsesPerIDMutex, NULL);
    int init_ret1 = pthread_mutex_init(&numReportsMutex, NULL);
    int init_ret2 = pthread_mutex_init(&numRecordsMutex, NULL);
    int init_ret3 = pthread_mutex_init(&interruptedMutex, NULL);

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
        // Get message from queue
        report_request_buf* response = getReportRequestBuf();

        messagesRecieved++;

        // Set number of records if not already set
        if(firstRequestRecieved == false)
        {
            processRecordsError("First request received");

            pthread_mutex_lock(&numReportsMutex);
            numReports = response->report_count;
            pthread_mutex_unlock(&numReportsMutex);

            firstRequestRecieved = true;

            for(int i = 0; i < numReports; i++)
            {
                responsesPerID[i] = 0;
            }
        }

        // Load variables from message
        int id = response->report_idx;
        char search[11];
        strcpy(response->search_string, search);

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
                report_record_buf* queryResult = getReportRecordBuf();
                queryResult->mtype = 1;
                strcpy(queryResult->record, currentReport.record);
                sendMessage(queryResult, (strlen(queryResult->record) + sizeof(int) + 1));
            }
        }

        // Send message with no payload to let Java program know to complete
        report_record_buf* nullBuf = getReportRecordBuf();
        nullBuf->mtype = 1;
        strcpy(nullBuf->record, "");
        sendMessage(nullBuf, (strlen(nullBuf->record) + sizeof(int) + 1));

        // Escape loop if complete
        pthread_mutex_lock(&numReportsMutex);
        if(messagesRecieved != numReports)
        {
            pthread_mutex_unlock(&numReportsMutex);
            break;
        }
        pthread_mutex_unlock(&numReportsMutex);

        // Escape loop if Ctrl+C
        pthread_mutex_lock(&interruptedMutex);
        if(interrupted == true)
        {
            pthread_mutex_unlock(&interruptedMutex);
            break;
        }
        pthread_mutex_unlock(&interruptedMutex);
    }
    // endregion

    exit(0);
}
