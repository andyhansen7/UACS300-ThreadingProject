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

void printStatusReport()
{
    // Lock mutex


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
    int c = 0;
    char line[RECORD_FIELD_LENGTH+1];
    
    // Main routine
    bool firstRequestRecieved = false;
    int numReports = 0;

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
        c++;    // i wish :(
    }
    // endregion

    //region // Create thread
    pthread_t statusThread;
//    pthread_create(&statusThread, NULL, func, NULL);

    //region // Main procedure
    while(true)
    {
        // Receive message
        int ret;
        do
        {
            ret = msgrcv(msqid, &rbuf, sizeof(report_request_buf), 1, 0);//receive type 1 message
            int errnum = errno;
            if (ret < 0 && errno !=EINTR)
            {
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
                fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
            }
        } while((ret < 0 ) && (errno == 4));

        // Load variables from message
        int id = rbuf.report_idx;
        char search[11];
        strcpy(rbuf.search_string, search);

        // Set number of records if not already set
        if(firstRequestRecieved == false)
        {
            numReports = rbuf.report_count;
            firstRequestRecieved = true;
        }

        // Search for string in list read
        for(int i = 0; i < c; i++)
        {
            // Load struct
            const struct reportrecordbuf currentReport = records[i];

            // If contains search, send it
            if(strstr(currentReport.record, search))
            {
                // Send report on correct queue
                
            }
        }
    }

    exit(0);
}
