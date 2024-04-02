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

    uint32_t *aa = (uint32_t *) ((char *) fdt_header + fdt_header->off_dt_struct);  // +off_dt_struct

    unflatten_tree(aa);

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

fdt_node_t *unflatten_tree(uint32_t *blob)
{
    fdt_node_t *node = build_node(NULL, NULL);
    int eaten;

    BLOB_ADVANCE(blob, read_string(node, blob));

    int index = 0;
    do {
        switch (swap_endian(*blob)) {
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
    } while (index++ < 3);

    return node;
}

/**
 * Return: bytes eaten
*/
int read_string(fdt_node_t *node, uint32_t *blob)
{
    // TODO: refactor structure here
    int eaten = 0;

    // read node name (start from FDT_BEGIN_NODE)
    if (swap_endian(*(blob++)) != FDT_BEGIN_NODE) {
        uart_puts("invalid (sub-)tree\n");
        return NULL;
    }
    eaten += sizeof(uint32_t);

    int len = 0;
    char c;
    node->name = blob;
    do {
        len++;
    } while((c = *((char *)blob + len)) != '\0');
    int padding = PADDING_4(len);

    eaten += padding;
    return eaten;
}

int read_property(fdt_node_t *node, uint32_t *blob)
{
    uint32_t *original = blob;
    fdt_propmeta_t *meta;

    while (1) {
        if (swap_endian(*blob) != FDT_PROP) {
            uart_puts("NONONO\n");
            break;
        }
        BLOB_ADVANCE(blob, sizeof(uint32_t));
        meta = (fdt_propmeta_t *) blob;

        BLOB_ADVANCE(blob, sizeof(fdt_propmeta_t));
        BLOB_ADVANCE(blob, PADDING_4(swap_endian(meta->len)));
    }

    return (int) ((char *) blob - (char *) original);
}

fdt_property_t *init_property_t()
{
    fdt_property_t *prop = (fdt_property_t *) malloc(sizeof(fdt_property_t));

    prop->name = NULL;
    prop->value = NULL;
    prop->next = NULL;

    return prop;
}
