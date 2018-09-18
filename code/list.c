#include "list.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*Funzione che alloca un nuovo nodo all'interno della lista collegata*/
struct fiber *alloc_node(void){
	struct fiber *p;

	p = vmalloc(sizeof(struct fiber));

	if (IS_ERR(p)) {
		printk(KERN_ALERT "alloc_node: failed\n");
		PTR_ERR(p);
	}

	return p;
}

/*Inserisco un elemento della lista che corrisponde ad un uovo client*/
void insert_new_node(pid_t parent, pid_t fiber_id, struct node_t **phead){
	struct fiber *new;

	new = alloc_node(); /*Creo un nuovo elemento..*/
	new->parent = parent;/*..inserisco la porta del client..*/
	new->fiber_id = fiber_id; /*..il suo indirizzo IP..*/
	new->active = 1; /*..ed il pid del processo del figlio che lo gestisce*/
	insert_after_node(new, phead); /*Infine lo inserisco all'interno della lista*/
}

/*Rimuovo il nodo p*/
void free_node(struct fiber *p){
	vfree(p);
}

/*Elimino l'intera lista*/
void free_all_nodes(struct fiber **h){
	struct fiber *p;

	while (*h != NULL) {
		p = remove_after_node(h);
		free_node(p);
	}
}

/*Inserisco un nuovo spostando di conseguenza i successivi*/
void insert_after_node(struct fiber *new, struct fiber **pnext){
	new->next = *pnext;
	*pnext = new;
}

/*Rimuovo un nodo spostando di conseguenza i successivi*/
struct fiber *remove_after_node(struct fiber **ppos){
	struct fiber *rpos = *ppos;
	*ppos = rpos->next;
	return rpos;
}

/*Rimuovo il client gestito dal processo specificato da pid*/
void remove_fiber(struct fiber **ppos, pid_t parent, pid_t fiber_id){
	//NOTE: /* Rememeber to check if the thread is deleting a fiber belonging to itself */
	struct fiber *r = *ppos;
	if( (r->parent = parent) && (r->fiber_id == fiber_id) ){
		remove_after_node(ppos);
	}
}

int check_is_in(struct fiber **pnext, pid_t fiber_id){

	struct fiber *p;

	for (p = *pnext; p != NULL; p = p->next) {
		if(p->fiber_id == fiber_id){
			return 1;// già presente in lista
		}
	}
	return 0;//non presente
}
//Not needed
/*Controllo se un client è già presente nella lista*/
int check_is_connected(struct node_t **pnext, unsigned long IP_addr, unsigned short port){

struct node_t *p;

for (p = *pnext; p != NULL; p = p->next) {
		if((p->IP_address == IP_addr && p->port == port)){
			return 1;// già presente in lista
		}
	}
	return 0;//non presente
}

