all: initramfs.cpio

initramfs.cpio: initramfs.conf
	../usr/gen_init_cpio initramfs.conf > $@

.PHONY:clean

clean:
	rm -f initramfs.cpio
