#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include <stdlib.h>
#include <string.h>

typedef struct fila_t{
	int inicio;
	int fim;
	int vetor[5];
}fila_t;

semaphore_t s_vaga, s_buffer, s_item;
task_t prod1, prod2, prod3, cons1, cons2;
fila_t fila;

void insere_fila(int valor)
{
	fila.vetor[fila.fim] = valor;
	
	if(fila.fim == 4)
		fila.fim = 0;
	else
		fila.fim++;

	
	return;
}

int retira_fila()
{
	int valor;
	valor = fila.vetor[fila.inicio];

	if(fila.inicio == 4)
		fila.inicio = 0;
	else
		fila.inicio++;

	return(valor);
}

void produtor(void *arg)
{
   while (1)
   {
      task_sleep (1000);
      int item = random() % 99;

      sem_down(&s_vaga);

      sem_down (&s_buffer);
      //insere item no buffer
      insere_fila(item);

      printf("%s produziu %d\n", (char *) arg, item);
      // printf("Produzi algo\n");

      //Libera o buffer
      sem_up(&s_buffer);
      
      //Sinaliza os consumidores de quem tem um item disponivel
      sem_up(&s_item);
   }
}


void consumidor(void *arg)
{
   while (1)
   {
      sem_down (&s_item);

      //Pede acesso ao buffer
      sem_down (&s_buffer);

      int item = retira_fila();
      // printf("Consumi algo\n");

      sem_up (&s_buffer);
      //Libera espaço do buffer

      sem_up (&s_vaga);
      //Libera uma vaga no buffer
      printf("%s consumiu %d \n", (char *)arg, item);
      //print item
      task_sleep (1000);
   }
}

int main(int argc, char *argv[])
{
	printf("main: inicio %d ms\n", systime());

   ppos_init();

   sem_create(&s_buffer,1);
   sem_create(&s_item,0);
   sem_create(&s_vaga,5);

	//Cria os produtores
	task_create(&prod1,produtor,"p1 ");
	task_create(&prod2,produtor,"   p2 ");
	task_create(&prod3,produtor,"      p3 ");

	//Cria os consumidores
	task_create(&cons1,consumidor,"                   c1 ");
	task_create(&cons2,consumidor,"                      c2 ");

	fila.inicio = 0;
	fila.fim = 0;

	task_join(&prod1);

   //Destroi semaforos
   sem_destroy(&s_item);
   sem_destroy(&s_buffer);
   sem_destroy(&s_vaga);

   printf("main: fim %d ms\n", systime());
	return 0;
}