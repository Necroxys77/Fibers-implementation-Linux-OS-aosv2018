#ifndef _LIST_H
#include <unistd.h>
#include "fibermodule.h"
#define _LIST_H

struct node_t {
	unsigned char value;
	pid_t child_pid;
	unsigned long IP_address;
	unsigned short port;
	struct node_t *next;
};

struct node_t *alloc_node(void);
void insert_new_client(unsigned long , pid_t , struct node_t **, unsigned short);
void free_node(struct node_t *);
void free_all_nodes(struct node_t **);
void insert_after_node(struct node_t *, struct node_t **);
struct node_t *remove_after_node(struct node_t **);
void remove_client(struct node_t **, pid_t pid);
int check_is_in(struct node_t **, unsigned char);
int check_is_connected(struct node_t **, unsigned long ,unsigned short);

#endif

