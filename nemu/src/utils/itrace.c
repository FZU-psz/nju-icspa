#include <common.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
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


// --------------------------ftrace-----------------------------------------
// reference: https://git.lug.ustc.edu.cn/nos-ae/pa-tmp/-/blob/main/itrace.c 

typedef struct {
	char name[32]; // func name, 32 should be enough
	paddr_t addr;
	unsigned char info;
	Elf64_Xword size;
} SymEntry;

SymEntry *symbol_tbl = NULL; // dynamic allocated
int symbol_tbl_size = 0;
int call_depth = 0;

typedef struct tail_rec_node {
	paddr_t pc;
	paddr_t depend;
	struct tail_rec_node *next;
} TailRecNode;
TailRecNode *tail_rec_head = NULL; // linklist with head, dynamic allocated

static void read_elf_header(int fd, Elf64_Ehdr *eh) {
	assert(lseek(fd, 0, SEEK_SET) == 0);
  assert(read(fd, (void *)eh, sizeof(Elf64_Ehdr)) == sizeof(Elf64_Ehdr));

	  // check if is elf using fixed format of Magic: 7f 45 4c 46 ...
  if(strncmp((char*)eh->e_ident, "\177ELF", 4)) {
		panic("malformed ELF file");
	}
}

static void display_elf_hedaer(Elf64_Ehdr eh) {
	/* Storage capacity class */
	log_write("Storage class\t= ");
	switch(eh.e_ident[EI_CLASS])
	{
		case ELFCLASS32:
			log_write("32-bit objects\n");
			break;

		case ELFCLASS64:
			log_write("64-bit objects\n");
			break;

		default:
			log_write("INVALID CLASS\n");
			break;
	}

	/* Data Format */
	log_write("Data format\t= ");
	switch(eh.e_ident[EI_DATA])
	{
		case ELFDATA2LSB:
			log_write("2's complement, little endian\n");
			break;

		case ELFDATA2MSB:
			log_write("2's complement, big endian\n");
			break;

		default:
			log_write("INVALID Format\n");
			break;
	}

	/* OS ABI */
	log_write("OS ABI\t\t= ");
	switch(eh.e_ident[EI_OSABI])
	{
		case ELFOSABI_SYSV:
			log_write("UNIX System V ABI\n");
			break;

		case ELFOSABI_HPUX:
			log_write("HP-UX\n");
			break;

		case ELFOSABI_NETBSD:
			log_write("NetBSD\n");
			break;

		case ELFOSABI_LINUX:
			log_write("Linux\n");
			break;

		case ELFOSABI_SOLARIS:
			log_write("Sun Solaris\n");
			break;

		case ELFOSABI_AIX:
			log_write("IBM AIX\n");
			break;

		case ELFOSABI_IRIX:
			log_write("SGI Irix\n");
			break;

		case ELFOSABI_FREEBSD:
			log_write("FreeBSD\n");
			break;

		case ELFOSABI_TRU64:
			log_write("Compaq TRU64 UNIX\n");
			break;

		case ELFOSABI_MODESTO:
			log_write("Novell Modesto\n");
			break;

		case ELFOSABI_OPENBSD:
			log_write("OpenBSD\n");
			break;

		case ELFOSABI_ARM_AEABI:
			log_write("ARM EABI\n");
			break;

		case ELFOSABI_ARM:
			log_write("ARM\n");
			break;

		case ELFOSABI_STANDALONE:
			log_write("Standalone (embedded) app\n");
			break;

		default:
			log_write("Unknown (0x%x)\n", eh.e_ident[EI_OSABI]);
			break;
	}

	/* ELF filetype */
	log_write("Filetype \t= ");
	switch(eh.e_type)
	{
		case ET_NONE:
			log_write("N/A (0x0)\n");
			break;

		case ET_REL:
			log_write("Relocatable\n");
			break;

		case ET_EXEC:
			log_write("Executable\n");
			break;

		case ET_DYN:
			log_write("Shared Object\n");
			break;
		default:
			log_write("Unknown (0x%x)\n", eh.e_type);
			break;
	}

	/* ELF Machine-id */
	log_write("Machine\t\t= ");
	switch(eh.e_machine)
	{
		case EM_NONE:
			log_write("None (0x0)\n");
			break;

		case EM_386:
			log_write("INTEL x86 (0x%x)\n", EM_386);
			break;

		case EM_X86_64:
			log_write("AMD x86_64 (0x%x)\n", EM_X86_64);
			break;

		case EM_AARCH64:
			log_write("AARCH64 (0x%x)\n", EM_AARCH64);
			break;

		default:
			log_write(" 0x%x\n", eh.e_machine);
			break;
	}

	/* Entry point */
	log_write("Entry point\t= 0x%08lx\n", eh.e_entry);

	/* ELF header size in bytes */
	log_write("ELF header size\t= 0x%08x\n", eh.e_ehsize);

	/* Program Header */
	log_write("Program Header\t= ");
	log_write("0x%08lx\n", eh.e_phoff);		/* start */
	log_write("\t\t  %d entries\n", eh.e_phnum);	/* num entry */
	log_write("\t\t  %d bytes\n", eh.e_phentsize);	/* size/entry */

	/* Section header starts at */
	log_write("Section Header\t= ");
	log_write("0x%08lx\n", eh.e_shoff);		/* start */
	log_write("\t\t  %d entries\n", eh.e_shnum);	/* num entry */
	log_write("\t\t  %d bytes\n", eh.e_shentsize);	/* size/entry */
	log_write("\t\t  0x%08x (string table offset)\n", eh.e_shstrndx);

	/* File flags (Machine specific)*/
	log_write("File flags \t= 0x%08x\n", eh.e_flags);

	/* ELF file flags are machine specific.
	 * INTEL implements NO flags.
	 * ARM implements a few.
	 * Add support below to parse ELF file flags on ARM
	 */
	int32_t ef = eh.e_flags;
	log_write("\t\t  ");

	if(ef & EF_ARM_RELEXEC)
		log_write(",RELEXEC ");

	if(ef & EF_ARM_HASENTRY)
		log_write(",HASENTRY ");

	if(ef & EF_ARM_INTERWORK)
		log_write(",INTERWORK ");

	if(ef & EF_ARM_APCS_26)
		log_write(",APCS_26 ");

	if(ef & EF_ARM_APCS_FLOAT)
		log_write(",APCS_FLOAT ");

	if(ef & EF_ARM_PIC)
		log_write(",PIC ");

	if(ef & EF_ARM_ALIGN8)
		log_write(",ALIGN8 ");

	if(ef & EF_ARM_NEW_ABI)
		log_write(",NEW_ABI ");

	if(ef & EF_ARM_OLD_ABI)
		log_write(",OLD_ABI ");

	if(ef & EF_ARM_SOFT_FLOAT)
		log_write(",SOFT_FLOAT ");

	if(ef & EF_ARM_VFP_FLOAT)
		log_write(",VFP_FLOAT ");

	if(ef & EF_ARM_MAVERICK_FLOAT)
		log_write(",MAVERICK_FLOAT ");

	log_write("\n");

	/* MSB of flags conatins ARM EABI version */
	log_write("ARM EABI\t= Version %d\n", (ef & EF_ARM_EABIMASK)>>24);

	log_write("\n");	/* End of ELF header */
}

