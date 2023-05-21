#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

#define STACKSIZE 32768        /* tamanho de pilha das threads */
#define QUANTUM 20

#define DEAD 0
#define ALIVE 1

task_t taskMain, *atual, *aux, dispatcher, *next = NULL;
int userTasks = 0, clock, userSleep = 0, exitCode;
task_t *filaOrdem, *filaProntos, *filaDorme;
// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;
// estrutura de inicialização to timer
struct itimerval timer;

unsigned int systime ()
{
    return(clock-1);
}

// suspende a tarefa corrente por t milissegundos
void task_sleep (int t)
{
    #ifdef DEBUG
        printf("A tarefa %d está indo dormir\n", atual->id);
    #endif

    atual->sleeping = 1;
    if(filaProntos == NULL){
        printf("eu entrei aqui\n");
        task_switch(atual);
    }
    else
        queue_append((queue_t **) &filaDorme, (queue_t *) queue_remove((queue_t **) &filaProntos, (queue_t *) atual));
    atual->sleep_time = t + systime();
    task_yield();
    return;
}

// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task)
{
    if(task == NULL || !task->status){
        return(-1);
    }

    #ifdef DEBUG
        printf("A tarefa %d está sendo colocado em uma fila, para esperar %d terminar\n", atual->id, task->id);
    #endif

    queue_append((queue_t **)&(task->filaSuspensa), queue_remove((queue_t **) &filaProntos, (queue_t*) atual));
    if(task->sleeping)
        task_yield();
    else
        task_switch(task);
    return(exitCode);
}



void tratador ()
{
    atual->cpu++;
    if(atual != &dispatcher)
    {
        atual->time--;
        if(atual->time <= 0){
            task_yield();
        }
    }
    clock++;
    return;
}


task_t *scheduler()
{
    if(atual != &dispatcher)
    {
        fprintf(stderr, "Erro\n");
        exit(1);
    }

    //Vai fazer a verificação da lista de fila dorme
    if(filaDorme)
    {
        task_t *aux, *controle = filaDorme;
        for (int i = 0; i < queue_size((queue_t *) filaDorme); i++)
        {
            if(controle->sleep_time <= systime()){
                aux = controle->next;
                queue_append((queue_t **) &filaProntos, (queue_t *) queue_remove((queue_t **) &filaDorme, (queue_t *) controle));
                controle->sleeping = 0;
                controle = aux;
            }
            controle = controle->next;
        }
    }
    userTasks = queue_size((queue_t *)filaProntos);
    userSleep = queue_size((queue_t *)filaDorme);
    if(userTasks > 0)
    {
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
        // queue_remove((queue_t **)&filaProntos, (queue_t *) next);
        // next->age = 0;
        /*Realiza o aging*/
        controle = filaProntos;
        for(int i=0; i < userTasks; i++)
        {
            // if(next != controle)
                controle->age-=1;
            // else
            //     controle->age =0;
            controle = controle->next;
        }
    }
    else
        return(&dispatcher);
    
    next->age = 0;
    // if(next != NULL)
        return(next);
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio)
{
    if(task){
        task->prio = prio;
        task->age = 0;
        #ifdef DEBUG
        printf("A nova prioridade da tarefa %d é %d\n",task->id, task->prio);
        #endif
    }
    else{
        atual->prio = prio;
        atual->age = 0;
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
    userSleep = queue_size((queue_t *)filaDorme);
    #ifdef DEBUG
        printf("A quantidade de tarefas atualmente é %d\n", userTasks);
        printf("A quantidade de tarefas dormindo é %d\n", userSleep);
    #endif

    task_switch(&dispatcher);
}

void dispatcher_body () // dispatcher é uma tarefa
{
   while(userTasks > 0 || userSleep > 0)
   {
      next = scheduler() ;  // scheduler é uma função
      if (next && next != &dispatcher)
      {
        next->time = QUANTUM;
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

    action.sa_handler = tratador;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 10 ;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }

    filaOrdem = NULL;
    filaProntos = NULL;
    filaDorme = NULL;
    // ///
    
    //Inicialização da taskMain
    queue_append((queue_t **) &filaProntos, (queue_t *) &taskMain);
    taskMain.id = 0;
    taskMain.time = QUANTUM;
    taskMain.age = 0;
    taskMain.prio = 0;
    taskMain.ativation = 0;
    taskMain.cpu = 0;
    taskMain.clock_total = systime()-1;
    taskMain.sleep_time = 0;
    taskMain.status = ALIVE;

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
        queue_append((queue_t **) &filaProntos, (queue_t *) task);
    
    if(task->prev == &taskMain && task->next == &taskMain)
        task->id = dispatcher.id + 1;
    else
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
        task->time = QUANTUM;
        task->filaSuspensa = NULL;
    }
    task->ativation = 0;
    task->cpu = 0;
    task->clock_total = systime()-1;
    task->status = ALIVE;
    return(1);
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
    #ifdef DEBUG
    printf ("task_exit: tarefa %d sendo encerrada\n", atual->id) ;
    #endif

    exitCode = exitCode;
    atual->clock_total = systime();
    if(userTasks > 0 || userSleep > 0)
    {
        printf("Task %d exit: execution time %4d ms, processor time %4d ms, %d activations\n",atual->id, 
            atual->clock_total, atual->cpu, atual->ativation );
        while(atual->filaSuspensa)
        {
            queue_append((queue_t **) &filaProntos, (queue_t *) queue_remove((queue_t **)&(atual->filaSuspensa),(queue_t *)atual->filaSuspensa));
        }
        queue_remove((queue_t **)&filaProntos, (queue_t *) atual);
        atual->status = DEAD;
        task_yield();
    }
    else
    {
        printf("Task %d exit: execution time %4d ms, processor time %4d ms, %d activations\n",atual->id, 
        atual->clock_total, atual->cpu, atual->ativation );
        queue_remove((queue_t **)&filaOrdem, (queue_t *) atual);
    }


    return;
    
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{
    if(atual == task)
        return(-1);

    aux = atual;
    atual = task;
    //
    atual->ativation++;
    //
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
