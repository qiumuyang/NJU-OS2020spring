QEMU = qemu-system-i386

os.img:
	@cd utils/genFS; make
	@cd bootloader; make
	@cd kernel; make
	@cd app; make
	@cd app_color; make
	@cd bounded_buffer; make
	@cd app_rand; make
	@cd app_scanf; make
	@cd app_sema; make
	@cd app_print; make
	@cd reader_writer; make
	@cd philosopher; make
	@cd app_shmem; make
	@./utils/genFS/genFS app/uMain.elf app_color/app_color.elf bounded_buffer/bounded_buffer.elf app_rand/app_rand.elf app_scanf/app_scanf.elf app_sema/app_sema.elf app_print/app_print.elf reader_writer/reader_writer.elf philosopher/philosopher.elf app_shmem/app_shmem.elf
	cat bootloader/bootloader.bin kernel/kMain.elf fs.bin > os.img

play: os.img
	$(QEMU) -serial stdio os.img

debug: os.img
	$(QEMU) -serial stdio -s -S os.img

clean:
	@cd utils/genFS; make clean
	@cd bootloader; make clean
	@cd kernel; make clean
	@cd app; make clean
	@cd app_color; make clean
	@cd bounded_buffer; make clean
	@cd app_rand; make clean
	@cd app_scanf; make clean
	@cd app_sema; make clean
	@cd app_print; make clean
	@cd reader_writer; make clean
	@cd philosopher; make clean
	@cd app_shmem; make clean
	rm -f fs.bin os.img