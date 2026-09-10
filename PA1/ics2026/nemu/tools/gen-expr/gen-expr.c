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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int pos = 0;

/* 随机返回 0 到 n-1 */
static unsigned choose(unsigned n) {
  return (unsigned)rand() % n;
}

/* 向 buf 末尾添加一个字符 */
static void gen(char c) {
  assert(pos + 1 < sizeof(buf));
  buf[pos++] = c;
  buf[pos] = '\0';
}

/* 随机添加 0 到 2 个空格 */
static void gen_space(void) {
  unsigned n = choose(3);
  while (n-- > 0) {
    gen(' ');
  }
}

/* 生成 0 到 99 的无符号整数 */
static void gen_num(void) {
  char number[16];
  snprintf(number, sizeof(number), "%uu", choose(100));

  for (int i = 0; number[i] != '\0'; i++) {
    gen(number[i]);
  }
}

/* 递归生成表达式，限制递归深度 */
static void gen_expr(int depth) {
  gen_space();

  /* 到达深度上限，只生成数字，不再递归 */
  unsigned choice = depth >= 4 ? 0 : choose(3);

  switch (choice) {
    case 0:
      gen_num();
      break;

    case 1:
      gen('(');
      gen_expr(depth + 1);
      gen(')');
      break;

    default:
      gen_expr(depth + 1);
      gen("+-*/"[choose(4)]);
      gen_expr(depth + 1);
      break;
  }

  gen_space();
}

static void gen_rand_expr(void) {
  pos = 0;
  buf[0] = '\0';
  gen_expr(0);
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    snprintf(code_buf, sizeof(code_buf), code_format, buf);

/* code_buf 保留 u，交给 GCC 做无符号计算。
 * buf 去掉 u，交给你的 expr() 处理。
 */
    int j = 0;
    for (int k = 0; buf[k] != '\0'; k++) {
      if (buf[k] != 'u') {
        buf[j++] = buf[k];
      }
    }
    buf[j] = '\0';

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system(
    "gcc -std=c11 -Wall -Werror /tmp/.code.c -o /tmp/.expr"
    );
    if (ret != 0) {
     continue;
    }

    fp = popen("/tmp/.expr", "r");
    if (fp == NULL) {
      continue;
    }

    unsigned result;
    int count = fscanf(fp, "%u", &result);
    int status = pclose(fp);

    if (count != 1 || status != 0) {
     continue;
    }

    printf("%u %s\n", result, buf);
     }
     return 0;
}
