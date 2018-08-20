#include "cu-list.h"
#include "cu-memory.h"

CUList *cu_list_prepend(CUList *list, void *data)
{
    CUList *entry = cu_alloc(sizeof(CUList));
    entry->data = data;
    entry->next = list;
    entry->prev = NULL;
    if (list)
        list->prev = entry;
    return entry;
}

CUList *cu_list_reverse(CUList *list)
{
    CUList *tmp;
    CUList *new_head = list;
    while (list) {
        new_head = list;
        tmp = list->next;
        list->next = list->prev;
        list->prev = tmp;
        list = list->prev;
    }
    return new_head;
}

void cu_list_free_full(CUList *list, CUDestroyNotifyFunc notify)
{
    CUList *tmp;
    while (list) {
        tmp = list->next;
        if (notify)
            notify(list->data);
        cu_free(list);
        list = tmp;
    }
}

CUList *cu_list_delete_link(CUList *list, CUList *link)
{
    if (!link || !list)
        return list;

    if (link->prev)
        link->prev->next = link->next;
    else
        list = link->next;
    if (link->next)
        link->next->prev = link->prev;

    cu_free(link);

    return list;
}
