CC=gcc
CFLAGS += -Wall -O3 -I. -g
LIBS=-lpthread -lrt

PREFIX := /usr

cu_SRC_FULL := $(wildcard *.c)
cu_SRC := $(filter-out bm-fixed-mem.c test.c, $(cu_SRC_FULL))
cu_OBJ := $(cu_SRC:.c=.o)
cu_HEADERS := $(wildcard *.h)



all: libcu.so.1.0 bm-fixed-mem test

libcu.so.1.0: $(cu_OBJ)
#	$(AR) cvr -o $@ $^
	$(CC) -shared -Wl,-soname,libcu.so.1 -o $@ $^ $(LIBS)
	ln -sf libcu.so.1.0 libcu.so.1
	ln -sf libcu.so.1 libcu.so

bm-fixed-mem: bm-fixed-mem.o cu-list.o cu-memory.o cu-avl-tree.o cu-stack.o cu-heap.o cu-fixed-stack.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test: test.o cu-heap.o cu-memory.o cu-list.o cu-avl-tree.o cu-stack.o cu-fixed-stack.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c $(cu_HEADERS)
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	$(RM) -f libcu.so* *.o bm-fixed-mem

install:
	install libcu.so.1.0 $(PREFIX)/lib/
	ln -sf $(PREFIX)/lib/libcu.so.1.0 $(PREFIX)/lib/libcu.so.1
	ln -sf $(PREFIX)/lib/libcu.so.1 $(PREFIX)/lib/libcu.so
	cp $(cu_HEADERS) $(PREFIX)/include

.PHONY: all clean install
