// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas


// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   void *stack ;			// aponta para a pilha da tarefa
   int status;
   int prio;
   int age;
   int time;
   int clock_total;
   int cpu;
   int ativation;
   struct task_t *filaSuspensa;
   int sleep_time;
   int sleeping;
   int type;
   int bloco;
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  task_t *fila;
  int count;
  int status;
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  int max_msgs;
  int msg_size;
  void *msg;
  struct mqueue_t *inicio;
  struct mqueue_t *fim;
  semaphore_t vaga;
  semaphore_t buffer;
  semaphore_t mssg;
  int qtd;
  // preencher quando necessário
} mqueue_t ;

task_t *filaOrdem, *filaProntos, *filaDorme;
task_t taskMain, *atual, *aux, dispatcher, *next;
int userTasks, clock, userSleep, exitCode;

#endif