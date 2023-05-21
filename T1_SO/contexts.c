// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.0 -- Março de 2015

// Demonstração das funções POSIX de troca de contexto (ucontext.h).

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

#define _XOPEN_SOURCE 600	/* para compilar no MacOS */

#define STACKSIZE 32768		/* tamanho de pilha das threads */

#define CONTEXT_CHANGE 10

ucontext_t ContextPing, ContextPong, ContextMain ;

/*****************************************************/

void BodyPing (void * arg)
{
   int i ;

   printf ("%s: inicio\n", (char *) arg) ;

   for (i=0; i<CONTEXT_CHANGE; i++)
   {
      printf ("%s: %d\n", (char *) arg, i) ;
      swapcontext (&ContextPing, &ContextPong) ;
   }
   printf ("%s: fim\n", (char *) arg) ;

   swapcontext (&ContextPing, &ContextMain) ;
}

/*****************************************************/

void BodyPong (void * arg)
{
   int i ;

   printf ("%s: inicio\n", (char *) arg) ;

   for (i=0; i<1; i++)
   {
      printf ("%s: %d\n", (char *) arg, i) ;
      swapcontext (&ContextPong, &ContextPing) ;
   }
   //Eu preciso continuar a chamada de troca de contexto, se um valor for modificado
   while(i < CONTEXT_CHANGE)
   {
   		swapcontext(&ContextPong,&ContextPing);
   		i++;
   }
   printf ("%s: fim\n", (char *) arg) ;

   swapcontext (&ContextPong, &ContextMain) ;
}

/*****************************************************/

int main (int argc, char *argv[])
{
   char *stack ;

   printf ("main: inicio\n") ;

   getcontext(&ContextPing);

   stack = malloc (STACKSIZE) ;
   //Se o stack ganhar o malloc certo ( stack ganha malloc no tipo void?)
   if (stack)
   {
      // Base do stack de signals
      ContextPing.uc_stack.ss_sp = stack ;
      //Tamanho em bytes
      ContextPing.uc_stack.ss_size = STACKSIZE ;
      ContextPing.uc_stack.ss_flags = 0 ;
      //ponteiro para o contexto que será resumido quando voltar o contexto
      ContextPing.uc_link = (ucontext_t *) 1 ;
   }
   else
   {
      perror ("Erro na criação da pilha: ") ;
      exit (1) ;
   }

   makecontext (&ContextPing, (void*)(*BodyPing), 1, "    Ping") ;

   getcontext (&ContextPong) ;

   stack = malloc (STACKSIZE) ;
   if (stack)
   {
      ContextPong.uc_stack.ss_sp = stack ;
      ContextPong.uc_stack.ss_size = STACKSIZE ;
      ContextPong.uc_stack.ss_flags = 0 ;
      ContextPong.uc_link = (ucontext_t *)1 ;
   }
   else
   {
      perror ("Erro na criação da pilha: ") ;
      exit (1) ;
   }

   makecontext (&ContextPong, (void*)(*BodyPong), 1, "        Pong") ;

   swapcontext (&ContextMain, &ContextPing) ;
   swapcontext (&ContextMain, &ContextPong) ;

   printf ("main: fim\n") ;

   exit (0) ;
}
