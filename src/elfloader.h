#ifndef ELFLOADER_H
#define ELFLOADER_H

# include <stdint.h>
 
typedef uint16_t Elf32_Half;    // Unsigned half int
typedef uint32_t Elf32_Off; // Unsigned offset
typedef uint32_t Elf32_Addr;    // Unsigned address
typedef uint32_t Elf32_Word;    // Unsigned int
typedef int32_t  Elf32_Sword;   // Signed int

# define ELF_NIDENT 16
 
typedef struct
{
  unsigned char e_ident[ELF_NIDENT]; /* Magic number and other info */
  Elf32_Half    e_type;         /* Object file type */
  Elf32_Half    e_machine;      /* Architecture */
  Elf32_Word    e_version;      /* Object file version */
  Elf32_Addr    e_entry;        /* Entry point virtual address */
  Elf32_Off e_phoff;        /* Program header table file offset */
  Elf32_Off e_shoff;        /* Section header table file offset */
  Elf32_Word    e_flags;        /* Processor-specific flags */
  Elf32_Half    e_ehsize;       /* ELF header size in bytes */
  Elf32_Half    e_phentsize;        /* Program header table entry size */
  Elf32_Half    e_phnum;        /* Program header table entry count */
  Elf32_Half    e_shentsize;        /* Section header table entry size */
  Elf32_Half    e_shnum;        /* Section header table entry count */
  Elf32_Half    e_shstrndx;     /* Section header string table index */
} Elf32_Ehdr;


enum Elf_Ident {
    EI_MAG0     = 0, // 0x7F
    EI_MAG1     = 1, // 'E'
    EI_MAG2     = 2, // 'L'
    EI_MAG3     = 3, // 'F'
    EI_CLASS    = 4, // Architecture (32/64)
    EI_DATA     = 5, // Byte Order
    EI_VERSION  = 6, // ELF Version
    EI_OSABI    = 7, // OS Specific
    EI_ABIVERSION   = 8, // OS Specific
    EI_PAD      = 9  // Padding
};
 
# define ELFMAG0    0x7F // e_ident[EI_MAG0]
# define ELFMAG1    'E'  // e_ident[EI_MAG1]
# define ELFMAG2    'L'  // e_ident[EI_MAG2]
# define ELFMAG3    'F'  // e_ident[EI_MAG3]
 
# define ELFDATA2LSB    (1)  // Little Endian
# define ELFCLASS32 (1)  // 32-bit Architecture

enum Elf_Type {
    ET_NONE     = 0, // Unkown Type
    ET_REL      = 1, // Relocatable File
    ET_EXEC     = 2  // Executable File
};
 
# define EM_386     (3)  // x86 Machine Type
# define EV_CURRENT (1)  // ELF Current Version


/* Section header.  */

typedef struct
{
  Elf32_Word    sh_name;        /* Section name (string tbl index) */
  Elf32_Word    sh_type;        /* Section type */
  Elf32_Word    sh_flags;       /* Section flags */
  Elf32_Addr    sh_addr;        /* Section virtual addr at execution */
  Elf32_Off sh_offset;      /* Section file offset */
  Elf32_Word    sh_size;        /* Section size in bytes */
  Elf32_Word    sh_link;        /* Link to another section */
  Elf32_Word    sh_info;        /* Additional section information */
  Elf32_Word    sh_addralign;       /* Section alignment */
  Elf32_Word    sh_entsize;     /* Entry size if section holds table */
} Elf32_Shdr;

#define SHN_UNDEF   0       /* Undefined section */
#define SHN_LORESERVE   0xff00      /* Start of reserved indices */
#define SHN_LOPROC  0xff00      /* Start of processor-specific */
#define SHN_BEFORE  0xff00      /* Order section before all others
                       (Solaris).  */
#define SHN_AFTER   0xff01      /* Order section after all others
                       (Solaris).  */
#define SHN_HIPROC  0xff1f      /* End of processor-specific */
#define SHN_LOOS    0xff20      /* Start of OS-specific */
#define SHN_HIOS    0xff3f      /* End of OS-specific */
#define SHN_ABS     0xfff1      /* Associated symbol is absolute */
#define SHN_COMMON  0xfff2      /* Associated symbol is common */
#define SHN_XINDEX  0xffff      /* Index is in extra table.  */
#define SHN_HIRESERVE   0xffff      /* End of reserved indices */

/* Legal values for sh_type (section type).  */