static void read_section(int fd, Elf64_Shdr sh, void *dst) {
	assert(dst != NULL);
	assert(lseek(fd, (off_t)sh.sh_offset, SEEK_SET) == (off_t)sh.sh_offset);
	assert(read(fd, dst, sh.sh_size) == sh.sh_size);
}

static void read_section_headers(int fd, Elf64_Ehdr eh, Elf64_Shdr *sh_tbl) {
	assert(lseek(fd, eh.e_shoff, SEEK_SET) == eh.e_shoff);
	for(int i = 0; i < eh.e_shnum; i++) {
		assert(read(fd, (void *)&sh_tbl[i], eh.e_shentsize) == eh.e_shentsize);
	}
}

static void display_section_headers(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[]) {
  // warn: C99
	char sh_str[sh_tbl[eh.e_shstrndx].sh_size];
  read_section(fd, sh_tbl[eh.e_shstrndx], sh_str);
  
	/* Read section-header string-table */

	log_write("========================================");
	log_write("========================================\n");
	log_write(" idx offset     load-addr  size       algn"
			" flags      type       section\n");
	log_write("========================================");
	log_write("========================================\n");

	for(int i = 0; i < eh.e_shnum; i++) {
		log_write(" %03d ", i);
		log_write("0x%08lx ", sh_tbl[i].sh_offset);
		log_write("0x%08lx ", sh_tbl[i].sh_addr);
		log_write("0x%08lx ", sh_tbl[i].sh_size);
		log_write("%-4ld ", sh_tbl[i].sh_addralign);
		log_write("0x%08lx ", sh_tbl[i].sh_flags);
		log_write("0x%08x ", sh_tbl[i].sh_type);
		log_write("%s\t", (sh_str + sh_tbl[i].sh_name));
		log_write("\n");
	}
	log_write("========================================");
	log_write("========================================\n");
	log_write("\n");	/* end of section header table */
}

