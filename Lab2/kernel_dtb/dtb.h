#define DT_MAGIC_NUMBER 0xD00DFEED
#define NULL 0x00000000
#define DT_BEGIN_NODE_TOKEN 0x00000001
#define DT_END_NODE_TOKEN 0x00000002
#define DT_PROP_TOKEN 0x00000003
#define DT_NOP_TOKEN 0x00000004
#define DT_END_TOKEN 0x00000009

/*
FDT_BEGIN_NODE (0x00000001)
The FDT_BEGIN_NODE token marks the beginning of a node’s representation. It shall be followed by the node’s
unit name as extra data. The name is stored as a null-terminated string, and shall include the unit address (see Section
2.2.1), if any. The node name is followed by zeroed padding bytes, if necessary for alignment, and then the next
token, which may be any token except FDT_END.
FDT_END_NODE (0x00000002)
The FDT_END_NODE token marks the end of a node’s representation. This token has no extra data; so it is followed
immediately by the next token, which may be any token except FDT_PROP.
FDT_PROP (0x00000003)
The FDT_PROP token marks the beginning of the representation of one property in the devicetree. It shall be
followed by extra data describing the property. This data consists first of the property’s length and name represented
as the following C structure:
struct {
    uint32_t len;
    uint32_t nameoff;
}
5.4. Structure Block 53
Devicetree Specification, Release v0.4
Both the fields in this structure are 32-bit big-endian integers.
• len gives the length of the property’s value in bytes (which may be zero, indicating an empty property, see
Section 2.2.4).
• nameoff gives an offset into the strings block (see Section 5.5) at which the property’s name is stored as a
null-terminated string.
After this structure, the property’s value is given as a byte string of length len. This value is followed by zeroed
padding bytes (if necessary) to align to the next 32-bit boundary and then the next token, which may be any token
except FDT_END.
FDT_NOP (0x00000004)
The FDT_NOP token will be ignored by any program parsing the device tree. This token has no extra data; so it
is followed immediately by the next token, which can be any valid token. A property or node definition in the tree
can be overwritten with FDT_NOP tokens to remove it from the tree without needing to move other sections of the
tree’s representation in the devicetree blob.
FDT_END (0x00000009)
The FDT_END token marks the end of the structure block. There shall be only one FDT_END token, and it shall
be the last token in the structure block. It has no extra data; so the byte immediately after the FDT_END token has
offset from the beginning of the structure block equal to the value of the size_dt_struct field in the device tree blob
header
*/

unsigned int big_to_little_endian(unsigned int value);
void dt_tranverse(char *address, char *target_property, void (*callback)(char *));