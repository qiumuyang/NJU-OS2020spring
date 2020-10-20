QEMU = qemu-system-i386

os.img:
	@cd utils/genFS; make
	@cd bootloader; make
	@cd kernel; make
	@cd shell; make
	@cd usr/app_sema; make
	@cd usr/app_print; make
	@cd usr/write_alpha; make
	@cd usr/argv; make
	@cd usr/philosopher; make
	@cd usr/fork_write; make
	@cd bin/stat; make
	@cd bin/cp; make
	@cd bin/ls; make
	@cd bin/cat; make
	@cd bin/mkdir; make
	@cd bin/rm; make
	@cd bin/touch; make
	@cd bin/rmdir; make
	@cd bin/mv; make
	@./utils/genFS/genFS shell/uMain.elf usr/app_sema/app_sema.elf usr/app_print/app_print.elf usr/write_alpha/write_alpha.elf usr/argv/argv.elf usr/philosopher/philosopher.elf usr/fork_write/fork_write.elf bin/stat/stat.elf bin/cp/cp.elf bin/ls/ls.elf bin/cat/cat.elf bin/mkdir/mkdir.elf bin/rm/rm.elf bin/touch/touch.elf bin/rmdir/rmdir.elf bin/mv/mv.elf
	cat bootloader/bootloader.bin kernel/kMain.elf fs.bin > os.img

play: os.img
	$(QEMU) -serial stdio os.img

debug: os.img
	$(QEMU) -serial stdio -s -S os.img

clean:
	@cd utils/genFS; make clean
	@cd bootloader; make clean
	@cd kernel; make clean
	@cd shell; make clean
	@cd usr/app_sema; make clean
	@cd usr/app_print; make clean
	@cd usr/write_alpha; make clean
	@cd usr/argv; make clean
	@cd usr/philosopher; make clean
	@cd usr/fork_write; make clean
	@cd bin/stat; make clean
	@cd bin/cp; make clean
	@cd bin/ls; make clean
	@cd bin/cat; make clean
	@cd bin/mkdir; make clean
	@cd bin/rm; make clean
	@cd bin/touch; make clean
	@cd bin/rmdir; make clean
	@cd bin/mv; make clean
	rm -f fs.bin os.img