static void read_symbol_table(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[], int sym_idx) {
  Elf64_Sym sym_tbl[sh_tbl[sym_idx].sh_size];
  read_section(fd, sh_tbl[sym_idx], sym_tbl);
  
  int str_idx = sh_tbl[sym_idx].sh_link;
  char str_tbl[sh_tbl[str_idx].sh_size];
  read_section(fd, sh_tbl[str_idx], str_tbl);
  
  int sym_count = (sh_tbl[sym_idx].sh_size / sizeof(Elf64_Sym));
	// log
  log_write("Symbol count: %d\n", sym_count);
  log_write("====================================================\n");
  log_write(" num    value            type size       name\n");
  log_write("====================================================\n");
  for (int i = 0; i < sym_count; i++) {
    log_write(" %-3d    %016lx %-4d %-10ld %s\n",
      i,
      sym_tbl[i].st_value, 
      ELF64_ST_TYPE(sym_tbl[i].st_info),
			sym_tbl[i].st_size,
      str_tbl + sym_tbl[i].st_name
    );
  }
  log_write("====================================================\n\n");

	// read
	symbol_tbl_size = sym_count;
	symbol_tbl = malloc(sizeof(SymEntry) * sym_count);
  for (int i = 0; i < sym_count; i++) {
    symbol_tbl[i].addr = sym_tbl[i].st_value;
		symbol_tbl[i].info = sym_tbl[i].st_info;
		symbol_tbl[i].size = sym_tbl[i].st_size;
		memset(symbol_tbl[i].name, 0, 32);
		strncpy(symbol_tbl[i].name, str_tbl + sym_tbl[i].st_name, 31);
  }
}

static void read_symbols(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[]) {
  for (int i = 0; i < eh.e_shnum; i++) {
		switch (sh_tbl[i].sh_type) {
		case SHT_SYMTAB: case SHT_DYNSYM:
			read_symbol_table(fd, eh, sh_tbl, i); break;
		}
  }
}

static void init_tail_rec_list() {
	tail_rec_head = (TailRecNode *)malloc(sizeof(TailRecNode));
	tail_rec_head->pc = 0;
	tail_rec_head->next = NULL;
}

/* ELF64 as default */
void parse_elf(const char *elf_file) {
  if (elf_file == NULL) return;
  
	Log("specified ELF file: %s", elf_file);
  int fd = open(elf_file, O_RDONLY|O_SYNC);
  Assert(fd >= 0, "Error %d: unable to open %s\n", fd, elf_file);
  
  Elf64_Ehdr eh;
	read_elf_header(fd, &eh);
  display_elf_hedaer(eh);

  Elf64_Shdr sh_tbl[eh.e_shentsize * eh.e_shnum];
	read_section_headers(fd, eh, sh_tbl);
  display_section_headers(fd, eh, sh_tbl);

  read_symbols(fd, eh, sh_tbl);

	init_tail_rec_list();

	close(fd);
}

