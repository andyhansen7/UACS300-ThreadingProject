/** CS300 Threading Project - process_records.c
 *  Andy Hansen, arhansen@crimson.ua.edu
 *  A C application to respond to queries from another application
 *  using the System V queue. Data is loaded from stdin and responses
 *  are sent on queues provided in the messages recieved
 */

// STL libraries
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// Headers
#include "report_record_formats.h"
#include "queue_ids.h"
#include "message_utils.h"

// I like C++ better
#define bool int
#define true 1
#define false 0
#define SIGINT  2

typedef struct requestentry
{
    report_request_buf* request;
    int response_count;
} request_entry;

//region Global Variables
// Protected variables
int numReports = 0;
int numRecords = 0;
request_entry** requests;

// Mutexes
pthread_mutex_t requestListMutex;
pthread_mutex_t numReportsMutex;
pthread_mutex_t numRecordsMutex;
pthread_mutex_t interruptMutex;

// Printing thread
pthread_t statusThread;
pthread_cond_t interrupt;
//endregion

void printStatusReport()
{
    // load values
    int numRec, numRep;
    pthread_mutex_lock(&numRecordsMutex);
        numRec = numRecords;
    pthread_mutex_unlock(&numRecordsMutex);
    pthread_mutex_lock(&numReportsMutex);
        numRep = numReports;
    pthread_mutex_unlock(&numReportsMutex);

    fprintf(stdout, "***REPORT***\n");
    fprintf(stdout, "%d records read for %d reports\n", numRec, numRep);

    pthread_mutex_lock(&requestListMutex);
    for(int i = 0; i < numRep; i++)
    {
        fprintf(stdout, "Records sent for report index %d: %d\n", i, requests[i]->response_count);
    }
    pthread_mutex_unlock(&requestListMutex);
};

void handleInterrupt(int signal)
{
    pthread_mutex_unlock(&interruptMutex);
    pthread_cond_signal(&interrupt);
};

void *statusPrintingThread(void* arg)
{
    pthread_mutex_lock(&interruptMutex);
        pthread_cond_wait(&interrupt, &interruptMutex);
    pthread_mutex_unlock(&interruptMutex);
    printStatusReport();
}

int main(int argc, char**argv)
{
    //region Local Variables
    // Local variables
    char line[RECORD_FIELD_LENGTH+1];

    // Main routine
    bool firstRequestRecieved = false;
    int messagesReceived = 0;
    int recordsLoaded = 0;

    // Signal handling
    signal(SIGINT, handleInterrupt);

    // Thread
    pthread_create(&statusThread, NULL, statusPrintingThread, NULL);

    // Initialize mutexes
    pthread_mutex_init(&requestListMutex, NULL);
    pthread_mutex_init(&numReportsMutex, NULL);
    pthread_mutex_init(&numRecordsMutex, NULL);
    pthread_mutex_init(&interruptMutex, NULL);
    pthread_cond_init(&interrupt, NULL);
    //endregion

    //region Load requests
    while(true)
    {
        // Get message from queue
        report_request_buf* response = getMessage();
        fprintf(stderr, "Received response with query: %s, queue: %d\n", response->search_string, response->report_idx);

        // Allocate array if not allocated
        if(messagesReceived == 0)
        {
            fprintf(stderr, "First request received\n");
            pthread_mutex_lock(&numReportsMutex);
                numReports = response->report_count;
            pthread_mutex_unlock(&numReportsMutex);
            requests = malloc(sizeof(request_entry*) * response->report_count);
        }

        // Create entry
        request_entry* newEntry = malloc(sizeof(request_entry));
        newEntry->request = response;
        newEntry->response_count = 0;

        // Add entry to list
        pthread_mutex_lock(&requestListMutex);
            requests[messagesReceived] = newEntry;
        pthread_mutex_unlock(&requestListMutex);

        // Increment responses
        messagesReceived++;

        // Break if done
        if(messagesReceived == response->report_count)
        {
            fprintf(stderr, "All requests received, sending replies\n");
            break;
        }
    }
    // endregion

    //region Load and return reports
    fprintf(stderr, "\n\nLoading records from stdin...\n");

    int numRep = 0;
    pthread_mutex_lock(&numReportsMutex);
        numRep = numReports;
    pthread_mutex_unlock(&numReportsMutex);

    while(fgets(line, sizeof line, stdin))
    {
        // Increment counter
        pthread_mutex_lock(&numRecordsMutex);
            numRecords++;
        pthread_mutex_unlock(&numRecordsMutex);

        // Check length is within bounds
        size_t eol = strcspn(line, "\n");
        fprintf(stderr, "system loaded record: %.24s ...\n", line);
        line[eol] = '\0';
        if(eol >= RECORD_FIELD_LENGTH)
        {
            fprintf(stderr, "read line too long: %s\n", line);
            break;
        }
        else if(strlen(line) < 2)
        {
            fprintf(stderr, "Exiting after completing all requests!\n");

            break;
        }

        // Send line to appropriate threads
        for(int i = 0; i < numRep; i++)
        {
            request_entry* currentRequest;
            pthread_mutex_lock(&requestListMutex);
            currentRequest = requests[i];
            fprintf(stderr, "current search is: %s\n", currentRequest->request->search_string);

            if(strstr(line, currentRequest->request->search_string) != NULL)
            {
                // increment counter for queue
                currentRequest->response_count++;

                // Send report on correct queue
                report_record_buf* queryResult = malloc(sizeof(report_record_buf));
                queryResult->mtype = 2;
                strcpy(queryResult->record, line);

                fprintf(stderr, "Search string: %s\n", currentRequest->request->search_string);
                fprintf(stderr, "Response: %.24s ...\n", line);
                fprintf(stderr, "Queue ID: %d\n", currentRequest->request->report_idx);
                fprintf(stderr, "Total responses to queue: %d\n\n", currentRequest->response_count);

                sendMessage(queryResult, (strlen(queryResult->record) + sizeof(int) + 1), currentRequest->request->report_idx);
            }
            pthread_mutex_unlock(&requestListMutex);    // Fix this later
        }

        // Sleep for 5 seconds after 10 records are read
        if(recordsLoaded == 10)
        {
            sleep(5);
        }
    }

    //endregion

    //region Send null terminators to threads
    fprintf(stderr, "\ndone searching, sending null responses to threads\n");
    report_record_buf* nullBuf = malloc(sizeof(report_record_buf));
    nullBuf->mtype = 2;
    strcpy(nullBuf->record, "");

    for(int i = 0; i < numRep; i++)
    {
        pthread_mutex_lock(&requestListMutex);
            report_request_buf* request = requests[i]->request;
            sendMessage(nullBuf, (strlen(nullBuf->record) + sizeof(int) + 1), request->report_idx);
        pthread_mutex_unlock(&requestListMutex);
    }
    fprintf(stderr, "done sending null responses!\n");
    //endregion

    // join status thread and exit
    pthread_mutex_unlock(&interruptMutex);
    pthread_cond_signal(&interrupt);

    pthread_join(statusThread, NULL);
    exit(0);
}
