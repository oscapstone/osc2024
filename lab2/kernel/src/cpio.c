#include "cpio.h"
#include "utils.h"

/* Parse an ASCII hex string into an integer. (big endian)*/
static unsigned int parse_hex_str(char *s, unsigned int max_len)
{
    unsigned int r = 0;

    for (unsigned int i = 0; i < max_len; i++) {
        r *= 16;
        if (s[i] >= '0' && s[i] <= '9') {
            r += s[i] - '0';
        }  else if (s[i] >= 'a' && s[i] <= 'f') {
            r += s[i] - 'a' + 10;
        }  else if (s[i] >= 'A' && s[i] <= 'F') {
            r += s[i] - 'A' + 10;
        } else {
            return r;
        }
    }
    return r; // return 一個unsigned integer
}


/* write pathname, data, next header into corresponding parameter */
/* if no next header, next_header_pointer = 0 */
/* return -1 if parse error*/
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize, char **data, struct cpio_newc_header **next_header_pointer)
{
    /* Ensure magic header exists. */
    /*
    檢查標頭中的魔數 (c_magic) 是否與預期的 CPIO_NEWC_HEADER_MAGIC 值相符，
    如果不符則返回 -1 表示解析錯誤。
    */
    if (strncmp(this_header_pointer->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(this_header_pointer->c_magic)) != 0) return -1;

    // transfer big endian 8 byte hex string to unsigned int and store into *filesize
    *filesize = parse_hex_str(this_header_pointer->c_filesize,8);

    // end of header is the pathname
    *pathname = ((char *)this_header_pointer) + sizeof(struct cpio_newc_header); 

    // get file data, file data is just after pathname
    unsigned int pathname_length = parse_hex_str(this_header_pointer->c_namesize,8);
    unsigned int offset = pathname_length + sizeof(struct cpio_newc_header);
    // The file data is padded to a multiple of four bytes
    // 這行程式碼是用來確保檔案資料在記憶體中的偏移量是 4 的倍數。
    // 在許多系統中，資料對齊到 4 的倍數可以提高存取效率。
    offset = offset % 4 == 0 ? offset:(offset+4-offset%4);
    // 如果餘數不為 0，則需要進行填充，以使 offset 成為 4 的倍數。
    // 這是通過將 offset 增加 4 - offset % 4 來實現的，
    // 這樣新的 offset 就是最接近的比原 offset 大的 4 的倍數。
    // 這行程式碼確保檔案資料在記憶體中的偏移量是 4 的倍數，
    // 這是為了提高存取效率和滿足某些系統對資料對齊的要求。

    *data = (char *)this_header_pointer+offset;
    // (char *)this_header_pointer + offset: 計算出檔案資料的實際起始地址再存入data中。
    // 把這個地址賦值給 data 指標，使得 data 指向檔案的內容。

    //get next header pointer
    if(*filesize==0) // 若file size大小為0
    {
        *next_header_pointer = (struct cpio_newc_header*)*data;
        // 將一個指向某種資料的指標轉換成指向 struct cpio_newc_header 類型的指標。
        /* 這在處理二進位資料時很常見，例如從檔案或網路接收的資料，
        你需要將原始位元組資料轉換成具體的結構體以便於存取其內部欄位。*/
    }else
    {
        offset = *filesize;
        *next_header_pointer = (struct cpio_newc_header*)(*data + (offset%4==0?offset:(offset+4-offset%4)));
    }
    /*這行程式碼是用來計算next header pointer的位置。
    它將當前資料指標 *data 向前移動 offset 位元組，如果 offset 不是 4 的倍數，則向上取整到最接近的 4 的倍數。
    這是因為在 CPIO 檔案格式中，每個檔案的資料區塊需要在 4 位元組邊界上對齊。
    */

    // if filepath is TRAILER!!! means there is no more files.
    if(strncmp(*pathname,"TRAILER!!!", sizeof("TRAILER!!!"))==0)
    {
        *next_header_pointer = 0;
    }

    return 0;
}
