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
#include "record_list.h"

// I like C++ better
#define bool int
#define true 1
#define false 0
#define SIGINT  2

//region Global Variables
// Protected variables
int numReports = 1;
int numRecords = 0;
record_list_node* head = NULL;
record_list_node* tail = NULL;
//bool interrupted = false;
int* responsesPerQueue;

// Mutexes
pthread_mutex_t recordListMutex;
pthread_mutex_t numReportsMutex;
pthread_mutex_t numRecordsMutex;
pthread_mutex_t interruptMutex;
pthread_mutex_t responsesPerQueueMutex;

// Thread
pthread_t statusThread;
pthread_cond_t interrupt;
//endregion

//region ResponsesPerQueue helper functions
int getResponses(int queue)
{
    int ret;
    pthread_mutex_lock(&responsesPerQueueMutex);
        ret = responsesPerQueue[queue];
    pthread_mutex_unlock(&responsesPerQueueMutex);
    return ret;
}

void incResponses(int queue)
{
    pthread_mutex_lock(&responsesPerQueueMutex);
        responsesPerQueue[queue]++;
    pthread_mutex_unlock(&responsesPerQueueMutex);
}
// endregion

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


    record_list_node* currentNode;
    pthread_mutex_lock(&recordListMutex);
        currentNode = head;
    pthread_mutex_unlock(&recordListMutex);
    for(int i = 0; i < numRep; i++)
    {
        fprintf(stdout, "Records sent for report index %d: %d\n", i, getResponses(i));
    }
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
    int messagesRecieved = 0;

    // Signal handling
    signal(SIGINT, handleInterrupt);

    // Initialize mutexes
    pthread_mutex_init(&recordListMutex, NULL);
    pthread_mutex_init(&numReportsMutex, NULL);
    pthread_mutex_init(&numRecordsMutex, NULL);
    pthread_mutex_init(&interruptMutex, NULL);
    pthread_cond_init(&interrupt, NULL);
    //endregion

    //region Read from stdin
    while(fgets(line, sizeof line, stdin))
    {
        // Check length is within bounds
        size_t eol = strcspn(line, "\n");
        fprintf(stderr, "system loaded record: %.24s ...\n", line);
        line[eol] = '\0';
        if(eol >= RECORD_FIELD_LENGTH)
        {
            //processRecordsError("line too long!");
            fprintf(stderr, "line too long: %s\n", line);
            break;
        }
        else if(strlen(line) < 2) break;

        // Add line to dictionary
        report_record_buf* newRecord = malloc(sizeof(report_record_buf));
        newRecord->mtype = 2;
        strcpy(newRecord->record, line);

        record_list_node* newNode = getRecordListNode(newRecord);

        pthread_mutex_lock(&recordListMutex);
            // List is empty
            if(head == NULL)
            {
                head = newNode;
            }
            else
            {
                // head is only node
                if(tail == NULL)
                {
                    tail = newNode;
                    head->next = newNode;
                    newNode->prev = head;
                }

                // average case
                else
                {
                    newNode->prev = tail;
                    tail->next = newNode;
                    tail = newNode;
                }
            }
        pthread_mutex_unlock(&recordListMutex);

        int currentRecordCount = 0;
        pthread_mutex_lock(&numRecordsMutex);
            numRecords++;
            currentRecordCount = numRecords;
        pthread_mutex_unlock(&numRecordsMutex);

        // Sleep for 5 seconds after 10 records are read
        if(currentRecordCount == 10)
        {
            sleep(5);
        }
    }
    // endregion

    //region Status thread
    pthread_create(&statusThread, NULL, statusPrintingThread, NULL);
    //endregion

    //region Main procedure
    while(true)
    {
        // Get message from queue
        report_request_buf* response = getMessage();
        fprintf(stderr, "received response with query: %s\n", response->search_string);
        messagesRecieved++;

        // Set number of records if not already set
        if(firstRequestRecieved == false)
        {
            pthread_mutex_lock(&numReportsMutex);
                numReports = response->report_count;
            pthread_mutex_unlock(&numReportsMutex);

            firstRequestRecieved = true;

            responsesPerQueue = malloc(sizeof(int) * response->report_count);
            for(int i = 0; i < response->report_count; i++) responsesPerQueue[i] = 0;
        }

        // Load variables from message
        char search[11];
        strcpy(search, response->search_string);

        // Search for string in list read
        pthread_mutex_lock(&recordListMutex);
            record_list_node* currentNode = head;
        pthread_mutex_unlock(&recordListMutex);

        while(currentNode != NULL)
        {
            // Load struct
            report_record_buf* currentRecord = currentNode->record;

            // If contains search, send it
            if(strstr(currentRecord->record, search) != NULL)
            {
                // increment counter for queue
                incResponses(response->report_idx - 1);

                // Send report on correct queue
                report_record_buf* queryResult = malloc(sizeof(report_record_buf));
                queryResult->mtype = 2;
                strcpy(queryResult->record, currentRecord->record);

                fprintf(stderr, "Search string: %s\n", search);
                fprintf(stderr,"Response: %.24s ...\n", queryResult->record);
                fprintf(stderr, "Queue ID: %d\n", response->report_idx);
                fprintf(stderr, "Total responses to queue: %d\n\n", getResponses(response->report_idx - 1));

                sendMessage(queryResult, (strlen(queryResult->record) + sizeof(int) + 1), response->report_idx);
            }

            // Update current node
            pthread_mutex_lock(&recordListMutex);
                currentNode = currentNode->next;
            pthread_mutex_unlock(&recordListMutex);
        }

        // Send message with no payload to let Java program know to complete
        report_record_buf* nullBuf = malloc(sizeof(report_record_buf));
        nullBuf->mtype = 2;
        strcpy(nullBuf->record, "");
        sendMessage(nullBuf, (strlen(nullBuf->record) + sizeof(int) + 1), response->report_idx);

        // Escape loop if complete
        pthread_mutex_lock(&numReportsMutex);
        if(messagesRecieved == numReports)
        {
            pthread_mutex_unlock(&numReportsMutex);
            fprintf(stderr, "Exiting after completing all requests!\n");

            // Signal interrupt
            pthread_mutex_unlock(&interruptMutex);
            pthread_cond_signal(&interrupt);

            break;
        }
        pthread_mutex_unlock(&numReportsMutex);
    }
    // endregion

    // join status thread and exit
    pthread_join(statusThread, NULL);
    exit(0);
}
