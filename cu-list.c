#include "cu-list.h"
#include "cu-memory.h"
#include "cu.h"

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

CUList *cu_list_append(CUList *list, void *data)
{
    CUList *entry = cu_alloc(sizeof(CUList));
    entry->data = data;
    entry->next = NULL;

    CUList *tmp;

    if (list) {
        tmp = list;
        while (tmp->next) {
            tmp = tmp->next;
        }
        tmp->next = entry;
        entry->prev = tmp;

        return list;
    }
    else {
        entry->prev = NULL;
        return entry;
    }
}

CUList *cu_list_insert_after(CUList *list, CUList *llink, void *data)
{
    CUList *entry = cu_alloc(sizeof(CUList));
    entry->data = data;
    entry->prev = llink;

    if (llink) {
        entry->next = llink->next;
        llink->next = entry;
        if (entry->next)
            entry->next->prev = entry;
        return list;
    }
    else {
        entry->next = list;
        if (list)
            list->prev = entry;
        return entry;
    }
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

/* Get the last element in the list. */
CUList *cu_list_last(CUList *list)
{
    CUList *last = list;
    while (cu_list_next(last)) {
        last = last->next;
    }
    return last;
}

/* Just for symmetry, get the first element of the list. */
CUList *cu_list_first(CUList *list)
{
    return list;
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

CUList *cu_list_remove(CUList *list, void *data)
{
    CUList *tmp;
    for (tmp = list; tmp; tmp = tmp->next) {
        if (tmp->data == data)
            return cu_list_delete_link(list, tmp);
    }
    return list;
}

CUList *cu_list_find_custom(CUList *list, void *data, CUCompareFunc compare)
{
    CUList *tmp;
    for (tmp = list; tmp; tmp = tmp->next) {
        if (compare(tmp->data, data) == 0)
            return tmp;
    }

    return NULL;
}

CUList *cu_list_remove_custom(CUList *list, void *data, CUCompareFunc compare)
{
    CUList *llink = cu_list_find_custom(list, data, compare);
    return cu_list_delete_link(list, llink);
}

/* Call func for each element in the list. Stop if func returns false. */
void cu_list_foreach(CUList *list, CUForeachFunc func, void *userdata)
{
    if (cu_unlikely(!func))
        return;
    for ( ; list; list = list->next) {
        if (!func(list->data, userdata))
            return;
    }
}