static int find_symbol_func(paddr_t target, bool is_call) {
	int i;
	for (i = 0; i < symbol_tbl_size; i++) {
		if (ELF64_ST_TYPE(symbol_tbl[i].info) == STT_FUNC) {
			if (is_call) {
				if (symbol_tbl[i].addr == target) break;
			} else {
				if (symbol_tbl[i].addr <= target && target < symbol_tbl[i].addr + symbol_tbl[i].size) break;
			}
		}
	}
	return i<symbol_tbl_size?i:-1;
}

static void insert_tail_rec(paddr_t pc, paddr_t depend) {
	TailRecNode *node = (TailRecNode *)malloc(sizeof(TailRecNode));
	node->pc = pc;
	node->depend = depend;
	node->next = tail_rec_head->next;
	tail_rec_head->next = node;
}

static void remove_tail_rec() {
	TailRecNode *node = tail_rec_head->next;
	tail_rec_head->next = node->next;
	free(node);
}

void trace_func_call(paddr_t pc, paddr_t target, bool is_tail) {
	if (symbol_tbl == NULL) return;

	++call_depth;

	if (call_depth <= 2) return; // ignore _trm_init & main

	int i = find_symbol_func(target, true);
  printf("call\n");
	log_write(FMT_PADDR ": %*scall [%s@" FMT_PADDR "]\n",
		pc,
		(call_depth-3)*2, "",
		i>=0?symbol_tbl[i].name:"???",
		target
	);

	if (is_tail) {
		insert_tail_rec(pc, target);
	}
}

void trace_func_ret(paddr_t pc) {
	if (symbol_tbl == NULL) return;
	
	if (call_depth <= 2) return; // ignore _trm_init & main

	int i = find_symbol_func(pc, false);
	log_write(FMT_PADDR ": %*sret [%s]\n",
		pc,
		(call_depth-3)*2, "",
		i>=0?symbol_tbl[i].name:"???"
	);
	
	--call_depth;

	TailRecNode *node = tail_rec_head->next;
	if (node != NULL) {
		int depend_i = find_symbol_func(node->depend, true);
		if (depend_i == i) {
			paddr_t ret_target = node->pc;
			remove_tail_rec();
			trace_func_ret(ret_target);
		}
	}
}

// typedef struct { // A Symbol Table item
//   paddr_t addr;
//   char name[32];
//   unsigned char info;
// } Symbol; 

// Symbol *symbol_tab = NULL;
// int symbol_tab_size = 0;
// int call_depth = 0;
// void  init_symbol_tab(){
        
// }
// void parse_elf(const char *elf_file) {
//   if (elf_file == NULL) return;
  
// 	// Log("specified ELF file: %s", elf_file);
//   // int fd = open(elf_file, O_RDONLY|O_SYNC);
//   // Assert(fd >= 0, "Error %d: unable to open %s\n", fd, elf_file);
  
//   // Elf64_Ehdr eh;
// 	// read_elf_header(fd, &eh);
//   // display_elf_hedaer(eh);

//   // Elf64_Shdr sh_tbl[eh.e_shentsize * eh.e_shnum];
// 	// read_section_headers(fd, eh, sh_tbl);
//   // display_section_headers(fd, eh, sh_tbl);

//   // read_symbols(fd, eh, sh_tbl);

// 	// init_tail_rec_list();

// 	// close(fd);
// }


// int find_func_name(paddr_t addr) {
//   int i;
//   for (i = 0; i < symbol_tab_size; i++) {
//     if (symbol_tab[i].addr == addr)
//       return i;
//   }
//   return -1;
// }
// void ftrace_call(paddr_t pc, paddr_t target, bool is_tail) {
//   call_depth++;
// }
// void ftrace_ret(paddr_t pc,bool is_tail){

//   int i = find_func_name(pc);
//   log_write("ret from %s\n", symbol_tab[i].name);
//   call_depth--;
// }