#define SHT_NULL      0     /* Section header table entry unused */
#define SHT_PROGBITS      1     /* Program data */
#define SHT_SYMTAB    2     /* Symbol table */
#define SHT_STRTAB    3     /* String table */
#define SHT_RELA      4     /* Relocation entries with addends */
#define SHT_HASH      5     /* Symbol hash table */
#define SHT_DYNAMIC   6     /* Dynamic linking information */
#define SHT_NOTE      7     /* Notes */
#define SHT_NOBITS    8     /* Program space with no data (bss) */
#define SHT_REL       9     /* Relocation entries, no addends */
#define SHT_SHLIB     10        /* Reserved */
#define SHT_DYNSYM    11        /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY    14        /* Array of constructors */
#define SHT_FINI_ARRAY    15        /* Array of destructors */
#define SHT_PREINIT_ARRAY 16        /* Array of pre-constructors */
#define SHT_GROUP     17        /* Section group */
#define SHT_SYMTAB_SHNDX  18        /* Extended section indeces */
#define SHT_NUM       19        /* Number of defined types.  */
#define SHT_LOOS      0x60000000    /* Start OS-specific.  */
#define SHT_GNU_ATTRIBUTES 0x6ffffff5   /* Object attributes.  */
#define SHT_GNU_HASH      0x6ffffff6    /* GNU-style hash table.  */
#define SHT_GNU_LIBLIST   0x6ffffff7    /* Prelink library list */
#define SHT_CHECKSUM      0x6ffffff8    /* Checksum for DSO content.  */
#define SHT_LOSUNW    0x6ffffffa    /* Sun-specific low bound.  */
#define SHT_SUNW_move     0x6ffffffa
#define SHT_SUNW_COMDAT   0x6ffffffb
#define SHT_SUNW_syminfo  0x6ffffffc
#define SHT_GNU_verdef    0x6ffffffd    /* Version definition section.  */
#define SHT_GNU_verneed   0x6ffffffe    /* Version needs section.  */
#define SHT_GNU_versym    0x6fffffff    /* Version symbol table.  */
#define SHT_HISUNW    0x6fffffff    /* Sun-specific high bound.  */
#define SHT_HIOS      0x6fffffff    /* End OS-specific type */
#define SHT_LOPROC    0x70000000    /* Start of processor-specific */
#define SHT_HIPROC    0x7fffffff    /* End of processor-specific */
#define SHT_LOUSER    0x80000000    /* Start of application-specific */
#define SHT_HIUSER    0x8fffffff    /* End of application-specific */

/* Legal values for sh_flags (section flags).  */

#define SHF_WRITE        (1 << 0)   /* Writable */
#define SHF_ALLOC        (1 << 1)   /* Occupies memory during execution */
#define SHF_EXECINSTR        (1 << 2)   /* Executable */
#define SHF_MERGE        (1 << 4)   /* Might be merged */
#define SHF_STRINGS      (1 << 5)   /* Contains nul-terminated strings */
#define SHF_INFO_LINK        (1 << 6)   /* `sh_info' contains SHT index */
#define SHF_LINK_ORDER       (1 << 7)   /* Preserve order after combining */
#define SHF_OS_NONCONFORMING (1 << 8)   /* Non-standard OS specific handling
                       required */
#define SHF_GROUP        (1 << 9)   /* Section is member of a group.  */
#define SHF_TLS          (1 << 10)  /* Section hold thread-local data.  */
#define SHF_COMPRESSED       (1 << 11)  /* Section with compressed data. */
#define SHF_MASKOS       0x0ff00000 /* OS-specific.  */
#define SHF_MASKPROC         0xf0000000 /* Processor-specific */
#define SHF_ORDERED      (1 << 30)  /* Special ordering requirement
                       (Solaris).  */
#define SHF_EXCLUDE      (1U << 31) /* Section is excluded unless
                       referenced or allocated (Solaris).*/


/* Program segment header.  */

typedef struct
{
  Elf32_Word    p_type;         /* Segment type */
  Elf32_Off p_offset;       /* Segment file offset */
  Elf32_Addr    p_vaddr;        /* Segment virtual address */
  Elf32_Addr    p_paddr;        /* Segment physical address */
  Elf32_Word    p_filesz;       /* Segment size in file */
  Elf32_Word    p_memsz;        /* Segment size in memory */
  Elf32_Word    p_flags;        /* Segment flags */
  Elf32_Word    p_align;        /* Segment alignment */
} Elf32_Phdr;

/* Special value for e_phnum.  This indicates that the real number of
   program headers is too large to fit into e_phnum.  Instead the real
   value is in the field sh_info of section 0.  */

#define PN_XNUM     0xffff

/* Legal values for p_type (segment type).  */

