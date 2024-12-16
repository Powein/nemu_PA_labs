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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
// encode for tokens
enum {
  TK_NOTYPE = 256, TK_EQ = 254, TK_SUB = 253, 
  TK_ADD = 252, TK_MUL = 251, TK_DIV = 250, 
  TK_DECIMAL = 249, TK_LEFT_P = 248, TK_RIGHT_P = 247
  /* TODO: Add more token types */
};

// regex matching rules for tokens
static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  // former means higher priority
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_ADD},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", TK_SUB}, // regex - token type
  {"\\/", TK_DIV},
  {"\\*", TK_MUL},
  {"[0-9]+", TK_DECIMAL}, // regex - token type
  {"\\(", TK_LEFT_P},// (
  {"\\)", TK_RIGHT_P},// )
};
// len(rules)
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
    // usage: int regcomp(regex_t *restrict preg, const char *restrict regex,int cflags);
    // this will compile the regex and store it in the re[i]
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

// max length for tokens array
// TODO: try to use this definition everywhere
#define MAX_TOKENS 32
#define TK_LEN 32
static Token tokens[32] __attribute__((used)) = {}; // make gcc happy
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  // this makes token from the input string `e`
  int position = 0;// the position of the current character in the string
  int i;// sth like c99 for loop
  regmatch_t pmatch;

  nr_token = 0; // stack top position

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        // matched successfully
        char *substr_start = e + position; // where substr starts
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        
        position += substr_len;// skip to go to next token

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        assert(nr_token < MAX_TOKENS);
        switch (rules[i].token_type) {
          case TK_ADD: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          };
          case TK_SUB: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          };
          case TK_MUL: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_DIV: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_DECIMAL: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_EQ: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_NOTYPE: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_LEFT_P: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_RIGHT_P: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          default: 
            Log("default"); // panic
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
