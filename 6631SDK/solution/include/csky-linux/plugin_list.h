#ifndef _PLUGIN_LIST_H_
#define _PLUGIN_LIST_H_

struct dlist_head
{
    struct dlist_head *next, *prev;
};

#define DLIST_HEAD_INIT(name) { &(name), &(name) }


#define INIT_DLIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

void list_add     (struct dlist_head *newnode, struct dlist_head *head);
void list_add_tail(struct dlist_head *newnode, struct dlist_head *head);
void list_del     (struct dlist_head *entry);
void list_del_init(struct dlist_head *entry);
int  list_empty   (struct dlist_head *head );
void list_splice  (struct dlist_head *list, struct dlist_head *head);

struct dlist_head *list_get(struct dlist_head *head);

#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)
	
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
			pos = pos->next)

#define list_for_each_entry(pos, head, member)                            \
	for (pos = list_entry((head)->next, typeof(*pos), member);        \
			&pos->member != (head);                                     \
			pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)                            \
	for (pos = list_entry((head)->prev, typeof(*pos), member);        \
			&pos->member != (head);                                     \
			pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_from_node(pos, node, member, head)                            \
	for (pos = list_entry((node)->next, typeof(*pos), member);        \
			pos != (head);                                     \
			pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_from_node_reverse(pos, node, member, head)                            \
	for (pos = list_entry((node)->prev, typeof(*pos), member);        \
			pos != (head);                                     \
			pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
			pos = n, n = pos->next)

#define list_for_each_entry_safe(pos, n, head, member)                           \
	for (pos = list_entry((head)->next, typeof(*pos), member),               \
			n = list_entry(pos->member.next, typeof(*pos), member);  \
			&pos->member != (head);                                    \
			pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, typeof(*pos), member),	\
		n = list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.prev, typeof(*n), member))


#endif
