#include <common.h>

#define MAX_IRINGBUFFER_SIZE 16
typedef struct {
  word_t pc;
  uint32_t inst;
} IringNode;
typedef struct {
  int p_cur;    // default 0
  bool is_full; // default false
  IringNode buffer[MAX_IRINGBUFFER_SIZE];
} IringBuffer;
IringBuffer iringBuffer;
void trace_inst(word_t pc, uint32_t inst) {
  iringBuffer.buffer[iringBuffer.p_cur].pc = pc;
  iringBuffer.buffer[iringBuffer.p_cur].inst = inst;
  iringBuffer.p_cur = (iringBuffer.p_cur + 1) % MAX_IRINGBUFFER_SIZE;
  if (iringBuffer.p_cur == 0) {
    iringBuffer.is_full = true;
  }
}
void display_inst() {
  if (!iringBuffer.is_full && iringBuffer.p_cur == 0) {
    return;
  }
  int end = iringBuffer.p_cur;
  int i = iringBuffer.is_full ? iringBuffer.p_cur : 0;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

  char buf[128];
  char *p;
  do {
    p = buf;
    p += sprintf(p, "%s" FMT_WORD ":%8x",
                 (i + 1) % MAX_IRINGBUFFER_SIZE == end ? " --> " : "     ",
                 iringBuffer.buffer[i].pc, iringBuffer.buffer[i].inst);
    if ((i + 1) % MAX_IRINGBUFFER_SIZE == end) {
      printf(ANSI_FG_RED);
    }
    puts(buf);
  } while (i = (i + 1) % MAX_IRINGBUFFER_SIZE, i != end);
  printf(ANSI_NONE);
}

void display_mem_addr(word_t addr, int len) {
  printf("addr = " FMT_WORD ", len = %d\n", addr, len);
}