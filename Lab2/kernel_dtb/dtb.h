#define DT_MAGIC_NUMBER 0xD00DFEED
#define NULL 0x0
#define DT_BEGIN_NODE_TOKEN 0x1
#define DT_END_NODE_TOKEN 0x2
#define DT_PROP_TOKEN 0x3
#define DT_NOP_TOKEN 0x4
#define DT_END_TOKEN 0x9

#define INITRD_START "linux,initrd-start"
unsigned int big_to_little_endian(unsigned int value);
void dt_tranverse(char *address, char *target_property, void (*callback)(char *));