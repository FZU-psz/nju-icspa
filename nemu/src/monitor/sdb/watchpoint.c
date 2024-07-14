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

#include "sdb.h"
#include "utils.h"
#include <string.h>

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expr[256];
  int val;
  /* TODO: Add more members if necessary */
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(){
  if(free_==NULL){
    printf("No enough watchpoints!\n");
    assert(0);
  }
  WP* new=free_;
  free_=free_->next;
  new->next=head;
  head=new;
  return new;
}
void free_wp(WP *wp){
  WP* p=head;
  if(p==wp){
    head=head->next;
    wp->next=free_;
    free_=wp;
    return;
  }
  while(p->next!=NULL){
    if(p->next==wp){
      p->next=wp->next;
      wp->next=free_;
      free_=wp;
      return;
    }
    p=p->next;
  }
  printf("No such watchpoint!\n");
  assert(0);
}
void scan_wp(){
  
  char expr_buffer[256];

  WP* p=head;
  bool flag = false;
  while(p!=NULL){
    bool success=true;
    strncpy(expr_buffer, p->expr, 256);
    word_t val = expr(expr_buffer,&success);
    if(val != p->val){//值发生了变化
    printf("Old value %d; New value %d\n",p->val,val);
      flag = true;
      p->val = val;
    }
    p = p->next;
  }
  if(flag){
      nemu_state.state =NEMU_STOP;
      printf("The value of Watchpoints changed!\n");
  }
}