/*
 * Copyright (c) 1984 Sun Microsystems, Inc.
 */

#ifndef lint
static char sccsid[] = "@(#)lists.c 1.1 94/10/31 SMI Copyr 1984 Sun Micro";
#endif

/*
 * General list definitions.
 *
 * The assumption is that the elements in a list are words,
 * usually pointers to the actual information.
 */

#include "defs.h"
#include "lists.h"

#ifndef public

typedef struct List *List;
typedef struct ListItem *ListItem;
typedef char *ListElement;

#define list_item(element) generic_list_item((ListElement) (element))
#define list_element(type, item) ((type) (item == nil ? nil : (item)->element))
#define list_head(list) ((list == nil) ? nil : (list)->head)
#define list_tail(list) ((list == nil) ? nil : (list)->tail)
#define list_next(item) ((item == nil) ? nil : (item)->next)
#define list_prev(item) ((item == nil) ? nil : (item)->prev)
#define list_size(list) (((list) == nil) ? 0 : (list)->nitems)
#define list_append_tail(item,list) list_append(list_item(item),list_tail(list),list)
#define list_append_head(item,list) list_append(list_item(item),nil,list)

#define foreach(type, i, list) \
{ \
    register ListItem _item; \
 \
    _item = list_head(list); \
    while (_item != nil) { \
	i = list_element(type, _item); \
	_item = list_next(_item);

#define endfor \
    } \
}

/*
 * Iterate through two equal-sized lists.
 */

#define foreach2(type1, i, list1, type2, j, list2) \
{ \
    register ListItem _item1, _item2; \
 \
    _item1 = list_head(list1); \
    _item2 = list_head(list2); \
    while (_item1 != nil) { \
	i = list_element(type1, _item1); \
	j = list_element(type2, _item2); \
	_item1 = list_next(_item1); \
	_item2 = list_next(_item2);

#define list_islast() (_item == nil)
#define list_curitem(list) (_item == nil ? list_tail(list) : list_prev(_item))

/*
 * Representation should not be used outside except through macros.
 */

struct List {
    Integer nitems;
    ListItem head;
    ListItem tail;
};

struct ListItem {
    ListElement element;
    ListItem next;
    ListItem prev;
};

#endif

/*
 * Allocate and initialize a list.
 */

public List list_alloc()
{
    List list;

    list = new(List);
    if (list == 0)
	fatal("No memory available. Out of swap space?");
    list->nitems = 0;
    list->head = nil;
    list->tail = nil;
    return list;
}

/*
 * Create a list item from an object (represented as pointer or integer).
 */

public ListItem generic_list_item(element)
ListElement element;
{
    ListItem i;

    i = new(ListItem);
    if (i == 0)
	fatal("No memory available. Out of swap space?");
    i->element = element;
    i->next = nil;
    i->prev = nil;
    return i;
}

/*
 * Append an item after the given item in a list.
 * if before is nil, then put item at head of list.
 */

public list_append(item, before, list)
ListItem item;
ListItem before;
List list;
{
    ListItem b;

    checkref(list);
    list->nitems = list->nitems + 1;
    if (list->head == nil) {
	list->head = item;
	list->tail = item;
    } else {
	if (before == nil) {
	    item->next = list->head;
	    /* item->prev = nil; */
	    if (list->head != nil)
		list->head->prev = item;
	    list->head = item;
	} else {
	    b = before;
	    item->next = b->next;
	    item->prev = b;
	    if (b->next != nil) {
		b->next->prev = item;
	    } else {
		list->tail = item;
	    }
	    b->next = item;
	}
    }
}

/*
 * Delete an item from a list.
 */

public list_delete(item, list)
ListItem item;
List list;
{
    checkref(item);
    checkref(list);
    assert(list->nitems > 0);
    if (item->next == nil) {
	list->tail = item->prev;
    } else {
	item->next->prev = item->prev;
    }
    if (item->prev == nil) {
	list->head = item->next;
    } else {
	item->prev->next = item->next;
    }
    list->nitems = list->nitems - 1;
}
