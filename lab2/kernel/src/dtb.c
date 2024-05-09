#include "dtb.h"
#include "uart1.h"
#include "utils.h"
#include "cpio.h"

extern void *CPIO_DEFAULT_PLACE; // External variable to hold the default place of the CPIO archive
char *dtb_ptr;                   // Global variable to hold the pointer to the device tree blob (DTB)

// Define the structure of the FDT (Flattened Device Tree) header
struct fdt_header
{
    uint32_t magic;             // Magic number to identify the FDT format
    uint32_t totalsize;         // Total size of the DTB
    uint32_t off_dt_struct;     // Offset to the structure block from the beginning of the DTB
    uint32_t off_dt_strings;    // Offset to the strings block from the beginning of the DTB
    uint32_t off_mem_rsvmap;    // Offset to the memory reserve map
    uint32_t version;           // Version of the DTB format
    uint32_t last_comp_version; // Last compatible version of the DTB format
    uint32_t boot_cpuid_phys;   // Physical ID of the booting CPU
    uint32_t size_dt_strings;   // Size of the strings block
    uint32_t size_dt_struct;    // Size of the structure block
};

// Function to traverse the device tree and execute a callback function for each node
void traverse_device_tree(void *dtb_ptr, dtb_callback callback)
{
    struct fdt_header *header = dtb_ptr;                      // Pointer to the FDT header
    if (__builtin_bswap32(header->magic) != 0xD00DFEED) // Check if the magic number is correct
    {
        uart_puts("traverse_device_tree : wrong magic in traverse_device_tree");
        return;
    }

    uint32_t struct_size = __builtin_bswap32(header->size_dt_struct);                            // Get the size of the structure block
    char *dt_struct_ptr = (char *)((char *)header + __builtin_bswap32(header->off_dt_struct));   // Pointer to the structure block dtb_spec.pdf p.50
    char *dt_strings_ptr = (char *)((char *)header + __builtin_bswap32(header->off_dt_strings)); // Pointer to the strings block

    char *end = (char *)dt_struct_ptr + struct_size; // Pointer to the end of the structure block
    char *pointer = dt_struct_ptr;                   // Pointer to the current position in the structure block

    while (pointer < end) // Loop through the structure block
    {
        uint32_t token_type = __builtin_bswap32(*(uint32_t *)pointer); // Get the token type

        pointer += 4;                     // Move the pointer to the next token
        if (token_type == FDT_BEGIN_NODE) // If the token is a beginning of a node
        {
            callback(token_type, pointer, 0, 0);            // Call the callback function
            pointer += strlen(pointer);                     // Move the pointer to the end of the node name
            pointer += 4 - (unsigned long long)pointer % 4; // Align the pointer to a 4-byte boundary
        }
        else if (token_type == FDT_END_NODE) // If the token is an end of a node
        {
            callback(token_type, 0, 0, 0); // Call the callback function
        }
        else if (token_type == FDT_PROP) // If the token is a property
        {
            uint32_t len = __builtin_bswap32(*(uint32_t *)pointer);                        // Get the length of the property value
            pointer += 4;                                                                        // Move the pointer to the property name offset
            char *name = (char *)dt_strings_ptr + __builtin_bswap32(*(uint32_t *)pointer); // Get the property name
            pointer += 4;                                                                        // Move the pointer to the property value
            callback(token_type, name, pointer, len);                                            // Call the callback function
            pointer += len;                                                                      // Move the pointer to the end of the property value
            if ((unsigned long long)pointer % 4 != 0)
                pointer += 4 - (unsigned long long)pointer % 4; // Align the pointer to a 4-byte boundary
        }
        else if (token_type == FDT_NOP) // If the token is a NOP (no operation)
        {
            callback(token_type, 0, 0, 0); // Call the callback function with no additional data
        }
        else if (token_type == FDT_END) // If the token is the end of the device tree
        {
            callback(token_type, 0, 0, 0); // Call the callback function to indicate the end of the device tree
            break;                         // Exit the loop
        }
        else // If the token is unrecognized
        {
            uart_puts("error type:%x\n", token_type); // Print an error message
            return;                                   // Exit the function
        }
    }
}

static int line_count = 0; // Add a static variable to track the number of printed lines

void pause_for_input()
{
    char c;
    if (line_count >= LINES_PER_PAGE)
    {                                            // If the number of lines printed reaches the limit per page
        uart_puts("Press ENTER to continue..."); // Prompt the user to press ENTER to continue
        while (1)
        {
            c = uart_recv(); // Receive a character from UART
            if (c == '\n' || c == '\r')
            {                    // If the received character is a newline or carriage return
                uart_puts("\n"); // Print a newline
                line_count = 0;  // Reset the line counter
                break;           // Break out of the waiting loop and continue printing
            }
        }
    }
}

void dtb_callback_show_tree(uint32_t node_type, char *name, void *data, uint32_t name_size)
{
    static int level = 0; // Static variable to keep track of the current level in the device tree
    if (node_type == FDT_BEGIN_NODE)
    { // If the node is the beginning of a new device tree node
        for (int i = 0; i < level; i++)
            uart_puts("   "); // Print indentation based on the current level
        uart_puts(name);      // Print the node name
        uart_puts("{\n");     // Print an opening brace
        level++;              // Increase the level for the next node
    }
    else if (node_type == FDT_END_NODE)
    {            // If the node is the end of a device tree node
        level--; // Decrease the level for the next node
        for (int i = 0; i < level; i++)
            uart_puts("   "); // Print indentation based on the current level
        uart_puts("}\n");     // Print a closing brace
    }
    else if (node_type == FDT_PROP)
    { // If the node is a property
        for (int i = 0; i < level; i++)
            uart_puts("   "); // Print indentation based on the current level
        uart_puts(name);      // Print the property name
        uart_puts("\n");      // Print a newline
    }

    line_count++;      // Increment the line counter for each printed line
    pause_for_input(); // Check if we need to pause for user input
}

void dtb_callback_initramfs(uint32_t node_type, char *name, void *value, uint32_t name_size)
{
    // https://github.com/stweil/raspberrypi-documentation/blob/master/configuration/device-tree.md
    // linux,initrd-start will be assigned by start.elf based on config.txt
    if (node_type == FDT_PROP && strcmp(name, "linux,initrd-start") == 0) // If the property is "linux,initrd-start"
    {
        CPIO_DEFAULT_PLACE = (void *)(unsigned long long)__builtin_bswap32(*(uint32_t *)value); // Set the default place of the CPIO archive
    }
}
