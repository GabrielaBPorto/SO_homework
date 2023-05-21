#include <stdio.h>
#include "queue.h"

void queue_append (queue_t **queue, queue_t *elem){
	if(elem == NULL)
	{
		fprintf(stdout,"O elemento não existe\n");
		return;
	}
	queue_t *aux = elem;
	if(aux->next != NULL && aux->prev != NULL)
	{
		fprintf(stdout, "O elemento existe em outra lista\n");
		return;
	}
	if((*queue) == NULL)
	{
		aux->prev = aux;
		aux->next = aux;
		*queue = aux;
		return;
	}
	queue_t *first = *queue;
	queue_t *prox = first->next;
	while(prox->next != first)
	{
		prox = prox->next;
	}
	first->prev = aux;
	aux->prev = prox;
	aux->next = first;
	prox->next = aux;
	return;
}

queue_t *queue_remove (queue_t **queue, queue_t *elem){
	if((*queue) == NULL)
	{
		fprintf(stdout, "A lista nao existe\n");
		return(NULL);
	}
	if(queue_size(*queue) == 0)
	{
		fprintf(stdout, "Nao existe elementos dentro da lista\n");
		return(NULL);
	}
	if(elem == NULL)
	{
		fprintf(stdout, "O elemento nao existe\n");
		return(NULL);
	}
	queue_t *aux = elem;
	queue_t *first = *queue;
	queue_t *prox = first->next;
	short check = 0;
	////////////////////////////Verificaçao de fila/////////////////////////////////////////
	if(first == aux)
	{
		check = 1;
	}
	while(prox != first && (!check))
	{
		if(prox == aux)
		{
			check = 1;
		}
		prox = prox->next;
	}
	if(!check)
	{
		fprintf(stdout, "O elemento não existe dentro dessa lista\n");
		return(NULL);
	}
	//////////////////////////Verificaçao da cabeça da fila////////////////////////////////
	if(first == aux)
	{
		*queue = aux->next;
	}
	//////////////////////////Verificação de um elemento na fila//////////////////////////
	if(queue_size(*queue) == 1)
	{
		*queue = NULL;
		aux->next = NULL;
		aux->prev = NULL;
		return(aux);
	}
	aux->prev->next = aux->next;
	aux->next->prev = aux->prev;
	aux->next = NULL;
	aux->prev = NULL;
	return(aux);
}

int queue_size (queue_t *queue){
	//Se a fila é nula, retorna 0 elementos
	if(queue == NULL)
	{
		return(0);
	}
	int i = 1;
	queue_t *first = queue;
	queue_t *prox = first->next;
	while(prox != first)
	{
		i++;
		prox = prox->next;
	}
	return(i);
	
}

void queue_print (char *name, queue_t *queue, void print_elem (void*)){
	printf("%s: [", name);
	if (queue != NULL)
	{
		queue_t *first = queue;
		queue_t *prox = first;
		do
		{
			print_elem(prox);
			prox = prox->next;
			if(prox != first)
				printf(" ");
		}
		while(prox != first);
	}
	printf("]\n");
	return;
}

