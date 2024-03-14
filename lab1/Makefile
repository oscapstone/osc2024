# 自動尋找所有.c檔案作為源碼檔案
SRCS = $(wildcard *.c)
# 將所有的.c檔案名換成.o作為目標檔案名
OBJS = $(SRCS:.c=.o)
# 設定編譯器的旗標：開啟所有警告、優化等級2、不使用標準庫、不使用啟動檔案
CFLAGS = -g -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles
# 定義編譯器
CC = aarch64-none-elf-gcc
# 定義鏈接器
LINKER = aarch64-none-elf-ld
# 定義目標複製工具，用於將ELF格式的可執行檔案轉換成二進位格式
OBJ_CPY = aarch64-none-elf-objcopy

# 預設目標：先清理再建立kernel8.img
all: clean kernel8.img

# 編譯啟動組合語言檔案
start.o: start.S
	$(CC) $(CFLAGS) -c start.S -o start.o

# 通用規則：將所有.c檔案編譯成.o檔案
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 連結所有的.o檔案，產生ELF格式的kernel8.elf，再將其轉換成二進位格式的kernel8.img
kernel8.img: start.o $(OBJS)
	$(LINKER) --nostdlib start.o $(OBJS) -T link.ld -o kernel8.elf
	$(OBJ_CPY) -O binary kernel8.elf kernel8.img

# 清理目標：刪除之前建立的檔案，忽略任何錯誤訊息
clean:
	rm kernel8.elf kernel8.img *.o >/dev/null 2>/dev/null || true

# 執行目標：使用QEMU模擬器執行kernel8.img，設定模擬的機器為Raspberry Pi 3模型B
run:
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none  -kernel kernel8.img
