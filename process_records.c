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
#include "message_utils.h"

// I like C++ better
#define bool int
#define true 1
#define false 0
#define SIGINT  2

// Linked list type
typedef struct recordlistnode
{
    struct recordlistnode* next;
    struct recordlistnode* prev;
    report_record_buf* record;
} record_list_node;

// Protected variables
int numReports = 1;
int numRecords = 0;
record_list_node* head = NULL;
record_list_node* tail = NULL;
bool interrupted = false;
int* responsesPerQueue;

// Mutexes
pthread_mutex_t recordListMutex;
pthread_mutex_t numReportsMutex;
pthread_mutex_t numRecordsMutex;
pthread_mutex_t interruptedMutex;
pthread_mutex_t responsesPerQueueMutex;

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

void processRecordsError(char* output)
{
    fprintf(stderr, "ProcessRecords.c: ");
    fprintf(stderr, "%s", output);
    fprintf(stderr, "\n");
}

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
    pthread_mutex_lock(&interruptedMutex);
        interrupted = true;
    pthread_mutex_unlock(&interruptedMutex);

};

void *statusPrintingThread(void* arg)
{
    while(interrupted == false) {}
    printStatusReport();
}

int main(int argc, char**argv)
{
    // Local variables
    char line[RECORD_FIELD_LENGTH+1];

    // Main routine
    bool firstRequestRecieved = false;
    int messagesRecieved = 0;

    // Signal handling
    signal(SIGINT, handleInterrupt);

    // Initialize mutexes
    int init_ret0 = pthread_mutex_init(&recordListMutex, NULL);
    int init_ret1 = pthread_mutex_init(&numReportsMutex, NULL);
    int init_ret2 = pthread_mutex_init(&numRecordsMutex, NULL);
    int init_ret3 = pthread_mutex_init(&interruptedMutex, NULL);

    //region // Read from stdin
    while(fgets(line, sizeof line, stdin))
    {
        // Check length is within bounds
        size_t eol = strcspn(line, "\n");
        fprintf(stderr, "loaded record line: %s\n", line);
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
        fprintf(stderr, "new record is: %s\n", newRecord->record);
        
        record_list_node* newNode = malloc(sizeof(record_list_node));
        newNode->record = newRecord;
        newNode->next = NULL;
        newNode->prev = NULL;

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
        
        pthread_mutex_lock(&numRecordsMutex);
            numRecords++;
        pthread_mutex_unlock(&numRecordsMutex);
    }
    // endregion

    //region // Create thread
    pthread_t statusThread;
    pthread_create(&statusThread, NULL, statusPrintingThread, NULL);

    //region // Main procedure
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
                fprintf(stderr,"Response: %s\n", queryResult->record);
                fprintf(stderr, "Queue ID: %d\n", response->report_idx);
                fprintf(stderr, "Total responses to queue: %d\n", getResponses(response->report_idx));

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
            processRecordsError("exiting after completing all requests");
            
            pthread_mutex_lock(&interruptedMutex);
                interrupted = true;
            pthread_mutex_unlock(&interruptedMutex);

            break;
        }
        pthread_mutex_unlock(&numReportsMutex);

        // Escape loop if Ctrl+C
        pthread_mutex_lock(&interruptedMutex);
        if(interrupted == true)
        {
            pthread_mutex_unlock(&interruptedMutex);
            processRecordsError("exiting from Ctrl+C");
            break;
        }
        pthread_mutex_unlock(&interruptedMutex);
    }
    // endregion

    pthread_join(statusThread, NULL);
    exit(0);
}
