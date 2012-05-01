all: initramfs.cpio

#pas=ARTATECH_PASS
#n=3
#s=PASSWORD\=\"`cat ${pas}`\"
initramfs.cpio: initramfs.conf
	sed -i "${n}c$s" init
	../../linux/usr/gen_init_cpio initramfs.conf > $@

.PHONY:clean

clean:
	rm -f initramfs.cpio
