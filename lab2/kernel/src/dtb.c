#include "dtb.h"
#include "uart1.h"
#include "utils.h"
#include "cpio.h"

extern void* CPIO_DEFAULT_PLACE;
char* dtb_ptr;

//stored as big endian
//flattened device tree header
//儲存硬體設備的屬性
struct fdt_header {
    uint32_t magic; //驗證設備樹是否正確
    uint32_t totalsize;//DT結構的總大小
    uint32_t off_dt_struct; //offset:儲存偏移量 ex:設備樹起始地址+offset=儲存struct的地方
    uint32_t off_dt_strings;//ex:設備樹起始地址+offset=儲存string的地方
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;//string塊的長度
    uint32_t size_dt_struct;//struct塊的長度
};

//將32位元的大端整數轉為小端整數
//為了處理不同系統之間 數字在記憶體中儲存的方式
//從DTB(大端)傳給kernel看(小端)
uint32_t uint32_endian_big2lttle(uint32_t data)
{
    //char pointer r point to 輸入資料的地址
    char* r = (char*)&data; 
    //uint32_t 是由4bytes組成 所以一次進去看4bytes
    //每bytes有不同的做法 把大端轉小端 ex: 0x123  0x321
    return (r[3]<<0) | (r[2]<<8) | (r[1]<<16) | (r[0]<<24);
}

void traverse_device_tree(void *dtb_ptr, dtb_callback callback) 
{
    // dtb_ptr指向fdt_header
    // 將dtb_ptr轉換為fdt_header結構的指針 名為header 
    //可以直接用header訪問設備樹header中的各個字串(上上面那部分)
    struct fdt_header* header = dtb_ptr; 
    //先把header->magic轉換為小端
    //在檢查是否等於 0xD00DFEED 若不是，則代表是無效的設備樹資料
    if(uint32_endian_big2lttle(header->magic) != 0xD00DFEED)
    {
        //打印錯誤訊息
        uart_puts("traverse_device_tree : wrong magic in traverse_device_tree");
        return;
    }
    // https://abcamus.github.io/2016/12/28/uboot%E8%AE%BE%E5%A4%87%E6%A0%91-%E8%A7%A3%E6%9E%90%E8%BF%87%E7%A8%8B/
    // https://blog.csdn.net/wangdapao12138/article/details/82934127
    uint32_t struct_size = uint32_endian_big2lttle(header->size_dt_struct);
    char* dt_struct_ptr = (char*)((char*)header + uint32_endian_big2lttle(header->off_dt_struct)); //計算struct的地址並存起來 ptr指向此地指
    char* dt_strings_ptr = (char*)((char*)header + uint32_endian_big2lttle(header->off_dt_strings));

    char* end = (char*)dt_struct_ptr + struct_size; //計算結束地址存回end
    char* pointer = dt_struct_ptr; //設備樹struct部分的開始地址傳給pointer

    //這個循環是照著tree讀取數據，根據讀取到的標記類型，決定後續處理方式(callback函式)
    while(pointer < end)
    {
        //pointer指到的位址 讀取4bytes 轉成小端 賦值給token_type
        uint32_t token_type = uint32_endian_big2lttle(*(uint32_t*)pointer);

        //向前移動4bytes
        pointer += 4;
        //若標記類型是 "節點開始" 
        if(token_type == FDT_BEGIN_NODE)
        {
            //????
            callback(token_type, pointer, 0, 0);
            //向前移動至 "當前節點的結尾"
            pointer += strlen(pointer);
            //讓pointer對齊4的倍數
            pointer += 4 - (unsigned long long) pointer % 4;           //alignment 4 byte
        }
        //若標記類型是一個節點的結束
        else if(token_type == FDT_END_NODE) 
        {
            callback(token_type, 0, 0, 0);
        }
        //若標記類型是一個屬性
        else if(token_type == FDT_PROP)
        {
            //儲存屬性資料的 *長度*
            uint32_t len = uint32_endian_big2lttle(*(uint32_t*)pointer);
            pointer += 4;
            //計算屬性 *名字* 的指針
            char* name = (char*)dt_strings_ptr + uint32_endian_big2lttle(*(uint32_t*)pointer);
            pointer += 4;
            callback(token_type, name, pointer, len);
            pointer += len;
            if((unsigned long long)pointer % 4 != 0) pointer += 4 - (unsigned long long)pointer%4;   //alignment 4 byte
        }
        //若標記類型是無操作
        else if(token_type == FDT_NOP)
        {
            callback(token_type, 0, 0, 0);
        }
        //若標記類型是 tree的結尾
        else if(token_type == FDT_END)
        {
            callback(token_type, 0, 0, 0);
        }
        //若標記類型不在預期之內
        else
        {
            uart_puts("error type:%x\n",token_type);
            return;
        }
    }
}

// shell.c
void dtb_callback_show_tree(uint32_t node_type, char *name, void *data, uint32_t name_size)
{
    static int level = 0; //追蹤tree traverse的深度
    if(node_type==FDT_BEGIN_NODE)
    {
        for(int i=0;i<level;i++)uart_puts("   "); //print與這個層級相應數量的空格
        uart_puts("%s{\n",name);//節點名稱
        level++;
    }else if(node_type==FDT_END_NODE)
    {
        level--;
        for(int i=0;i<level;i++)uart_puts("   "); //print與這個層級相應數量的空格
        uart_puts("}\n");
    }else if(node_type==FDT_PROP) //屬性
    {
        for(int i=0;i<level;i++)uart_puts("   ");
        uart_puts("%s\n",name);//屬性名稱
    }
}

// main.c
// kernel啟動時 初始化RAM文件系統
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size) {
    // https://github.com/stweil/raspberrypi-documentation/blob/master/configuration/device-tree.md
    // linux,initrd-start will be assigned by start.elf based on config.txt
    
    //若屬性名稱me等於"linux,initrd-start" 回傳0
    if(node_type==FDT_PROP && strcmp(name,"linux,initrd-start")==0)
    {
        CPIO_DEFAULT_PLACE = (void *)(unsigned long long)uint32_endian_big2lttle(*(uint32_t*)value);
    }
}
