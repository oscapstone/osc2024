#include "fdt.h"
#include "malloc.h"
#include "uart.h"


// Linux fdt.c
//      https://elixir.free-electrons.com/linux/v4.15-rc9/source/drivers/of/fdt.c#L445
// unflatten tree
//      https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/flattree.c#L745
static fdt_header_t *fdt_header;
void fdt_traverse(fdt_callback_t cb)
{
    // parse header
    asm volatile("mov %0, x28" : "=r"(fdt_header));

    // swap little to big endian
    uint32_t *p = (uint32_t *) fdt_header;
    for (int i = 0; i < 10; i++) {
        p[i] = swap_endian(p[i]);
    }

    blob_t *blob = (blob_t *) malloc(sizeof(blob_t));
    blob->ptr = (uint32_t *) fdt_header;
    BLOB_ADVANCE(blob, fdt_header->off_dt_struct);

    unflatten_tree(blob);

    // execute callback fn
}

fdt_node_t *build_node(fdt_property_t *proplist, fdt_node_t *children)
{
    fdt_node_t *node = (fdt_node_t *) malloc(sizeof(fdt_node_t));

    node->proplist = proplist;
    node->children = children;

    // assign children's parent
    for (fdt_node_t *child = node->children; child; child = child->next_sibling) {
        child->parent = node;
    }

    return node;
}

fdt_node_t *unflatten_tree(blob_t *blob)
{
    fdt_node_t *node = build_node(NULL, NULL);
    int eaten;

    BLOB_ADVANCE(blob, read_string(node, blob));

    int index = 0;
    do {
        switch (swap_endian(*blob->ptr)) {
        case FDT_BEGIN_NODE:
            uart_puts("FDT_BEGIN_NODE\n");
            break;
        case FDT_END_NODE:
            uart_puts("FDT_END_NODE\n");
            break;
        case FDT_PROP:
            uart_puts("FDT_PROP\n");
            BLOB_ADVANCE(blob, read_property(node, blob));
            break;
        case FDT_END:
            uart_puts("FDT_END\n");
            break;
        case FDT_NOP:
            uart_puts("FDT_NOP\n");
            break;
        default:
            uart_puts("Default\n");
            break;
        }
    } while (++index < 7);

    // TODO: unflatten return 前應該要加 blob->ptr
    return node;
}

/**
 * Return: bytes eaten
*/
int read_string(fdt_node_t *node, blob_t *blob)
{
    // TODO: refactor structure here
    int eaten = 0;

    // read node name (start from FDT_BEGIN_NODE)
    if (swap_endian(*(blob->ptr++)) != FDT_BEGIN_NODE) {
        uart_puts("invalid (sub-)tree\n");
        return NULL;
    }
    eaten += sizeof(uint32_t);

    int len = 0;
    char c;
    node->name = blob->ptr;
    do {
        len++;
    } while((c = *((char *)blob->ptr + len)) != '\0');
    int padding = PADDING_4(len);

    eaten += padding;
    return eaten;
}

int read_property(fdt_node_t *node, blob_t *blob)
{
    uint32_t *original = blob->ptr;
    fdt_propmeta_t *meta;

    if (swap_endian(*blob->ptr) != FDT_PROP) {
        return -1;
    }
    BLOB_ADVANCE(blob, sizeof(uint32_t));
    meta = (fdt_propmeta_t *) blob->ptr;

    BLOB_ADVANCE(blob, sizeof(fdt_propmeta_t));
    BLOB_ADVANCE(blob, PADDING_4(swap_endian(meta->len)));

    return (int) ((char *) blob->ptr - (char *) original);
}

fdt_property_t *init_property_t()
{
    fdt_property_t *prop = (fdt_property_t *) malloc(sizeof(fdt_property_t));

    prop->name = NULL;
    prop->value = NULL;
    prop->next = NULL;

    return prop;
}
