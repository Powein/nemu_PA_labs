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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(myemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);// = ffffffff;
  return 0;
}


static int cmd_q(char *args) {
  // panic("Fuck you!");
  return -1;
}

static int cmd_p(char *args) {
  bool suc = true;
  bool *success = & suc;
  // word_t is just like uint (4 bytes)
  word_t val = expr(args, success);
  printf("%d\n", val);
  return 0;
}

static int cmd_w(char *args) {
  // panic("Not implemnted");
  Warn("Working in progress");
  bool suc = true;
  bool *success = & suc;
  // word_t is just like uint (4 bytes)
  expr(args, success);
  if(!*success) {
    printf("Invalid expression. Watchpoint is not added.\n");
    return 0;
  }
  // printf("%d\n", val);
  char* x = args;
  Log("Expression: %s", x);
  WP* newWp = new_wp(args, success);
  // a->expr = "i love you";
  if(!*success) {
      printf("Failed to create watchpoint\n");
      return 0;
  }
  printf("New watchpoint %d created\n", newWp->NO);
  return 0;
}

static int cmd_si(char *args) {
  if (args == NULL) {
    cpu_exec(1);
  } else {
    int step = atoi(args);
    cpu_exec(step);
  }
  return 0;
}

static int cmd_info(char *args) {
  if (strcmp(args, "r") == 0) {
    // Log("command : info r");
    isa_reg_display();
  } else if (strcmp(args, "w") == 0) {
    watchpoint_display();
    // panic("Not implemented");
  } else {
    printf("Usage: info r for reg, w for watchpoints\n");
  }
  return 0;
}

#define BYTE_PER_ROW 16
//word_t paddr_read(paddr_t addr, int len);
static int cmd_x(char *args) {
  char* arg_bc = strtok(NULL, " ");
  char* arg_ad = strtok(NULL, " ");
  bool* success = malloc(sizeof(bool));
  *success = true;
  int ct = 0;
  if (arg_bc == NULL || arg_ad == NULL) {
    printf("Usage: x <byte_count(expression)> <address(hex)>\n");
    return 0;
  };
  Log("Translating expression to integers.");
  word_t byte_count = expr(arg_bc, success);
  if(!*success) {
    printf("Invalid expression for byte count\n");
    return 0;
  }
  // if(!*success) {
  //   printf("Invalid expression for byte count");
  // }
  // Log("The address is %s", arg_ad);
  // word_t address = expr(arg_ad, success);
  // word_t address = strtol(arg_ad, NULL, 0);
  word_t address = expr(arg_ad, success);
  if(!*success) {
    printf("Invalid expression for address\n");
    return 0;
  }
  free(success);
  // Log("Get the args");
  if (address < 0x80000000 || address + byte_count >= 0x87ffffff) {
    printf("Invalid address. Use effective addr: [0x80000000, 0x87ffffff]\n");
    return 0;
  }
  printf("XXXXXXXX");
  for (word_t i = 0; i < BYTE_PER_ROW; i++)
  {
    printf(" %02x", i);
  }
  
  for (int i = 0; i < byte_count; i++) {
    word_t t = paddr_read(address + i, 1);
    if (ct % BYTE_PER_ROW == 0) {
      printf("\n");
      printf("%x", address + i);
    }
    printf(" ");
    printf("%02x", t);
    ct++;
  }
  printf("\n");
  return 0;
}

static int cmd_go(char *args) {
  // test anything
  // init_wp_pool();
  bool* success = malloc(sizeof(bool));
  *success = true;
  int count = 0;
  while(true){
    *success = true;
    char* x = "aaa";
    WP* a = new_wp(x, success);
    Log("Created watchpoints %d", count);
    Log("address of the watchpoint is %p",a);
    // a->expr = "i love you";
    count++;
    if(!*success) {
        printf("Failed to create watchpoint\n");
        break;
      }
  }
  return 0;
}

static int cmd_d(char* args) {
  bool suc = true;
  word_t deleteNum = atoi(args);
  free_wp(deleteNum, &suc);
  if(!suc) {
    printf("Watchpoint %d does not exists\n", deleteNum);
    return 0;
  }
  printf("Watchpoint %d deleted\n", deleteNum);
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "p", "Print the value of the expression", cmd_p },
  { "w", "Watch the value of the expression, and break when it changes", cmd_w},
  { "si", "Take a single step of the program", cmd_si},
  { "info", "Display the status of the program", cmd_info},
  { "x", "Examine memory", cmd_x},
  { "w", "Create a watchpoint", cmd_w},
  { "go", "test", cmd_go},
  { "d", "Delete a watchpoint by watchpoint number", cmd_d},
  // { "d", "Delete a watchpoint", cmd_d},

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)
static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  // for test
  // generate_some_pointers();
  // init_wp_pool();
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
