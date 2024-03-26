#include "dtb.h"
#include "uart1.h"
#include "utils.h"
#include "cpio.h"

extern void* CPIO_DEFAULT_PLACE; //在shell.c中定義
// extern 表示: CPIO_DEFAULT_PLACE 變數在其他地方定義，這裡僅僅是聲明它的存在。
// void* 表示: 這是一個指向任意類型的指標，用於存儲 CPIO 檔案系統的起始地址。
// CPIO_DEFAULT_PLACE 是一個全局指標，它被用來指向系統中 CPIO 檔案系統的預設位置，通常這是在啟動時由啟動程序設定的。
char* dtb_ptr;
// char* 是一個指向character的指標，而它實際上被用來指向設備樹的二進制數據。
// dtb_ptr 是一個全局指標，用於存儲設備樹的起始地址。

// stored as big endian
// 以下代表設備樹的頭部信息
struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};


// 將一個 32 bits的數值從 big-endian 轉換為小端序 little-endian
// big-endian: 最高位元組 (MSB) 放在最低的記憶體位址，而在little-endian中，最低位元組 (LSB) 放在最低的記憶體位址。
// uint32_t: 32 位元的無符號整數 (unsigned integer)
uint32_t uint32_endian_big2lttle(uint32_t data)
{
    char* r = (char*)&data;
    return (r[3]<<0) | (r[2]<<8) | (r[1]<<16) | (r[0]<<24);
}
/*ARM 架構的處理器通常使用小端序來儲存數據，
這意味著在記憶體中，一個數值的最低有效位元組（Least Significant Byte, LSB）儲存在最低的位址處，
而最高有效位元組（Most Significant Byte, MSB）儲存在最高的位址處。
然而，設備樹（Device Tree Blob, DTB）中的資料通常以大端序的格式存儲，即 MSB 儲存在最低的位址處。*/


//分別在main.c與shell.c調用此函數
void traverse_device_tree(void *dtb_ptr, dtb_callback callback)
{
    struct fdt_header* header = dtb_ptr;
    // 0xD00DFEED是一個用於標識設備樹格式的固定數值
    // 這個檢查用於確保正在解析的數據確實是一個有效的設備樹。
    if(uint32_endian_big2lttle(header->magic) != 0xD00DFEED) 
    {
        uart_puts("traverse_device_tree : wrong magic in traverse_device_tree");
        return;
        // 不是一個有效的設備樹
    }
    // https://abcamus.github.io/2016/12/28/uboot%E8%AE%BE%E5%A4%87%E6%A0%91-%E8%A7%A3%E6%9E%90%E8%BF%87%E7%A8%8B/
    // https://blog.csdn.net/wangdapao12138/article/details/82934127
    uint32_t struct_size = uint32_endian_big2lttle(header->size_dt_struct);// 將設備樹結構體的大小從大端序轉換為小端序，並存儲到 struct_size 變量中。
    char* dt_struct_ptr = (char*)((char*)header + uint32_endian_big2lttle(header->off_dt_struct)); // 計算設備樹結構體的起始地址。首先將 header->off_dt_struct 從大端序轉換為小端序，然後將其加到 header 的地址上，得到設備樹結構體的起始指針 dt_struct_ptr。
    char* dt_strings_ptr = (char*)((char*)header + uint32_endian_big2lttle(header->off_dt_strings)); // 計算設備樹字符串區域的起始地址，方法類似於上一步，但是使用的是 header->off_dt_strings。

    char* end = (char*)dt_struct_ptr + struct_size;// 計算設備樹結構體的結束地址，即起始地址加上結構體的大小。
    char* pointer = dt_struct_ptr;// 初始化一個指針 pointer，用於遍歷設備樹結構體。

    while(pointer < end) //直到 pointer 指到設備樹結構體的結束地址時跳出while loop。
    {
        uint32_t token_type = uint32_endian_big2lttle(*(uint32_t*)pointer);
        /*讀取當前指針位置的 4 個字節，將其從大端序轉換為小端序，
        並將結果存儲到 token_type 變量中。
        這 4 個字節代表了設備樹節點的類型。*/

        pointer += 4;// 將指針向前移動 4 個字節，跳過剛才讀取的節點類型。

        if(token_type == FDT_BEGIN_NODE)//如果節點類型是 FDT_BEGIN_NODE，表示這是一個新的設備樹節點的開始。
        {
            callback(token_type, pointer, 0, 0);
            /*調用callback函數，傳入節點類型、當前指針位置、以及其他適當的參數。
            這個callback函數用於處理設備樹的節點。*/
            
            pointer += strlen(pointer);
            /*將指針向前移動，跳過節點名稱的字符串。
            strlen(pointer) 計算當前指針位置的字符串的長度。*/

            pointer += 4 - (unsigned long long) pointer % 4;//alignment 4 byte

        }
        else if(token_type == FDT_END_NODE)//表示一個設備樹節點的結束。
        {
            callback(token_type, 0, 0, 0);
            /*調用回調函數，傳入節點類型 token_type 和三個為 0 的參數。
            這表示節點結束，沒有額外的數據需要處理。*/
        }
        else if(token_type == FDT_PROP)//如果節點類型是 FDT_PROP，表示這是一個屬性節點。
        {
            uint32_t len = uint32_endian_big2lttle(*(uint32_t*)pointer);
            pointer += 4;
            char* name = (char*)dt_strings_ptr + uint32_endian_big2lttle(*(uint32_t*)pointer);//最後加上字符串區的起始地址 dt_strings_ptr。
            pointer += 4;
            callback(token_type, name, pointer, len);//調用回調函數，傳入節點類型 token_type、屬性名稱 name、屬性值的指針 pointer 和屬性長度 len。
            pointer += len;//將指針向前移動 len 個字節，跳過屬性值。
            if((unsigned long long)pointer % 4 != 0)
                pointer += 4 - (unsigned long long)pointer%4;   //alignment 4 byte
        }   
        else if(token_type == FDT_NOP)//如果節點類型是 FDT_NOP，表示這是一個空操作，通常用於填充或對齊。
        {
            callback(token_type, 0, 0, 0);
        }
        else if(token_type == FDT_END)//如果節點類型是 FDT_END，表示設備樹的結束。
        {
            callback(token_type, 0, 0, 0);
        }
        else
        {
            uart_puts("error type:%x\n",token_type);
            return;
        }
    }
}

