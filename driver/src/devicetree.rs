// A parser parse device tree

struct
    DeviceTreeParser
{
    buffer: *const u8,
    cur_pos: *const u8,
}