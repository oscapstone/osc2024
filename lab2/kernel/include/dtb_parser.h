#define FDT_BEGIN_NODE 0x1 /* Start node: full name */
#define FDT_END_NODE 0x2   /* End node */
#define FDT_PROP 0x3       /* Property: name off, size, content */
#define FDT_NOP 0x4        /* nop */
#define FDT_END 0x9

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef void (*FuncPtr)(uint32_t , char *, char *);

struct fdt_header
{
    uint32_t magic;             /* magic word FDT_MAGIC */
    uint32_t totalsize;         /* total size of DT block */
    uint32_t off_dt_struct;     /* offset to structure */
    uint32_t off_dt_strings;    /* offset to strings */
    uint32_t off_mem_rsvmap;    /* offset to memory reserve map */
    uint32_t version;           /* format version */
    uint32_t last_comp_version; /* last compatible version */

    /* version 2 fields below */
    uint32_t boot_cpuid_phys; /* Which physical CPU id we're
                    booting on */
    /* version 3 fields below */
    uint32_t size_dt_strings; /* size of the strings block */

    /* version 17 fields below */
    uint32_t size_dt_struct; /* size of the structure block */
};

struct fdt_property
{
    uint32_t len;
    uint32_t nameoff;
};

extern char *dtb_ptr;

void fdt_traverse(FuncPtr fun_ptr);
void struct_parser(FuncPtr fun_ptr, struct fdt_header *header, char *struct_ptr, char *string_ptr);
void initramfs_callback(uint32_t token , char *name, char *prop);
void device_name_callback(uint32_t token , char *name, char *prop);