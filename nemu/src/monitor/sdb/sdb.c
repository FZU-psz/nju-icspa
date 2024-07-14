/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "sdb.h"
#include "common.h"
#include <cpu/cpu.h>
#include <isa.h>
#include <memory/vaddr.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <string.h>

static int is_batch_mode = false;
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expr[256];
  int val;
  /* TODO: Add more members if necessary */
} WP;

extern WP *new_wp();
extern void free_wp(WP *wp);
void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin.
 */
static char *rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) { return -1; }

static int cmd_help(char *args);

// 默认执行1步，如果有参数则执行参数指定的步数n
static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    cpu_exec(1);
  } else {
    // atoi 将字符串转换为整数
    int n = atoi(arg);
    cpu_exec(n);
  }
  return 0;
}
static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Please input the argument\n");
    return 0;
  }
  if (strcmp(arg, "r") == 0) {
    // 打印寄存器的值
    isa_reg_display();
  } else if (strcmp(arg, "w") == 0) {
    // 打印监视点的信息
    //  TODO: Print watchpoint info
    printf("No watchpoint\n");
  } else {
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_p(char *args) {
  bool success = true;
  word_t result = expr(args, &success);
  if (success) {
    printf("0x%08x\n", result);
  }
  return 0;
}
static int cmd_x(char *args) {
  char *arg1 = strtok(NULL, " ");
  char *arg2 = strtok(NULL, " ");
  if (arg1 == NULL || arg2 == NULL) {
    printf("Please input the argument\n");
    return 0;
  }
  int n = atoi(arg1);
  // char * hex_str = arg2;
  // vaddr_t vaddr_start = strtol(hex_str,NULL,16);
  // for(int i=0;i<n;i++){
  //   if(i%4==0){
  //     printf("0x%08x: ",vaddr_start+i);
  //   }
  //   printf("0x%02x ",vaddr_read(vaddr_start+i ,1));
  //   if((i+1)%4==0){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  bool success = true;
  word_t addr = expr(arg2, &success);
  if (success) {
    for (int i = 0; i < n; i++) {
      if (i % 4 == 0) {
        printf("0x%08x: ", addr + i);
      }
      vaddr_t vaddr = addr + i;
      printf("0x%02x ", vaddr_read(vaddr, 1));
      if ((i + 1) % 4 == 0) {
        printf("\n");
      }
    }
  }
  return 0;
}
static int cmd_w(char *args) {
  char buffer[256];
  WP *new = new_wp();
  bool success = true;
  strncpy(new->expr, args, 256);
  strncpy(buffer, args, 256);
  new->val = expr(buffer, &success);
  if (success) {
    printf("Watchpoint %d: %s\n", new->NO, args);
  }
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},

    /* TODO: Add more commands */
    [3] = {"si", "si N: Execute N step", cmd_si},
    [4] = {"info",
           "info r: Printf the info about register\ninfo w: Printf the info "
           "about watch point",
           cmd_info},
    [5] = {"p", "p EXPR: Calculate the value of the expression", cmd_p},
    [6] = {"x", "x N EXPR: Scan memory from expr", cmd_x},
    [7] = {"w", "w EXPR: Watch the value of EXPR", cmd_w},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  } else {
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() { is_batch_mode = true; }

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL;) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) {
      continue;
    }

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
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD) {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
