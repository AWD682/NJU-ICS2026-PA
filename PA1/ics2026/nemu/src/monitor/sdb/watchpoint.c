/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expression[256];
  word_t old_value;
  /* TODO: Add more members if necessary */

} WP;
static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

WP *new_wp(void)
{
   assert(free_!=NULL);
   
   WP *wp=free_;
   free_=wp->next;

   wp->next=head;
   head=wp;

   return wp;
}

void free_wp(WP *wp)
{
   assert(wp!=NULL);

   if(head==wp)
   {
     head=wp->next;
   }
   else 
   {
     WP *prev=head;
     while(prev!=NULL&&prev->next!=wp)
     {
        prev=prev->next;
     }

     assert(prev!=NULL);
     prev->next=wp->next;
     
   }
   
     wp->next=free_;
     free_=wp;
}


void create_watchpoint(char *expression)
{
   if(expression==NULL)
   {
     printf("Usage:w EXPR\n");
     return ;
   }
   
   while(*expression==' '||*expression=='\t')
   {
      expression++;
   }
   if(*expression=='\0')
   {
      printf("Usage:w EXPR\n");
      return ;
   }
   if (strlen(expression) >= sizeof(wp_pool[0].expression)) {
        printf("Expression is too long.\n");
        return;
   }

   bool success=true;
   word_t value=expr(expression,&success);
   if(!success)
   {
     printf("Invalid Expression\n");
     return;
   }
   WP *wp = new_wp();
   strcpy(wp->expression, expression);
   wp->old_value = value;

   printf("Watchpoint %d: %s\n", wp->NO, wp->expression);
   printf("Initial value = " FMT_WORD "\n", wp->old_value);

}

void delete_watchpoint(int no)
{
  WP *wp=head;
  while(wp!=NULL)
  {
    if(wp->NO==no)
    {
       free_wp(wp);
       printf("Deleted watchpoint %d.\n", no);
       return ;
    }
    wp=wp->next;
  
  }
  printf("Watchpoint %d does not exit\n",no);
}

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}
void display_watchpoint(void)
{
  if(head==NULL)
  {
    printf("No watchpoint\n");
    return ;
  }
  printf("NO\tValue\t\tExpression\n");
  for(WP *wp=head;wp!=NULL;wp=wp->next)
  {
    printf("%d\t" FMT_WORD "\t%s\n",wp->NO, wp->old_value, wp->expression);
  }


}
bool check_watchpoint(void)
{
  bool need_stop=false;

  for(WP *wp=head;wp!=NULL;wp=wp->next)
  {
     bool success=true;
     word_t new_value=expr(wp->expression,&success);
     if (!success)
        {
            printf("Failed to evaluate watchpoint %d: %s\n",
                   wp->NO, wp->expression);

            need_stop = true;
            continue;
        }
     if(new_value!=wp->old_value)
     {
        printf("Watchpoint %d triggered: %s\n",wp->NO,wp->expression);
	printf("Old value = " FMT_WORD "\n", wp->old_value);
        printf("New value = " FMT_WORD "\n", new_value);
        wp->old_value=new_value;
	need_stop=true;
     }
  
  }
   return need_stop;
}
/* TODO: Implement the functionality of watchpoint */

