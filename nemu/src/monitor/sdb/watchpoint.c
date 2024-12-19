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

// typedef struct watchpoint {
//   int NO;
//   struct watchpoint *next;
  
//   /* TODO: Add more members if necessary */

// } WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  if(head != NULL) return;
  Log("Initializing watchpoint pool.");
  int i;
  wp_pool[0].NO = 0;
  wp_pool[0].next = &wp_pool[1];
  wp_pool[0].vacant = true;
  head = wp_pool;
  for (i = 1; i < NR_WP; i ++) { 
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].vacant = true;
    Log("Initializing watchpoint %d, vacant = %d", i, wp_pool[i].vacant);
  }
  free_ = wp_pool; // set free to the first element
  Log("Watchpoint pool initialized. free_ = %p, head = %p", free_, head);
}

/* TODO: Implement the functionality of watchpoint */
static void info_wp(WP* wp) {
  // Log("=+=+=+=+=");
  // Log("Watchpoint NO: %d", wp->NO);
  // Log("Watchpoint value: %s", wp->expr);
  // Log("Watchpoint vacant: %d", wp->vacant);
  // Log("=+=+=+=+=");
}

WP* new_wp(char* e ,bool* success) {
  Log("Adding new watch point");
  for (word_t i = 0; i < NR_WP + 1; i++)
  {
    // Log("Checking wp at %d", i);
    info_wp(&wp_pool[i]);
    if(free_-> vacant == true) {
      Log("Find a vacant wp in pool: %d", free_->NO);
      free_ -> expr = e;
      free_-> vacant = false;
      *success = true;
      return free_;
    }
    if(free_->next != NULL) {
      free_ = free_->next;
    } else {
      free_ = wp_pool;
    }
  }
  *success = false;
  return NULL;
}

void free_wp(WP *wp) {
  Log("Freeing watch point %d", wp->NO);
  wp->vacant = true;
  free_ = wp;
  return;
}

void watchpoint_display() {
  for (word_t i = 0; i < NR_WP; i++) {
    printf("Watchpoint %d, tracking expr: %s\n", i, wp_pool[i].expr);
  }
  return;
}