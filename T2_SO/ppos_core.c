#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include <ucontext.h>
#include <stdlib.h>

#define STACKSIZE 32768        /* tamanho de pilha das threads */
#define DEBUG

task_t taskMain, *atual, *aux, dispatcher, *next = NULL;
int userTasks = 0;
task_t *filaOrdem, *filaProntos;

task_t *scheduler()
{
    if(atual != &dispatcher)
    {
        fprintf(stderr, "Erro\n");
        exit(1);
    }
    /*Faz uma busca de qual é o elemento com maior prioridade*/
    int min = filaProntos->prio;
    next = filaProntos;
    task_t *controle = filaProntos;
    for(int i=0; i < userTasks; i++)
    {
        if(controle->prio + controle->age < min){
            min = controle->prio + controle->age;
            next = controle;
        }
        controle = controle->next;
    }
    controle = filaProntos;
    for(int i=0; i < userTasks; i++)
    {
        if(next != controle)
            controle->age-=1;
        else
            controle->age = 0;
        controle = controle->next;
    }
    
    if(next != NULL)
        return(next);
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio)
{
    if(task){
        task->prio = prio;
        // task->age = prio;
        #ifdef DEBUG
        printf("A nova prioridade da tarefa %d é %d\n",task->id, task->prio);
        #endif
    }
    else{
        atual->prio = prio;
        // atual->age = prio;
        #ifdef DEBUG
        printf("A nova prioridade da atual %d é %d\n",atual->id, atual->prio);
        #endif
    }
    return;
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task)
{
    if(task)
    {
        #ifdef DEBUG
            printf("A prioridade da tarefa %d é %d\n",task->id, task->prio);
        #endif
        return(task->prio);
    }
    else
    {  
        #ifdef DEBUG
            printf("A prioridade da atual %d é %d\n",atual->id, atual->prio);
        #endif
        return(atual->prio);
    }
}

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield ()
{
    //Conta a quantidade de tarefas que ainda precisam ser executadas(contar a partir do dispatcher até main ?)
    userTasks = queue_size((queue_t *)filaProntos);
    #ifdef DEBUG
        printf("A quantidade de tarefas atualmente é %d\n", userTasks);
    #endif
    task_switch(&dispatcher);
}

void dispatcher_body () // dispatcher é uma tarefa
{
   while ( userTasks > 0 )
   {
      next = scheduler() ;  // scheduler é uma função
      if (next)
      {
        task_switch (next) ; // transfere controle para a tarefa "next"
      }
   }
   task_exit(0) ; // encerra a tarefa dispatcher
}

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init ()
{
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0) ;
    filaOrdem = NULL;
    filaProntos = NULL;
    queue_append((queue_t **) &filaOrdem, (queue_t *) &taskMain);
    taskMain.id = -1;
    getcontext(&(taskMain.context));
    atual = &taskMain;

    task_create(&dispatcher,dispatcher_body,0);
}

// gerência de tarefas =========================================================

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task,            // descritor da nova tarefa
                 void (*start_func)(void *),    // funcao corpo da tarefa
                 void *arg)            // argumentos para a tarefa
{
    if(task == &dispatcher)
        queue_append((queue_t **) &filaOrdem, (queue_t *) task);
    else
        queue_append((queue_t **)&filaProntos, (queue_t *) task);
    task->id = task->prev->id + 1;
    getcontext(&(task->context));
    task->stack =  malloc (STACKSIZE);

    if(task->stack)
    {
        task->context.uc_stack.ss_sp = task->stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        perror ("Erro na criação da pilha: ") ;
        return(-1);
    }

    makecontext(&(task->context), (void *) start_func, 1, arg);

    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->id) ;
    #endif
    if(task != &dispatcher){
        task_setprio(task,0);
        task->age = 0;
    }
    return(1);
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
    task_t *controle = NULL;
    #ifdef DEBUG
    printf ("task_exit: tarefa %d sendo encerrada\n", atual->id) ;
    #endif

    if(userTasks != 0)
    {
        controle = filaProntos;
        //Atualização dos ids
        while(controle->next != filaProntos)
        {
            controle->next->id = controle->id;
            controle = controle->next;
        }
        queue_remove((queue_t **)&filaProntos, (queue_t *) atual);
        task_yield();
    }

    queue_remove((queue_t **)&filaOrdem, (queue_t *) &dispatcher);
    task_switch(&taskMain);

    return;
    
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{
    aux = atual;
    atual = task;
    swapcontext(&(aux->context),&(atual->context));
    #ifdef DEBUG
    printf ("task_switch: trocando contexto %d -> %d \n", aux->id, atual->id);
    #endif
    return(0);
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id ()
{
    return(atual->id);
}