//印出設備樹 display
//從 shell.c call 此function
void dtb_callback_show_tree(uint32_t node_type, char *name, void *data, uint32_t name_size)
{
    static int level = 0;
    /*定義一個靜態變量 level，用於記錄當前節點的深度（即在設備樹中的層級）。
    靜態變量在函數調用之間保持其值不變。
    */

    if(node_type==FDT_BEGIN_NODE)//新的設備樹節點的開始。
    {   
        /*根據當前的層級 level，輸出適當數量的空格，
        以便在顯示時能夠體現出節點在設備樹中的層次結構。*/
        for(int i=0;i<level;i++){
            uart_puts("   ");
        }

        uart_puts("%s{\n",name);
        /*輸出節點的名稱，
        並在名稱後面加上一個左大括號 { 來表示這是一個節點的開始，然後換行。*/
        
        level++;
    }
    else if(node_type==FDT_END_NODE)//表示一個設備樹節點的結束。
    {
        level--;//將層級 level 減 1，因為離開了當前節點。
        for(int i=0;i<level;i++){
            uart_puts("   ");
        }
        uart_puts("}\n");//輸出一個右大括號 } 來表示一個節點的結束，然後換行。
    }
    else if(node_type==FDT_PROP)//如果節點類型是 FDT_PROP，表示這是一個屬性。
    {
        for(int i=0;i<level;i++){
            uart_puts("   ");
        }
        uart_puts("%s\n",name);// 輸出屬性的名稱，然後換行。
    }
}

//從 main.c call 此 function
//這是一個用於處理設備樹中屬性的callback函數。
//這個函數專門用來查找並設置初始 RAM 檔案系統 (initramfs) 的起始位置。
void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size) {
    // https://github.com/stweil/raspberrypi-documentation/blob/master/configuration/device-tree.md
    // linux,initrd-start will be assigned by start.elf based on config.txt
    // 首先確認節點類型是屬性 (FDT_PROP)，然後檢查屬性名稱是否是 "linux,initrd-start"。
    // 這個屬性名稱指定了 initramfs 的起始地址。
    if(node_type==FDT_PROP && strcmp(name,"linux,initrd-start")==0)
    {
        /*它將屬性值（即 initramfs 的起始地址）從大端序轉換為小端序，
        然後將結果轉換為指針並賦值給全局變量 CPIO_DEFAULT_PLACE。*/
        CPIO_DEFAULT_PLACE = (void *)(unsigned long long)uint32_endian_big2lttle(*(uint32_t*)value);
    }
    /*這個變量用來存儲 initramfs 的起始地址，
    以便後續的程式碼可以訪問和使用 initramfs。*/
}
/*為什麼要初始化ram? (initramfs) 
1. 在某些情況下，遍歷設備樹的過程可能與初始 RAM 磁盤相關聯。
例如，如果系統需要在啟動過程中從設備樹中獲取有關初始 RAM 磁盤位置或配置的信息，
那麼在遍歷設備樹時就需要處理這些信息。
2. 遍歷設備樹的callback函數會被用來設置初始 RAM 磁盤的相關參數，
以便正確地加載和使用初始 RAM 磁盤。
(設備樹中包含了 initramfs 的位置和大小等參數)
*/

/*樹莓派啟動順序：
系統會先初始化 RAM 磁盤（initramfs 或 initrd），
然後再讀取設備樹（Device Tree Blob，DTB）。
在啟動過程中，啟動加載程序（如 U-Boot）會先將 initramfs 加載到記憶體中，
然後加載kernel8.img。kernel啟動後，它會使用 initramfs 作為臨時的根文件系統，
並讀取設備樹來獲取硬件設備的配置信息。
這樣，kernel就能正確地識別和初始化系統中的硬件設備。
*/
/*
- linux,initrd-start
這個屬性會在樹莓派加載了kernel檔案後，
但尚未掛載根文件系統之前被callback函數讀取。
在這個階段，kernel需要先找到initramfs的位置，
以便將它加載到記憶體中使用它作為臨時的根文件系統。

- linux,initrd-start這個屬性用於指定初始RAM磁盤(initramfs)在記憶體中的起始地址
用於告訴kernel哪裡可以找到initramfs.cpio

- rootfs 目錄中的內容會被打包成 initramfs.cpio 存檔，
並在系統啟動時被解壓縮到記憶體中，形成一個臨時的根文件系統。
在啟動過程中，一旦實際的根文件系統準備好了，
內核就會切換到實際的根文件系統，並繼續剩餘的啟動過程。
*/

/*
臨時的根文件系統==initramfs
initramfs.cpio 文件是一個 CPIO 格式的存檔，
它包含了用於形成初始 RAM 文件系統（initramfs）的一組文件和目錄。
其實就是包含rootfs這個資料夾

*/

