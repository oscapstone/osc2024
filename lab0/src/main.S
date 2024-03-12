.section ".text.boot"

.global _start


_start:

	wfe
	b	_start


