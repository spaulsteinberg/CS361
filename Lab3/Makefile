#Makefile for MZOS
#COSC361
#Spring 2019
#Stephen Marz
#6 January 2019
RV=riscv64-unknown-linux-gnu
CFLAGS=-Wall -g -O0 -march=rv32ima -mabi=ilp32 -I. -Tvirt.lds -ffreestanding -nostdlib -nostartfiles -Lmzos -L.
SOURCES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SOURCES))
LIB=mzos
OUT=cs361_kernel.elf
QEMU=qemu-system-riscv32
QARGS=-machine virt -m 128M -nographic -serial mon:stdio -kernel $(OUT)

all: $(OUT)

$(OUT): $(OBJS) Makefile
	$(RV)-g++ $(CFLAGS) -o $(OUT) $(OBJS) -l$(LIB)

%.o: %.cpp Makefile
	$(RV)-g++ $(CFLAGS) -c $< -o $@


.PHONY: clean run
run: $(OUT)
	$(QEMU) $(QARGS)
susp: $(OUT)
	$(QEMU) $(QARGS) -S
gdb: $(OUT)
	$(QEMU) $(QARGS) -S -s
test:
	$(MAKE) -C mzos clean
	$(MAKE) -C mzos
	$(MAKE) clean
	$(MAKE) run
testg:
	$(MAKE) -C mzos clean
	$(MAKE) -C mzos
	$(MAKE) clean
	$(MAKE) gdb
clean:
	rm -f $(OUT) $(OBJS)
