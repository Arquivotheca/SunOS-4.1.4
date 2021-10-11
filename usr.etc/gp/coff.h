/* @(#)coff.h 1.1 94/10/31 SMI */

/*
 * coff.h 
 */

#ifndef COFF

#define C30_MAGIC		0x0093
#define C30_OPT_MAGIC		0x0108
#define COFF_HDR_SIZE		20
#define OPT_HDR_SIZE		28
#define SYMBOL_ENTRY_SIZE	18
#define SECTION_HDR_SIZE	40
#define MAX_SECTIONS		255

typedef struct {
	       unsigned short	magic;
	       unsigned short	nheaders;
	       unsigned long	timedate;
	       unsigned long	symbt_off;
	       unsigned long	nsym;
	       unsigned short	optheadlen;
	       unsigned short	flags;
	       } COFF_HEADER;

typedef struct {
	       unsigned short magic;
	       unsigned short version;
	       unsigned long textbits;
	       unsigned long databits;
	       unsigned long bssbits;
	       unsigned long addrexec;
	       unsigned long addrbss;
	       unsigned long addrdata;
	       } OPT_HEADER;

typedef struct {
	       char name[8];
	       unsigned long physaddr;
	       unsigned long virtaddr;
	       unsigned long sizelongs;
	       unsigned long data_off;
	       unsigned long reloc_off;
	       unsigned long lnum_off;
	       unsigned short nreloc;
	       unsigned short nlnum;
	       unsigned short flags;
	       char reserved;
	       char mempage;
	       } SECTION_HEADER;

typedef struct {
	       unsigned long virtaddr;
	       unsigned long tblindex;
	       unsigned short type;
	       } RELOC_INFO;

typedef struct {
	       char name[8];
	       unsigned long value;
	       short sectnum;
	       unsigned short typespec;
	       char class;
	       char naux;
	       } SYMBOL;

#define T_NULL	 0
#define T_CHAR	 2
#define T_SHORT  3
#define T_INT	 4
#define T_LONG	 5
#define T_FLOAT  6
#define T_DOUBLE 7
#define T_STRUCT 8
#define T_UNION  9
#define T_ENUM	 10
#define T_MOE	 11
#define T_UCHAR  12
#define T_USHORT 13
#define T_UINT	 14
#define T_ULONG  15

#define DT_NON 0
#define DT_PTR 1
#define DT_FCN 2
#define DT_ARY 3

#define C_NULL	  0
#define C_AUTO	  1
#define C_EXT	  2
#define C_STAT	  3
#define C_REG	  4
#define C_EXTDEF  5
#define C_LABEL   6
#define C_ULABEL  7
#define C_MOS	  8
#define C_ARG	  9
#define C_STRTAG  10
#define C_MOU	  11
#define C_UNTAG   12
#define C_TPDEF   13
#define C_USTATIC 14
#define C_ENTAG   15
#define C_MOE	  16
#define C_REGPARM 17
#define C_FIELD   18
#define C_BLOCK   100
#define C_FCN	  101
#define C_EOS	  102
#define C_FILE	  103
#define C_LINE	  104

/* Section flags. */
#define F_REG        0x00           /* REGULAR SECTION                   */
#define F_DSECT      0x01           /* DUMMY SECTION (NOT LOADED)        */
#define F_NOLOAD     0x02           /* NOLOAD SECTION (NOT LOADED)       */
#define F_GROUP      0x04           /* FORMED BY GROUPING OTHER SECTIONS */
#define F_PAD        0x08           /* PADDING SECTION (NOT RELOCATED)   */
#define F_COPY       0x10           /* COPY SECTION (NOT RELOCATED)      */
#define F_TEXT       0x20           /* EXECUTABLE CODE                   */
#define F_DATA       0x40           /* INITIALIZED DATA                  */
#define F_BSS        0x80           /* UNINITIALIZED DATA                */


typedef struct {
	       char name[14];
	       char pad[4];
	       } S_FNAME;

typedef struct {
	       unsigned long length;
	       unsigned short nrelocs;
	       unsigned short nlinenos;
	       char pad[10];
	       } S_SECTION;

typedef struct {
	       char pad[6];
	       unsigned short size;
	       char pad1[4];
	       unsigned long next;
	       char pad2[2];
	       } S_TAGNAME;

typedef struct {
	       unsigned long tagindex;
	       char pad[2];
	       unsigned short size;
	       char pad1[10];
	       } S_ENDSTRUCT;

typedef struct {
	       unsigned long tagindex;
	       unsigned long size;
	       unsigned long ptrlineno;
	       unsigned long next;
	       char pad[2];
	       } S_FUNCTION;

typedef struct {
	       unsigned long tagindex;
	       unsigned short lineno;
	       unsigned short size;
	       unsigned short firstdim;
	       unsigned short seconddim;
	       unsigned short thirddim;
	       unsigned short fourthdim;
	       char pad[2];
	       } S_ARRAY;

typedef struct {
	       char pad[4];
	       unsigned short srcline;
	       char pad1[12];
	       } S_ENDBLOCK;

typedef struct {
	       char pad[4];
	       unsigned short srcline;
	       char pad1[6];
	       unsigned long next;
	       char pad2[2];
	       } S_STARTBLOCK;

typedef struct {
	       unsigned long tagindex;
	       char pad[2];
	       unsigned short size;
	       char pad1[10];
	       } S_TYPENAME;

typedef union  {
	       unsigned long  vlong;
	       unsigned short vshort[2];
	       } S_VALUE;

typedef struct {
	       unsigned long  value0;
	       S_VALUE	      value1;
	       S_VALUE	      value2;
	       S_VALUE	      value3;
	       char	      pad[2];
	       } AUX_SYMBOL;

typedef union {
    char type_a[18];
    struct {
        unsigned long a[4];
	char b[2];
    } type_b;
    struct {
        char a[4];
        unsigned short b[2];
	char c[4];
	unsigned long d;
	char e[2];
    } type_c;
    struct {
        unsigned long a;
        unsigned short b[6];
	char c[2];
    } type_d;
    struct {
	char a[4];
        unsigned short b;
	char c[6];
        unsigned long d;
	char e[2];
    } type_e;
} AUX_UNION;

#define COFF
#endif
