CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -g -mgeneral-regs-only
C    = aarch64-linux-gnu

LIB_PATH ?= ../lib
BSP_PATH ?= ../bsp
BUILD_DIR ?= ../build
CFLAGS += -I$(LIB_PATH) -I$(BSP_PATH) -I..

LIB_SRCS = $(shell find $(LIB_PATH) -type f -name '*.c')
BSP_SRCS = $(shell find $(BSP_PATH) -type f -name '*.c')
LOCAL_SRCS = $(shell find . -type f -name '*.c')
SRCS = $(LOCAL_SRCS) $(LIB_SRCS) $(BSP_SRCS)
OBJS = $(patsubst ./%.c,$(BUILD_DIR)/obj/%.o,$(LOCAL_SRCS)) \
       $(patsubst $(LIB_PATH)/%.c,$(BUILD_DIR)/obj/lib/%.o,$(LIB_SRCS)) \
       $(patsubst $(BSP_PATH)/%.c,$(BUILD_DIR)/obj/bsp/%.o,$(BSP_SRCS))

all: kernel8.img

$(BUILD_DIR)/obj/%.o: %.c
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/start.o: start.S
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/lib/%.o: $(LIB_PATH)/%.c
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/bsp/%.o: $(BSP_PATH)/%.c
	mkdir -p $(dir $@)
	$(C)-gcc $(CFLAGS) -c $< -o $@

kernel8.img: $(BUILD_DIR)/obj/start.o $(OBJS)
	$(C)-ld -nostdlib $^ -T $(BSP_PATH)/linker.ld -o $(BUILD_DIR)/kernel8.elf
	$(C)-objcopy -O binary $(BUILD_DIR)/kernel8.elf $(BUILD_DIR)/kernel8.img