#define PT_NULL     0       /* Program header table entry unused */
#define PT_LOAD     1       /* Loadable program segment */
#define PT_DYNAMIC  2       /* Dynamic linking information */
#define PT_INTERP   3       /* Program interpreter */
#define PT_NOTE     4       /* Auxiliary information */
#define PT_SHLIB    5       /* Reserved */
#define PT_PHDR     6       /* Entry for header table itself */
#define PT_TLS      7       /* Thread-local storage segment */
#define PT_NUM      8       /* Number of defined types */
#define PT_LOOS     0x60000000  /* Start of OS-specific */
#define PT_GNU_EH_FRAME 0x6474e550  /* GCC .eh_frame_hdr segment */
#define PT_GNU_STACK    0x6474e551  /* Indicates stack executability */
#define PT_GNU_RELRO    0x6474e552  /* Read-only after relocation */
#define PT_LOSUNW   0x6ffffffa
#define PT_SUNWBSS  0x6ffffffa  /* Sun Specific segment */
#define PT_SUNWSTACK    0x6ffffffb  /* Stack segment */
#define PT_HISUNW   0x6fffffff
#define PT_HIOS     0x6fffffff  /* End of OS-specific */
#define PT_LOPROC   0x70000000  /* Start of processor-specific */
#define PT_HIPROC   0x7fffffff  /* End of processor-specific */

/* Legal values for p_flags (segment flags).  */

#define PF_X        (1 << 0)    /* Segment is executable */
#define PF_W        (1 << 1)    /* Segment is writable */
#define PF_R        (1 << 2)    /* Segment is readable */
#define PF_MASKOS   0x0ff00000  /* OS-specific */
#define PF_MASKPROC 0xf0000000  /* Processor-specific */

/* Legal values for note segment descriptor types for core files. */

#define NT_PRSTATUS 1       /* Contains copy of prstatus struct */
#define NT_FPREGSET 2       /* Contains copy of fpregset struct */
#define NT_PRPSINFO 3       /* Contains copy of prpsinfo struct */
#define NT_PRXREG   4       /* Contains copy of prxregset struct */
#define NT_TASKSTRUCT   4       /* Contains copy of task structure */
#define NT_PLATFORM 5       /* String from sysinfo(SI_PLATFORM) */
#define NT_AUXV     6       /* Contains copy of auxv array */
#define NT_GWINDOWS 7       /* Contains copy of gwindows struct */
#define NT_ASRS     8       /* Contains copy of asrset struct */
#define NT_PSTATUS  10      /* Contains copy of pstatus struct */
#define NT_PSINFO   13      /* Contains copy of psinfo struct */
#define NT_PRCRED   14      /* Contains copy of prcred struct */
#define NT_UTSNAME  15      /* Contains copy of utsname struct */
#define NT_LWPSTATUS    16      /* Contains copy of lwpstatus struct */
#define NT_LWPSINFO 17      /* Contains copy of lwpinfo struct */
#define NT_PRFPXREG 20      /* Contains copy of fprxregset struct */
#define NT_SIGINFO  0x53494749  /* Contains copy of siginfo_t,
                       size might increase */
#define NT_FILE     0x46494c45  /* Contains information about mapped
                       files */
#define NT_PRXFPREG 0x46e62b7f  /* Contains copy of user_fxsr_struct */
#define NT_PPC_VMX  0x100       /* PowerPC Altivec/VMX registers */
#define NT_PPC_SPE  0x101       /* PowerPC SPE/EVR registers */
#define NT_PPC_VSX  0x102       /* PowerPC VSX registers */
#define NT_386_TLS  0x200       /* i386 TLS slots (struct user_desc) */
#define NT_386_IOPERM   0x201       /* x86 io permission bitmap (1=deny) */
#define NT_X86_XSTATE   0x202       /* x86 extended state using xsave */
#define NT_S390_HIGH_GPRS   0x300   /* s390 upper register halves */
#define NT_S390_TIMER   0x301       /* s390 timer register */
#define NT_S390_TODCMP  0x302       /* s390 TOD clock comparator register */
#define NT_S390_TODPREG 0x303       /* s390 TOD programmable register */
#define NT_S390_CTRS    0x304       /* s390 control registers */
#define NT_S390_PREFIX  0x305       /* s390 prefix register */
#define NT_S390_LAST_BREAK  0x306   /* s390 breaking event address */
#define NT_S390_SYSTEM_CALL 0x307   /* s390 system call restart data */
#define NT_S390_TDB 0x308       /* s390 transaction diagnostic block */
#define NT_ARM_VFP  0x400       /* ARM VFP/NEON registers */
#define NT_ARM_TLS  0x401       /* ARM TLS register */
#define NT_ARM_HW_BREAK 0x402       /* ARM hardware breakpoint registers */
#define NT_ARM_HW_WATCH 0x403       /* ARM hardware watchpoint registers */

/* Legal values for the note segment descriptor types for object files.  */

#define NT_VERSION  1       /* Contains a version string.  */





#define CODE_ADDRESS 0x1000
#define MAX_CODE_SIZE 0x10000
#define DATA_ADDRESS 0x100000
#define MAX_DATA_SIZE 0x10000
#define MAX_SECTIONS 16

bool loadElf(char* code, size_t* codeSize, char* data, size_t* dataSize, char* raw, size_t size);

#endif