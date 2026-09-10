cmd_/home/jinhang/ics2026/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o := unused

source_/home/jinhang/ics2026/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o := src/nemu-main.c

deps_/home/jinhang/ics2026/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o := \
    $(wildcard include/config/target/am.h) \
  /home/jinhang/ics2026/nemu/include/common.h \
    $(wildcard include/config/mbase.h) \
    $(wildcard include/config/msize.h) \
    $(wildcard include/config/isa64.h) \
  /home/jinhang/ics2026/nemu/include/macro.h \
  /home/jinhang/ics2026/nemu/include/debug.h \
  /home/jinhang/ics2026/nemu/include/utils.h \
    $(wildcard include/config/target/native/elf.h) \

/home/jinhang/ics2026/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o: $(deps_/home/jinhang/ics2026/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o)

$(deps_/home/jinhang/ics2026/nemu/build/obj-riscv32-nemu-interpreter/src/nemu-main.o):
