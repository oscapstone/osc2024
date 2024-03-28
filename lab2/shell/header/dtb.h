
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef void (*fdt_callback)(int type,const char* name,const void *data,uint32_t size);


// Create a structure for holding the FDT header information(fdt_header)
// structure block和string block是FDT格式的兩個重要部分，在boot將硬體資訊傳遞給 Linux核心。
// structure block:包含設備節點和屬性的線性化樹
// 樹中的每個節點代表一個設備或設備的邏輯部分，屬性是這些設備的屬性。
// structure block由一系列表示DT的結構和內容的標記組成。
struct __attribute__((packed)) fdt_header {
    uint32_t magic;             // contain the value 0xd00dfeed (big-endian).
    uint32_t totalsize;         // 包含DTB的總大小(byte)
    uint32_t off_dt_struct;     // structure block從header的offset(byte)
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;   // DTB的string block部分的長度(byte)
    uint32_t size_dt_struct;    // DTB的structure block部分的長度(byte)
};

int fdt_traverse(fdt_callback cb,void *dtb_ptr);
void get_cpio_addr(int token,const char* name,const void* data,uint32_t size);
void print_dtb(int token, const char* name, const void* data, uint32_t size);
