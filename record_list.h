//
// Created by andyh on 10/30/21.
//

#ifndef CS300_RECORD_LIST_H
#define CS300_RECORD_LIST_H

typedef struct recordlistnode
{
    struct recordlistnode* next;
    struct recordlistnode* prev;
    report_record_buf* record;
} record_list_node;

record_list_node* getRecordListNode(report_record_buf* record)
{
    record_list_node* node = malloc(sizeof(record_list_node));
    node->record = record;
    node->next = NULL;
    node->prev = NULL;

    return node;
}

#endif //CS300_RECORD_LIST_H
