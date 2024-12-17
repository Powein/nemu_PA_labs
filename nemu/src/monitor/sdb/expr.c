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
  // higher means higher op priority
  TK_NOTYPE = 256, TK_EQ = 254, TK_SUB = 253, 
  TK_ADD = 252, TK_MUL = 251, TK_DIV = 250, 
  TK_DECIMAL = 249, TK_LEFT_P = 248, TK_RIGHT_P = 247,
  TK_AND = 246, TK_OR = 245, TK_HEX = 244, TK_NEQ = 243,
  TK_NOT = 240, TK_DEREF = 239, TK_STAR = 238,
  TK_REGISTER = 237,
  /* TODO: Add more token types */
};

typedef struct token {
  int type;
  char str[32];
} Token;

word_t get_priority(Token token) {
  /* Get the priority of the operator.
   * The higher the priority, the lower the number.
   * For now, we only have two priorities: 0 and 1.
   * 0 is for + and -.
   * 1 is for * and /.
   * -1 is for default and extras.
   */
  word_t pri = 0;
  word_t token_type = token.type;
  switch (token_type) {
    // follow c language priority
    case TK_AND: {
      pri = 11;
      break;
    }
    case TK_OR: {
      pri = 12;
      break;
    }
    case TK_DEREF :{
      pri = 2;
      break;
    }
    case TK_MUL: {
      pri = 3;
      break;
    }
    case TK_DIV: {
      pri = 3;
      break;
    }
    case TK_ADD: {
      pri = 4;
      break;
    }
    case TK_SUB: {
      pri = 4;
      break;
    }
    case TK_EQ: {
      pri = 7;
      break;
    }
    case TK_NEQ: {
      pri = 7;
      break;
    }
    default: {
      pri = 0;
    }
  }
  Log("Priority of %s is %d", token.str, pri);
  return pri;
}



// static word_t ret_value = 0;

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
  {"(0x)[0-9,a-f,A-F]+", TK_HEX}, //hex number
  {"\\+", TK_ADD},         // plus
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ}, // unequal
  {"!", TK_NOT},
  {"\\&\\&", TK_AND}, // and
  {"\\|\\|", TK_OR}, // or
  {"\\-", TK_SUB}, 
  {"\\/", TK_DIV},
  {"\\*", TK_STAR},
  {"[0-9]+", TK_DECIMAL}, 
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

