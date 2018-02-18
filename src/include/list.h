/* author: Kexuan Zou
   date: 11/25/2017
   external source: http://elixir.free-electrons.com/linux/v2.6.22.5/source/include/linux/list.h
*/

#ifndef _LIST_H
#define _LIST_H

/* pointers that result in page faults. used to mark uninitialized list entries */
#define LIST_POISON1 ((void* )0x00100100)
#define LIST_POISON2 ((void* )0x00200200)

typedef struct list_head {
    struct list_head *next, *prev;
} list_head;

#define LIST_HEAD(name) \
    struct list_head (name) = {&(name), &(name)}

#define PTR_OFFSET(type, name) \
    ((uint32_t) &((type* )0)->name)

/* LIST_ENTRY - get the struct for the embeded list_head
   @ptr - the struct's list_head pointer
   @type - type of the struct list_head is embeded in
   @name - name of the lsit_head struct */
#define LIST_ENTRY(ptr, type, name) ({                  \
    const typeof(((type* )0)->name)* addr = (ptr);      \
    (type *)((char* )addr - PTR_OFFSET(type, name));    \
})

/* LIST_FIRST_ENTRY - get first emtry from a list
   @ptr - the struct's list_head pointer
   @type - type of the struct list_head is embeded in
   @name - name of the lsit_head struct */
#define LIST_FIRST_ENTRY(head, type, name) \
    LIST_ENTRY((head)->next, type, name)

/* iterate through a list */
#define list_itr(itr, head) \
    for ((itr) = (head)->next; (itr) != (head); (itr) = (itr)->next)

/* iterate through a list in reverse order */
#define list_itr_rev(itr, head) \
    for ((itr) = (head)->prev; (itr) != (head); ((itr) = (itr)->prev))

/* iterate through a list of given type */
#define list_entry_itr(itr, head, name) \
    for ((itr) = LIST_ENTRY((head)->next, typeof(*itr), name); &(itr)->name != (head); (itr) = LIST_ENTRY((itr)->name.next, typeof(*itr), name))

/* iterate through a list of given type in reverse order */
#define list_entry_itr_rev(itr, head, name) \
    for ((itr) = LIST_ENTRY((head)->prev, typeof(*itr), name); &(itr)->name != (head); (itr) = LIST_ENTRY((itr)->name.prev, typeof(*itr), name))

/* list_is_tail
   description: test whether a node is the tail of the list
   input: node - node to test
          head - head of the list
   output: none
   return value: none
   side effect: none
*/
static inline int list_is_tail(const struct list_head* node, struct list_head* head) {
    return node->next == head;
}


/* list_is_empty
   description: test whether a list is empty.
   input: head - head of the list
   output: none
   return value: none
   side effect: none
*/
static inline int list_is_empty(const struct list_head* head) {
    return head->next == head;
}


/* init_list_head
   description: initialize a node as the head of the list. in this case, the node is self-contained.
   input: node - node to initialize
   output: none
   return value: none
   side effect: none
*/
static inline void init_list_head(struct list_head* node) {
    node->next = node->prev = node;
}


/* __list_insert
   description: insert a node betwen two given nodes
   input: node - node to insert
          prev - node to insert after
          next - node to insert before
   output: none
   return value: none
   side effect: none
*/
static inline void __list_insert(struct list_head* node, struct list_head* prev, struct list_head* next) {
    next->prev = node;
    node->prev = prev;
    node->next = next;
    prev->next = node;
}


/* __list_delete
   description: delete a node betwen two given nodes
   input: prev - node to delete after
          next - node to delete before
   output: none
   return value: none
   side effect: none
*/
static inline void __list_delete(struct list_head* prev, struct list_head* next) {
    next->prev = prev;
    prev->next = next;
}


/* list_insert_after
   description: insert a node after a given node
   input: node - node to insert
          head - node to insert after
   output: none
   return value: none
   side effect: none
*/
static inline void list_insert_after(struct list_head* node, struct list_head* head) {
    __list_insert(node, head, head->next);
}


/* list_insert_before
   description: insert a node before a given node
   input: node - node to insert
          head - node to insert after
   output: none
   return value: none
   side effect: none
*/
static inline void list_insert_before(struct list_head* node, struct list_head* tail) {
    __list_insert(node, tail->prev, tail);
}


/* list_delete
   description: delete a node
   input: node - node to delete
   output: none
   return value: none
   side effect: none
*/
static inline void list_delete(struct list_head* node) {
    __list_delete(node->prev, node->next);
    node->next = LIST_POISON1;
    node->prev = LIST_POISON2;
}


/* list_replace
   description: replace dest node with src node.
   input: dest - old node to be replace
          src - new node to replace
   output: none
   return value: none
   side effect: none
*/
static inline void list_replace(struct list_head* dest, struct list_head* src) {
    src->next = dest->next;
    src->next->prev = src;
    src->prev = dest->prev;
    src->prev->next = src;
}


/* list_merge
   description: merge two lists. the list is added after the position.
   input: head - head of the list to merge
          pos - position to add after
   output: none
   return value: none
   side effect: none
*/
static inline void list_merge(struct list_head* head, struct list_head* pos) {
    if (!list_is_empty(head)) {
        struct list_head* first = head->next;
        struct list_head* last = head->prev;
        struct list_head* temp = pos->next;
        first->prev = pos;
        pos->next = first;
        last->next = temp;
        temp->prev = last;
    }
}

#endif
