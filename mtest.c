#include<stdio.h>
#include<malloc.h>
#include<unistd.h>
int bss_var;      //uninitialized static variable(in BSS)
int data_var0=1;  //initialized static variable(in data)

void show_stack_growth(int stack_var0)  //"a" presents function parameter(in stack)
{
  static int time = 0; /* called time */
	auto int stack_var1;  //automatic variable(on stack)
  if (++time == 3) /* avoid infinite recursion */
		return;
  printf("\tfunction called %d: address of stack_var0(local variable): %p stack_var1(function parameter): %p\n", time, & stack_var1, &stack_var0);
	show_stack_growth(data_var0); /* recursive call */
}
int main(int argc,char **argv)
{
  printf("below are addresses of types of process's mem\n");
  printf("Text location:\n");
  printf("\tAddress of main(Code Segment):%p\n",main);
  printf("\tAddress of show_stack_growth(Code Segment):%p\n",show_stack_growth);
  printf("____________________________\n");

  printf("Stack Location:\n");
  show_stack_growth(data_var0);
  printf("____________________________\n");
  
  printf("Data Location:\n");
  printf("\tAddress of data_var(Data Segment):%p\n",&data_var0);
  static int data_var1=4;
  printf("\tNew end of data_var(Data Segment):%p\n",&data_var1);
  printf("____________________________\n");
  
  printf("BSS Location:\n");
  printf("\tAddress of bss_var:%p\n",&bss_var);
  printf("____________________________\n");

  char *b = sbrk((ptrdiff_t)0);   /*initial end of heap*/
  printf("Heap Location:\n");
  printf("\tInitial end of heap:%p\n",b);
  b = sbrk((ptrdiff_t) 32);     /*grow heap*/
  b = sbrk((ptrdiff_t) 0);
  printf("\tAfter growth:%p\n",b);
  b=sbrk((ptrdiff_t)-6);
  b=sbrk((ptrdiff_t)0);      /*shrink it*/
  printf("\tAfter shrink:%p\n",b); 

  sleep(1000);
  return 0;
}