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
#include <memory/paddr.h>

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
   * Follows the priority of C language operators.
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
      pri = 1;
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
  {"\\$(\\$0|ra|sp|gp|tp|t0|t1|t2|s0|s1|a0|a1|a2|a3|a4|a5|a6|a7|s2|s3|s4|s5|s6|s7|s8|s9|s10|s11|t3|t4|t5|t6)", TK_REGISTER},
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


// max length for tokens array
// TODO: try to use this definition everywhere
#define MAX_TOKENS 32
#define TK_LEN 32
static Token tokens[32] __attribute__((used)) = {}; // make gcc happy
static int nr_token __attribute__((used))  = 0;

static void push_token(char *substr_start, int substr_len, int index) {
  Token newToken = {.type = rules[index].token_type, .str = ""};
  strncpy(newToken.str, substr_start, substr_len);
  tokens[nr_token] = newToken;
  nr_token = nr_token + 1;
  return;
}



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
          case TK_STAR: {
            // if is the first token, make it deref
            // if its former is not a number/ notype, make it deref
            if (nr_token == 0 || (tokens[nr_token - 1].type != TK_DECIMAL &&
            tokens[nr_token - 1].type != TK_HEX && tokens[nr_token - 1].type != TK_NOTYPE)) {
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
          case TK_NOT: {
            Log("Not implemented");
            return false;
            Token newToken = {.type = rules[i].token_type, .str = ""};
            strncpy(newToken.str, substr_start, substr_len);
            tokens[nr_token] = newToken;
            nr_token = nr_token + 1;
            break;
          }
          default: 
            push_token(substr_start, substr_len, i);
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

word_t eval(word_t p, word_t q, bool *success);
word_t expr(char *e, bool *success) {
  // init the return val
  if(*success == false){
    Warn("Passing a false value into expr!");
    return 0;
  }

  if (!make_token(e)) {
    Warn("Fail to get the expression value. Returning 0");
    *success = false;
    return 0;
  }
  Log("Successfully make token, now evaluating");
  word_t value = eval((word_t) 0, (word_t) (nr_token - 1), success);
  if(!*success) {
    Warn("Fail to get the expression value. Returning 0");
  }
  return value;
  // may check the validity of the tokens here
  

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
      Warn("Parentheses are not matched");
      return false;
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

static word_t deal_double_operator(word_t left_half_val, 
  word_t right_half_val, word_t master_position, bool *success);

static word_t deal_single_operator(word_t master_position, word_t rval, bool *success);

static word_t deal_orpand(word_t p, bool*success);

static word_t find_master_operator(word_t p, word_t q, bool *success);

word_t eval(word_t p, word_t q, bool* success) {
  /* Get the evaluation of the expression from p to q.
   * q is included.*/
  if (*success == false) {
    return 0;
  };
  if (tokens[p].type == TK_NOTYPE) {
    return eval(p + 1, q, success);
  }
  if (tokens[q].type == TK_NOTYPE) {
    return eval(p, q - 1, success);
  }
  if (p > q) {
    Log("Invalid expression provided: left is greater than right");
    Warn("Invalid expression provided!");
    *success = false;
    return 0;
  }
  else if (p == q) {
    // It is a single token. number/ register
    word_t value = deal_orpand(p, success);
    if(!(*success)) return 0;
    return value;
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
      */
    Log("remove parentheses at %u-%u", p, q);
    return eval(p + 1, q - 1, success);
  } 
  // get the master operator, which is the one with the lowest priority

  word_t master_position = find_master_operator(p, q, success);
  if (!(*success)) return 0;

  Log("Master operator found at %d : %s", master_position, tokens[master_position].str);

  if (check_single_operator(master_position, -1)) {
    word_t rval = eval(p + 1, q, success);
    word_t value = deal_single_operator(rval, master_position, success);
    if(!(*success)) return 0;
    return value;
  }

  word_t left_half_val = eval(p, master_position - 1, success);
  word_t right_half_val = eval(master_position + 1, q, success);
  word_t value = deal_double_operator(left_half_val, 
    right_half_val, master_position, success);
  if(!(*success)) return 0;
  return value;
  Warn("Not a recognized double operator!");
  *success = false;
  return 0;
}

static word_t find_master_operator(word_t p, word_t q, bool *success) {
  int i = 0;
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
  if (master_position == -1) {
    Warn("No master operator found!");
    *success = false;
    return 0;
  }
  return master_position;
}

static word_t deal_orpand(word_t p, bool*success){
  word_t r = 0;
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
      r = isa_reg_str2val(tokens[p].str, success);
      if(!(*success)) return 0;
      break;
    }
    default: {
      // panic("Internal Wrong, Can not get the evaluation of the expression!");
      Warn("Not recognized token type when p == q. May add more numeric cases.");
      *success = false;
      return 0;
    }
  }
  return r;
}



static word_t deal_single_operator(word_t rval, word_t master_position, bool *success) {
  word_t p = master_position;
  switch (tokens[p].type){
    case TK_DEREF:{
      // get right value
      // derefrence
      // if (rval == 0 || (uintptr_t)rval % sizeof(word_t) != 0) {
      //     panic("Invalid memory address for dereference");
      // }
      if (rval < 0x80000000 || rval > 0x87ffffff) {
        Warn("Not a effective address.");
        printf("Invalid address. Use effective addr: [0x80000000, 0x87ffffff]\n");
        *(success) = false;
        return 0;
      }
      Log("Derefrencing address 0x%x\n", rval);
      return paddr_read(rval, 1);
      break;
    }
    default: {
      Warn("NO SUCH OPREATOR! Something may went wrong with check single operator");
      *success = false;
      return 0;
      break;
    }
  }
}

static word_t deal_double_operator(word_t left_half_val, 
  word_t right_half_val, word_t master_position, bool *success) {

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
      *success = false;
  }
  return 0;
}