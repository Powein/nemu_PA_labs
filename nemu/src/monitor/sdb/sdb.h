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

#ifndef __SDB_H__
#define __SDB_H__
#define MAX_EXPR_LEN 64
#include <common.h>

// See expr.c. token are used to store the tokens of the expression.
// Saving token will make it quicker to evaluate the expression.
// may do this later
// we have not hit the limit yet
// remember to make eval accecpt tokens as a parameter when we do that optimization

// typedef struct token {
//   int type;
//   char str[32];
// } Token;

// get a value from a string expression
word_t expr(char *e, bool *success);
// word_t eval;
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expr[MAX_EXPR_LEN];
  bool vacant;
  word_t last_value; // the last value of the expression
  /* TODO: Add more members if necessary */

} WP;
WP* new_wp(char* e ,bool* success);
void watchpoint_display();
void free_wp(int wpNO, bool* success);
void check_watchpoint();
void toggle_wp(bool target_status);
#endif
