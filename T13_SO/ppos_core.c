#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#define STACKSIZE 32768        /* tamanho de pilha das threads */
#define QUANTUM 20

#define DEAD 0
#define ALIVE 1



int preempcion = 0;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;
// estrutura de inicialização to timer
struct itimerval timer;



// filas de mensagens

// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size){
    if(max <= 0)
        return(-1);
    if(size <= 0)
        return(-1);
    if(queue == NULL)
        return(-1);

    queue->max_msgs = max;
    queue->msg_size = size;
    queue->qtd = 0;
    sem_create(&(queue->vaga),max);
    sem_create(&(queue->buffer),1);
    sem_create(&(queue->mssg), 0);
    
    if(!(queue->msg = (void *) malloc (max * size)))
        printf("Erro na alocação de memória\n");

    queue->inicio = queue->fim = queue->msg;

    return(0);
}

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg) {
    if(queue == NULL)
        return(-1);

    if(sem_down(&(queue->vaga)))
        return(-1);

    sem_down(&(queue->buffer));

    queue->qtd++;
    memcpy(queue->fim, msg, queue->msg_size);
    if(((void *) ((char *) queue->fim + queue->msg_size)) > (void *) ((char *) queue->msg + (queue->msg_size * queue->max_msgs)))
        queue->fim = queue->msg;
    else
        queue->fim = (void *) ((char *) queue->fim + queue->msg_size);

    sem_up(&(queue->buffer));

    sem_up(&(queue->mssg));

    return(0);
}

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg){
    if(queue == NULL)
        return(-1);

    if(sem_down(&(queue->mssg)))
        return(-1);

    sem_down(&(queue->buffer));

    queue->qtd--;
    memcpy(msg, queue->inicio, queue->msg_size);
    if(((void *) ((char *) queue->inicio + queue->msg_size)) > (void *) ((char *) queue->msg + (queue->msg_size * queue->max_msgs)))
        queue->inicio = queue->msg;
    else
        queue->inicio = (void *) ((char *) queue->inicio + queue->msg_size);

    sem_up(&(queue->buffer));

    sem_up(&(queue->vaga));
    
    return(0);
}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue){

    if(queue == NULL)
        return(-1);

    sem_destroy(&(queue->vaga));
    sem_destroy(&(queue->buffer));
    sem_destroy(&(queue->mssg));

    // free(queue->msg);

    return(0);
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue){
    return(queue->qtd);
}


unsigned int systime ()
{
    return(clock);
}

// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value){
    if(s == NULL)
        return(-1);

    #ifdef DEBUG
        printf("Semáforo Criado\n");
    #endif
    s->status = ALIVE;
    s->fila = NULL;
    s->count = value;
    if(s->count < 0)
        //Faz algo importante
        printf("o valor é menor do que 0\n");


    return(0);
}

// requisita o semáforo
//solicita acesso a sessão critica
int sem_down (semaphore_t *s){
    if(s == NULL || s->status == DEAD)
        return(-1);

    #ifdef DEBUG
        printf("Dando sem_down, a tarefa atual é %d\n", atual->id);
    #endif
    s->count--;

    if(s->count < 0)
    {
        queue_append((queue_t **) &(s->fila), (queue_t *)queue_remove((queue_t **)&filaProntos, (queue_t *) atual));
        task_yield();   
    }

    return(0);
}

// libera o semáforo
int sem_up (semaphore_t *s){
    if(s == NULL || s->status == DEAD)
        return(-1);

    #ifdef DEBUG
        printf("Dando sem_up, a tarefa atual é %d\n", atual->id);
    #endif


    if(!preempcion)
        preempcion = 1;

    s->count++;
    if(s->fila){
        queue_append((queue_t **) &filaProntos, (queue_t *) queue_remove((queue_t **) &(s->fila), (queue_t *) s->fila));
        // printf("Everyday of my life\n");
    }

    return(0);
}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) {
    // Quando for destruido dar um s->status = DEAD
    if(s == NULL || s->status == DEAD){
        return(-1);
    }

    #ifdef DEBUG
        printf("Semaforo sendo destruido\n");
    #endif

    while(s->fila)
    {
        queue_append((queue_t **) &filaProntos, (queue_t *) queue_remove((queue_t **) &(s->fila), (queue_t *) s->fila));
    }

    s->status = DEAD;

    return(0);
}

// suspende a tarefa corrente por t milissegundos
void task_sleep (int t)
{
    #ifdef DEBUG
        printf("A tarefa %d está indo dormir\n", atual->id);
    #endif

    atual->sleeping = 1;
    if(filaProntos == NULL){
        task_switch(atual);
    }
    else{
        queue_append((queue_t **) &filaDorme, (queue_t *) queue_remove((queue_t **) &filaProntos, (queue_t *) atual));
    }
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
    // if(task->sleeping)
        task_yield();
    // else
    //     task_switch(task);
    return(exitCode);
}

void tratador ()
{
    atual->cpu++;
    // if(preempcion){
        if(atual != &dispatcher)
        {
            atual->time--;
            if(atual->time <= 0){
                task_yield();
            }
        }
    // }
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
        /*Realiza o aging*/
        controle = filaProntos;
        for(int i=0; i < userTasks; i++)
        {
                controle->age-=1;
            controle = controle->next;
        }
    }
    else
        return(&dispatcher);
    
    next->age = 0;
        return(next);
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

    next = NULL;

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
    task->clock_total = systime();
    task->status = ALIVE;

    userTasks = queue_size((queue_t *)filaProntos);
    userSleep = queue_size((queue_t *)filaDorme);

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
        if(atual != &dispatcher){
            printf("Task %d exit: execution time %4d ms, processor time %4d ms, %d activations\n",atual->id, 
                atual->clock_total, atual->cpu, atual->ativation );
            while(atual->filaSuspensa)
            {
                queue_append((queue_t **) &filaProntos, 
                    (queue_t *) queue_remove((queue_t **)&(atual->filaSuspensa),(queue_t *)atual->filaSuspensa));
            }

            queue_remove((queue_t **)&filaProntos, (queue_t *) atual);
            atual->status = DEAD;
            task_yield();
        }
    }
    else
    {
        // if(atual == &dispatcher){
            printf("Task %d exit: execution time %4d ms, processor time %4d ms, %d activations\n",atual->id, 
            atual->clock_total, atual->cpu, atual->ativation );
            queue_remove((queue_t **)&filaOrdem, (queue_t *) atual);
        //}
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