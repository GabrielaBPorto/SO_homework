#include <stdio.h>
#include "queue.h"

void queue_append (queue_t **queue, queue_t *elem){
	//Se o elemento for nulo, ele não existe, e não pode ser adicionado a fila
	if(elem == NULL)
	{
		fprintf(stderr,"O elemento não existe\n");
		return;
	}
	//Se o elemento tiver proximo, ou anterior, significa que esta em outra lista
	if(elem->next != NULL || elem->prev != NULL)
	{
		fprintf(stderr, "O elemento existe em outra lista\n");
		return;
	}
	queue_t *aux = elem;
	//A fila está vazia
	if((*queue) == NULL && queue_size(*queue) < 1)
	{
		aux->prev = aux;
		aux->next = aux;
		*queue = aux;
		return;
	}
	queue_t *first = *queue;
	queue_t *prox = first->next;
	//Vai até o fim da lista para colocar o próximo elemento
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
	//Se a fila estiver vazia
	if((*queue) == NULL)
	{
		fprintf(stderr, "A lista não eiste\n");
		return(NULL);
	}
	//Se existem menos do que 1 elemento dentro da lista, não existem elementos dentro da lista
	if(queue_size(*queue) < 1)
	{
		fprintf(stderr, "Nao existe elementos dentro da lista\n");
		return(NULL);
	}
	//O elemento não existe
	if(elem == NULL)
	{
		fprintf(stderr, "O elemento nao existe\n");
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
		fprintf(stderr, "O elemento existe em outra lista\n");
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

