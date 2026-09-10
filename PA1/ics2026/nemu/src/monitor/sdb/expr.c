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

#include <isa.h>
#include<stdlib.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <string.h>
#include <memory/vaddr.h>

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_NUM,
  TK_NEG,
  TK_HEX,
  TK_REG,
  TK_NEQ,
  TK_AND,
  TK_DEREF
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-",'-'},
  {"\\*",'*'},
  {"/",'/'},
  {"\\(",'('},
  {"\\)",')'},
  {"==", TK_EQ},  //equal
  {"!=",TK_NEQ},
  {"&&",TK_AND},
  {"0[xX][0-9a-fA-F]+",TK_HEX},
  {"[0-9]+",TK_NUM},
  {"\\$[a-zA-Z0-9_]+", TK_REG}
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[8196] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* 依次尝试每条规则 */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len,
            substr_len, substr_start);

        position += substr_len;

        /* 非空格 token 需要占用数组位置 */
        if (rules[i].token_type != TK_NOTYPE &&
            nr_token >= sizeof(tokens) / sizeof(tokens[0])) {
          printf("Too many tokens\n");
          return false;
        }

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            /* 跳过空格 */
            break;

          case TK_NUM:
	  case TK_HEX:
	  case TK_REG:
            /* 留一个位置保存 '\0' */
            if (substr_len >= sizeof(tokens[nr_token].str)) {
              printf("Number token is too long\n");
              return false;
            }

            tokens[nr_token].type = rules[i].token_type;
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;

          default:
            /* 运算符和括号只记录类型 */
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
        }

        /* 当前 token 匹配成功，停止尝试其他规则 */
        break;
      }
    }

    /* 所有规则都匹配失败 */
    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n",
             position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p,int q,bool *success)
{
if(p>q)
{
  *success=false;
  return false;
}
int depth=0;
bool enclosed=(tokens[p].type=='(')&&(tokens[q].type==')');
for(int i=p;i<=q;i++)
{
   if(tokens[i].type=='(')
   {
     depth++;
   }
   else if(tokens[i].type==')')
   {
     depth--;
   }
  if(depth<0)
  {
    *success=false;
    return false;
  }
  if(i<q&&depth==0)
  {
    enclosed=false;
  }
}
if(depth!=0)
{
*success=false;
return false;
}
return enclosed;
}

static word_t eval(int p,int q,bool *success)
{
  if(*success==false)
  {
    return 0;
  }
  if(p>q)
  {
     *success=false;
     return 0;
  }
  else if (p == q)
  {
    if (tokens[p].type == TK_NUM) {
        return (word_t)strtoul(tokens[p].str, NULL, 10);
    }

    if (tokens[p].type == TK_HEX) {
        return (word_t)strtoul(tokens[p].str, NULL, 16);
    }
    if (tokens[p].type == TK_REG) {
        return isa_reg_str2val(tokens[p].str + 1, success);
    }

    *success = false;
    return 0;
  }
  bool enclosed=check_parentheses(p,q,success);
  if(!*success)
  {
    return 0;
  }
  if(enclosed)
  {
   return eval(p+1,q-1,success); 
  }

  int op=-1;
  int lowest_priority=5;
  int depth=0;
  for(int i=p;i<=q;i++)
  {
     int type=tokens[i].type;
     if(type=='(')
     {
        depth++;
	continue;
     }
     else if(type==')')
     {
        depth--;
        continue;
     }
     if(depth>0)
     {
       continue;
     }

     int priority;
     if(type==TK_AND)
     {
       priority=1;
     }
     else if(type==TK_EQ||type==TK_NEQ)
     {
       priority=2;
     }
     else if(type=='+'||type=='-')
     {
       priority=3;
     }
     else if(type=='*'||type=='/')
     {
       priority=4;
     }
     else
     {
     continue;
     }

     if(priority<=lowest_priority)
     {
       lowest_priority=priority;
       op=i;
     }
  }
  if (op == -1)
  {
    if (tokens[p].type == TK_NEG ||
        tokens[p].type == TK_DEREF)
    {
        word_t value = eval(p + 1, q, success);
        if (!*success) {
            return 0;
        }

        if (tokens[p].type == TK_NEG) {
            return (word_t)0 - value;
        }

        return vaddr_read(value, 4);
    }

    *success = false;
    return 0;
  }
  word_t val1=eval(p,op-1,success);
  if(!*success)
  {
    return 0;	  
  }
  if (tokens[op].type == TK_AND && val1 == 0) {
    return 0;
  }
  word_t val2=eval(op+1,q,success);
  if(!*success)
  {
    return 0;
  }
  switch(tokens[op].type)
  {
   case '+':
	   return val1+val2;
   case '-':
	   return val1-val2;
   case '*':
	   return val1*val2;
   case '/':
	   if(val2==0)
	   {
	     *success=false;
	     return 0;
	   }
	   return val1/val2;
   case TK_EQ:
           return val1 == val2;

   case TK_NEQ:
           return val1 != val2;

   case TK_AND:
           return val1 && val2;
   default:
	   *success=false;
	   return 0;
  }

}  



word_t expr(char *e, bool *success) {

  *success=true;
 

  if (e==NULL||!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  
  if(nr_token==0)
  {
    *success=false;
    return 0;
  }
  for (int i = 0; i < nr_token; i++)
  {
    bool unary = (i == 0 ||
        (tokens[i - 1].type != TK_NUM &&
         tokens[i - 1].type != TK_HEX &&
         tokens[i - 1].type != TK_REG &&
         tokens[i - 1].type != ')'));

    if (unary) {
        if (tokens[i].type == '-') {
            tokens[i].type = TK_NEG;
        }
        else if (tokens[i].type == '*') {
            tokens[i].type = TK_DEREF;
        }
    }
  }
  
  return eval(0,nr_token-1,success);
}
