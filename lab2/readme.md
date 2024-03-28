## Initial Ramdisk

why
- we want to load user program, but we don't have filesystem now

how
- initial ramdisk can be loaded by kernel, it's an archive which can be extracted to build a root filesystem

### Create CPIO
The [cpio](https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5) archive format	collects any number of files, directories, and other file system objects (symbolic links, device nodes,	etc.) into  a single stream of bytes.

```bash
mkdir rootfs
cd rootfs
...
# create some file and directory
echo "larem" > file.txt
#
find . | cpio -o -H newc > ../initramfs.cpio
cd ..

# change config.txt to let raspi load initramfs.cpio
echo "initramfs initramfs.cpio 0x20000000" >> config.txt
```

Each file system object(dir, file) comprises a header with numeric metadata followed by the full pathname of the entry and the file data. we use new ascii format here, check docs to read the header format.

```c

/*
The  "new"  ASCII format	uses 8-byte hexadecimal	fields for all numbers
and separates device numbers into separate fields for major and minor
numbers.
*/
struct cpio_newc_header {
    char    c_magic[6];         //070701
    char    c_ino[8];           //007E024E
    char    c_mode[8];          //000081B4
    char    c_uid[8];           //000003E8
    char    c_gid[8];           //000003E8
    char    c_nlink[8];         //00000001
    char    c_mtime[8];         //65E42C17
    char    c_filesize[8];      //0000002A
    char    c_devmajor[8];      //00000103
    char    c_devminor[8];      //00000005
    char    c_rdevmajor[8];     //00000000
    char    c_rdevminor[8];     //00000000
    char    c_namesize[8];      //00000005
    char    c_check[8];         //00000000  // always zero
    // after header:            //flag{0Sc_rOoTf$/FlAg::TH!5_Is_f1a9_OF_RoO7F$!}
};


/*
Except  as  specified  below, the fields	here match those specified for
the new binary format above.

magic   The string "070701".

check   This field is always set	to zero	 by  writers  and  ignored  by readers.
See the next section for more details.

The  pathname  is  followed  by NUL bytes so that the total size	of the fixed header plus pathname is a multiple	of four.  Likewise,  the  file data is padded to a multiple of four bytes.  Note that this format supports  only 4 gigabyte files (unlike the	older ASCII format, which sup-
ports 8 gigabyte files).

In this format, hardlinked files are handled by setting the filesize to zero for each entry except the first one that appears in the archive.
*/

```

## Devicetree
why
- kernel has to know what devices are connected and use corresponding driver to initialize/access it.
- kernel in simple bus can't detect devices like powerful buses (PCIe/USB)

how
- devicetree is an file, recording the properties and relationships between each device.

### Format
Devicetree
- devicetree source(dts)
  - human-readable
- flattenbed devicetree(dtb)
  - converted by dts

### DTB

[spec download](https://github.com/devicetree-org/devicetree-specification/releases)

```
<--- low mem addr --->

|  struct fdt_header  |
|    (free space)     |
|   mem reser block   |
|    (free space)     |
|   structure block   |
|    (free space)     |
|    strings block    |
|    (free space)     |

<--- high mem addr --->
```

All data is in big endian

```c
struct fdt_header {
	uint32_t magic;					// 0xd00dfeed (big-endian) 
	uint32_t totalsize;				// size in bytes
	uint32_t off_dt_struct;			// bytes offset to struct block
	uint32_t off_dt_strings;		// bytes offset to string block
	uint32_t off_mem_rsvmap;		// bytes offset to memory reservation block
	uint32_t version;				// version : now 17
	uint32_t last_comp_version;		// last compatible version, for 17 is 16
	uint32_t boot_cpuid_phys;		// physical ID of the system's boot CPU
	uint32_t size_dt_strings;		// size in bytes of strings block
	uint32_t size_dt_struct;		// size in bytes of struct block
};
```

#### Memory reservation block
A list of areas in physical memory which are reserved. Provided to the client program.

```c
struct fdt_reserve_entry {
    unsigned long long address;
    unsigned long long size;
};

// special terminal entry address == 0 size == 0;
```

#### Structure Block
- Describes the structure and contents of the devicetree
- Composed of a sequence of tokens
  - FDT_BEGIN_NODE (0x00000001)
  - FDT_END_NODE (0x00000002)
  - FDT_PROP (0x00000003) mark the beginiing of the representation of one property
```c
	struct {
		uint32_t len;
		uint32_t nameoff;
	}
```
  - FDT_NOP (0x00000004)
  - FDT_END (0x00000009)

