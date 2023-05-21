#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h> 
#include "ppos_disk.h"
#include "ppos.h"
#include "ppos_data.h"
#include "hard_disk.h"
#include "queue.h"

#define WRITE 1
#define READ 2

disk_t disco;

void diskDriverBody (void * args)
{
	#ifdef DEBUG
		printf("Estou no gerenciador de disco\n");
	#endif
   	while (1) 
   {
   		printf("hasudhbij\n");
   	  sem_down(&(disco.acesso));
   	  
      // obtém o semáforo de acesso ao disco
 
      // se foi acordado devido a um sinal do disco
      if(disco.sinal){
      	disco.sinal = 0;
      	queue_append((queue_t **) &filaProntos, queue_remove((queue_t **) &(disco.fila), (queue_t *) disco.fila));
      }
 		
      // se o disco estiver livre e houver pedidos de E/S na fila
      if ((disk_cmd (DISK_CMD_STATUS, 0, 0)) == 1 && (disco.fila != NULL))
      {
      	task_t *auxiliar;
      	auxiliar = (task_t *)queue_remove((queue_t **) &(disco.fila), (queue_t*) disco.fila);

      	if(auxiliar->type == READ){
      		printf("UHAL\n");
      		printf("%d e msg %s\n", auxiliar->bloco, (char*)disco.queue->msg );
      		disk_cmd (DISK_CMD_READ, auxiliar->bloco, disco.queue->msg);
      		printf("UHUL\n");
      	}
      	if(auxiliar->type == WRITE){
      		if(disk_cmd (DISK_CMD_WRITE, auxiliar->bloco, disco.queue->msg) < 0)
      			fprintf(stderr, "ERRO ESCRITA\n");
      	}
      }
 	  printf("AShbdjsakbdkas\n");	
	  queue_remove((queue_t**) &filaProntos, (queue_t *) &(disco.gerente)); 	
      disco.busy = 0;
 		sem_up(&(disco.acesso));
      // libera o semáforo de acesso ao disco
 		task_yield();
      // suspende a tarefa corrente (retorna ao dispatcher)
   }
}

void trata(int sinal)
{
	#ifdef DEBUG
		printf("Tratador\n");
	#endif
	disco.sinal = 1;
	if(!disco.busy){
		queue_append((queue_t **) &filaProntos, (queue_t *) &(disco.gerente));
		userTasks++;
	}
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize){
	if(disk_cmd (DISK_CMD_INIT, 0, 0) < 0)
		return(-1);

	if(task_create(&(disco.gerente),diskDriverBody,0) == -1)
		return(-1);

	queue_remove((queue_t**) &filaProntos, (queue_t *) &(disco.gerente));

    disco.action.sa_handler = trata;
    sigemptyset (&(disco.action.sa_mask));
    disco.action.sa_flags = 0 ;

	if (sigaction (SIGUSR1, &disco.action, 0) < 0)
    {
        perror ("Erro") ;
        exit (1) ;
    }

	*blockSize = disco.blockSize = disk_cmd (DISK_CMD_BLOCKSIZE, 0, 0);

	*numBlocks = disco.numBlocks = disk_cmd(DISK_CMD_DISKSIZE,0,0);

	disco.busy = 0;
	disco.sinal = 0;
	disco.fila = NULL;

	mqueue_create(disco.queue, *numBlocks, *blockSize);
	sem_create(&(disco.acesso),1);

	return(0);
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer){
	#ifdef DEBUG
		printf("Eu vou ler o disco\n");
	#endif
	sem_down(&(disco.acesso));
	// obtém o semáforo de acesso ao disco
 
 	// disco.busy = 1;

   	// inclui o pedido na fila_disco
   	atual->type = READ;
   	queue_append((queue_t **) &(disco.fila), queue_remove((queue_t **) &filaProntos, (queue_t *) atual));
   	mqueue_recv(disco.queue, buffer);
   	atual->bloco = block;
 
  	if(!disco.busy)
  	{
   		queue_append((queue_t **) &filaProntos, (queue_t *) &(disco.gerente));
   		disco.busy = 1;
    }
 
   // libera semáforo de acesso ao disco
	sem_up(&(disco.acesso));
 
 	task_yield();
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer){
	#ifdef DEBUG
		printf("Eu vou escrever no disco\n");
	#endif
	if(buffer == NULL)
		return(-1);

	disco.busy = 1;

	sem_down(&(disco.acesso));
	// obtém o semáforo de acesso ao disco
 	atual->type = READ;
   	queue_append((queue_t **) &(disco.fila), (queue_t *) atual);
   	mqueue_send(disco.queue, buffer);
   	atual->bloco = block;
   // inclui o pedido na fila_disco
 
   if (!disco.busy)
   {
      queue_append((queue_t **) &filaProntos, (queue_t *) &(disco.gerente));
      disco.busy = 1;
   }
 
   // libera semáforo de acesso ao disco
	sem_up(&(disco.acesso));

	task_yield();
   // suspende a tarefa corrente (retorna ao dispatcher)/
}
