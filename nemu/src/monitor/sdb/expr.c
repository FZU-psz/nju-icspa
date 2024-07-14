/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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

#include "common.h"
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include<memory/vaddr.h>

enum {
  TK_NOTYPE = 256, TK_EQ,
  /* TODO: Add more token types */
  TK_NUM,TK_VAR,TK_AND,TK_NOTEQ,TK_HEXNUM,TK_REG,DEREF
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
  {"==", TK_EQ},        // equal
  [3] = {"\\-", '-'},   // minus
  [4] = {"\\*", '*'},   // multiply
  [5] = {"\\/", '/'},   // divide
  [6] = {"\\(", '('},   // left bracket
  [7] = {"\\)", ')'},   // right bracket
  [8] = {"[0-9]+", TK_NUM}, // number
  [9] = {"\\$[a-zA-Z_][a-zA-Z0-9_]*", TK_REG}, // register
  [10] = {"&&", TK_AND}, // and
  [11] = {"!=", TK_NOTEQ}, // not equal
  [12] = {"0x[0-9a-fA-F]+", TK_HEXNUM}, // hex number
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

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        tokens[nr_token].type = rules[i].token_type;
        strncpy(tokens[nr_token].str, substr_start, substr_len);
        nr_token++;
        // switch (rules[i].token_type) {
        //   default: TODO();
        // }
        // printf("str=%s\n",tokens[nr_token-1].str);

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
bool check_parentheses(int p,int q){
  int i;
  int cnt=0;

  if(tokens[p].type!='('||tokens[q].type!=')'){//如果整个表达不是被括号包围的
    return false;
  }
  for(i=p+1;i<=q-1;i++){
    if(tokens[i].type=='('){
      cnt++;
    }
    else if(tokens[i].type==')'){
      cnt--;
    }
    //如果左括号的数量比右括号的数量多,则不合法
    if(cnt==0&&i!=q-1){
      return false;
    }
  }
  return true;
}
int find_op(int p,int q,int  *op_type){
  int i;
  int op_pos=-1;
  int op_level=0;
  for(i=q;i>=p;i--){
    if(tokens[i].type==TK_NUM || tokens[i].type==TK_HEXNUM || tokens[i].type==TK_REG){
      continue;
    }
    else if(tokens[i].type==')'){
      for(;i>=p;i--){
        if(tokens[i].type=='('){
          break;
        }
      }
    }
    else if(tokens[i].type==TK_EQ||tokens[i].type==TK_NOTEQ||tokens[i].type==TK_AND){
      if(op_level<=3){
        op_level=3;
        op_pos=i;
        *op_type=tokens[i].type;
      }
    }
    else if(tokens[i].type=='+'||tokens[i].type=='-'){
      if(op_level<=2){
        op_level=2;
        op_pos=i;
        *op_type=tokens[i].type;
      }
    }
    else if(tokens[i].type=='*'||tokens[i].type=='/'){
      if(op_level<=1){
        op_level=1;
        op_pos=i;
        *op_type=tokens[i].type;
      }
    }
    else if(tokens[i].type == DEREF){
      if(op_level<=0){
        op_level=0;
        op_pos=i;
        *op_type=tokens[i].type;
      }
    }
  }
  
  return op_pos;
}
word_t eval(int p,int q){
  if(p>q){
    return 0;
  }
  else if(p==q){
    if(tokens[p].type==TK_NUM){
      return atoi(tokens[p].str);
    }
    else if(tokens[p].type==TK_HEXNUM){
      return strtol(tokens[p].str,NULL,16);
    }
    else if(tokens[p].type==TK_REG){
      bool success = false;
      word_t result = isa_reg_str2val(tokens[p].str+1, &success);
      if(success){
        return result;
      }
      else{
        printf("Register not found\n");
        return 0;
      }
    }
    else{
      Assert(0,"Invalid expression");
    }
  }
  else if(check_parentheses(p,q)==true){
    //括号的数量能对上,但是不一定是合法的
    return eval(p+1,q-1);
  }
  else {
   //找出主运算符号
    int  op_type;
    int op_pos= find_op(p,q, &op_type);

    switch(op_type){
      case '+': return eval(p,op_pos-1)+eval(op_pos+1,q);
      case '-': return eval(p,op_pos-1)-eval(op_pos+1,q);
      case '*': return eval(p,op_pos-1)*eval(op_pos+1,q);
      case '/': return eval(p,op_pos-1)/eval(op_pos+1,q);
      case TK_EQ: return eval(p,op_pos-1)==eval(op_pos+1,q);
      case TK_NOTEQ: return eval(p,op_pos-1)!=eval(op_pos+1,q);
      case TK_AND: return eval(p,op_pos-1)&&eval(op_pos+1,q);
      case DEREF: return vaddr_read(eval(op_pos+1,q),4);
      
    }
  }
  return 0;
}
word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  for (int i = 0; i < nr_token; i ++) {
  if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_NUM && tokens[i-1].type!=TK_HEXNUM && tokens[i-1].type != ')')) ) {
    tokens[i].type = DEREF;
  }
}
  word_t result = eval(0,nr_token-1);
  return result;
}
