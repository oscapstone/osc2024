#include "../include/cpio.h"
#include "../include/utils.h"

/* Parse an ASCII hex string into an integer. (big endian)*/
static unsigned int parse_hex_str(char *s, unsigned int max_len)
{
	unsigned int r = 0;

	for (unsigned int i = 0; i < max_len; i++)
	{
		r *= 16;
		if (s[i] >= '0' && s[i] <= '9')
		{
			r += s[i] - '0';
		}
		else if (s[i] >= 'a' && s[i] <= 'f')
		{
			r += s[i] - 'a' + 10;
		}
		else if (s[i] >= 'A' && s[i] <= 'F')
		{
			r += s[i] - 'A' + 10;
		}
		else
		{
			return r;
		}
	}
	return r;
}

//
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize,
						   char **data, struct cpio_newc_header **next_header_pointer)
{
	// this_header_pointer: ptr point to newc format header
	// pathname: double ptr point to "ptr point to 'this_header_pointer'"
	/* Ensure magic header exists. */
	if (strncmp(this_header_pointer->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(this_header_pointer->c_magic)) != 0)
		return -1;

	// transfer big endian 8 byte hex string to unsigned int and store into *filesize
	// 8 is the size of type "long"
	*filesize = parse_hex_str(this_header_pointer->c_filesize, 8);

	// file path start at the bit after header
	*pathname = ((char *)this_header_pointer) + sizeof(struct cpio_newc_header); // pointer + offset

	// data start at the bit after file path
	unsigned int pathname_length = parse_hex_str(this_header_pointer->c_namesize, 8);
	unsigned int offset = pathname_length + sizeof(struct cpio_newc_header);

	// pad data to align 4-byte
	offset = offset % 4 == 0 ? offset : (offset + 4 - offset % 4);
	*data = (char *)this_header_pointer + offset; // header + pathname len

	// get next header
	// if is last file, point to end position, else point to next file header
	if (*filesize == 0)
	{
		*next_header_pointer = (struct cpio_newc_header *)*data;
	}
	else
	{
		offset = *filesize;
		*next_header_pointer = (struct cpio_newc_header *)(*data + (offset % 4 == 0 ? offset : (offset + 4 - offset % 4)));
	}

	// if file path == TRAILER!!! mean there's no more file behind
	if (strncmp(*pathname, "TRAILER!!!", sizeof("TRAILER!!!")) == 0)
	{
		*next_header_pointer = 0;
	}
	return 0;
}