// // check if close parenthesis is matched
// static bool is_matched() {
//   int i = 0;
//   int cnt = 0;
//   while (tokens[i] != '\0') {
//     if (str[i] == '(') {
//       cnt++;
//     } else if (str[i] == ')') {
//       cnt--;
//     }
//     i++;
//   }
//   return cnt == 0;
// }

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
          case TK_STAR: {
            // TODO: make dereference here
            // if is the first token, make it deref
            // if its former is not a number, make it deref
            if (nr_token == 0 || (tokens[nr_token - 1].type != TK_DECIMAL &&
            tokens[nr_token - 1].type != TK_HEX)) {
                Token newToken = {.type = TK_DEREF, .str = ""};
                strncpy(newToken.str, substr_start, substr_len);
                tokens[nr_token] = newToken;
                nr_token = nr_token + 1;
                break;// engouh
              }
            Token newToken = {.type = TK_MUL, .str = ""};
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
          case TK_HEX: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_NEQ: {
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
          case TK_NOT: {
            panic("Not implemented");
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_AND: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_OR: {
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          case TK_REGISTER :{
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

word_t eval(word_t p, word_t q);
word_t expr(char *e, bool *success) {
  // init the return val


  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  Log("Successfully make token, now evaluating");
  return eval((word_t) 0, (word_t) (nr_token - 1));
  // may check the validity of the tokens here
  
  /* TODO: Insert codes to evaluate the expression. */
  // evaluate that thing and return it
  // TODO();

  // return 0;
}

bool check_parentheses(word_t p, word_t q) {
  /* Check if the expression is surrounded by a matched pair of parentheses.
   * If that is the case, return true.
   * Otherwise, return false.
   */
  word_t i;
  int32_t left_count = 0;
  int32_t last_status = left_count;
  word_t become_zero_cnt = 0;
  for (i = p; i <= q; i++) {
    if(left_count < 0) {
      panic("Parentheses are not matched");
    }
    if (tokens[i].type == TK_LEFT_P) {
      left_count ++;
    } else if (tokens[i].type == TK_RIGHT_P) {
      left_count --;
    }
    if (last_status > 0 && left_count == 0) {
      become_zero_cnt ++;
    }
    last_status = left_count;
  }


  if (become_zero_cnt == 1 && left_count == 0 
  && tokens[p].type == TK_LEFT_P && tokens[q].type == TK_RIGHT_P) {
    return true;
  }
  else {
    return false;
  }
}

bool check_single_operator(word_t p,word_t q) {
  /* Check if the expression is a single operator.
   * If that is the case, return true.
   * Otherwise, return false.
   */
  if (tokens[p].type == TK_DEREF) {
    return true;
  } else return false;
}





word_t eval(word_t p, word_t q) {
  /* Get the evaluation of the expression from p to q.
   * q is included.
  */
  int i = 0;
  if (tokens[p].type == TK_NOTYPE) {
    return eval(p + 1, q);
  }
  if (tokens[q].type == TK_NOTYPE) {
    return eval(p, q - 1);
  }
  if (p > q) {
    Log("Invalid expression provided: left is greater than right");
    panic("Internal Wrong, Can not get the evaluation of the expression!");
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    wchar_t r = 0;
    // decimal
    switch (tokens[p].type) {
      case TK_DECIMAL: {
        r = strtoul(tokens[p].str, NULL, 10);
        break;
      }
      case TK_HEX: {
        r = strtoul(tokens[p].str, NULL, 16);
        break;
      }
      case TK_REGISTER: {
        bool* suc = malloc(sizeof(bool));
        r = isa_reg_str2val(tokens[p].str, suc);
        if (!*suc) panic("fail to find that register");
        break;
      }
      default: {
        panic("Internal Wrong, Can not get the evaluation of the expression!");
      }
    };
    return r;
    // hex
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
      */
    return eval(p + 1, q - 1);
  } else if (check_single_operator(p, q) == true){
    // find the single-operator and do something for them
    switch (tokens[p].type){
    case TK_DEREF:{
      // get right value
      word_t rval = eval(p + 1, q);
      // derefrence
      if (rval == 0 || (uintptr_t)rval % sizeof(word_t) != 0) {
          panic("Invalid memory address for dereference");
      }
      Log("Derefrencing address 0x%x\n", rval);
      return *((word_t*)(uintptr_t)rval);
      break;
    }
    default: {
      panic("NO SUCH OPREATOR! Something may went wrong with check single operator");
      break;
    }
    }
  }
  else {
    /* We should do more things here. */

    // get the master operator, which is the one with the lowest priority

    word_t lowest_priority = 0;
    word_t master_position = -1;
    // get the first lowest priority operator
    word_t flag = 0;
    for (i = p; i < q + 1; i++)
    {
      if(tokens[i].type == TK_NOTYPE) {
        continue;
      }
      if(tokens[i].type == TK_LEFT_P) {
        flag = flag + 1;
        continue;
      }
      if(tokens[i].type == TK_RIGHT_P) {
        flag = flag - 1;
        continue;
      }
      if(flag > 0) {
        continue;
      }
      word_t current_priority = get_priority(tokens[i]);
      if(lowest_priority < current_priority) {
        lowest_priority = current_priority;
        master_position = i;
      }
    }
    Log("Master operator found at %d : %s", master_position, tokens[master_position].str);
    // leftpart and rightpart, eval them
    word_t left_half_val = eval(p, master_position - 1);
    word_t right_half_val = eval(master_position + 1, q);
    switch (tokens[master_position].type) {
      case TK_ADD:{
        Log("Opreator found %s", tokens[master_position].str);
        return left_half_val + right_half_val;
      };
      case TK_SUB:{
        Log("Opreator found %s", tokens[master_position].str);
        return left_half_val - right_half_val;
      }
      case TK_MUL:{
        Log("Opreator found %s", tokens[master_position].str);
        return left_half_val * right_half_val;
      }
      case TK_DIV:{
        Log("Opreator found %s", tokens[master_position].str);
        if (right_half_val == 0) {
          panic("Division by zero");
        }
        return left_half_val / right_half_val;
      }
      case TK_AND: {
        Log("Opreator found %s", tokens[master_position].str);
        return left_half_val && right_half_val;
      }
      case TK_OR: {
        Log("Opreator found %s", tokens[master_position].str);
        return left_half_val || right_half_val;
      }
      case TK_EQ: {
        Log("Opreator found %s", tokens[master_position].str);
        return left_half_val == right_half_val;
      }
      case TK_NEQ: {
        Log("Opreator found %s", tokens[master_position].str);
        return left_half_val != right_half_val;
      }
      default:
        Log("Unrecognized operator %s", tokens[master_position].str);
    }
  }
  panic("Unexpected error in eval");
  return -1;
}

