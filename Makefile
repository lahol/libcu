CC=gcc
CFLAGS += -Wall -O3 -I. -g
LIBS=-lpthread -lrt

PREFIX := /usr

CU_SRC_FULL := $(wildcard *.c)
cu_SRC := $(filter-out test.c, $(CU_SRC_FULL))
cu_OBJ := $(cu_SRC:.c=.o)
cu_HEADERS := $(wildcard *.h)



all: libcu.so.1.0 test

libcu.so.1.0: $(cu_OBJ)
#	$(AR) cvr -o $@ $^
	$(CC) -shared -Wl,-soname,libcu.so.1 -o $@ $^ $(LIBS)
	ln -sf libcu.so.1.0 libcu.so.1
	ln -sf libcu.so.1 libcu.so

test: test.o cu-heap.o cu-memory.o cu-list.o

%.o: %.c $(cu_HEADERS)
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	$(RM) -f libcu.so* *.o

install:
	install libcu.so.1.0 $(PREFIX)/lib/
	ln -sf $(PREFIX)/lib/libcu.so.1.0 $(PREFIX)/lib/libcu.so.1
	ln -sf $(PREFIX)/lib/libcu.so.1 $(PREFIX)/lib/libcu.so
	cp $(cu_HEADERS) $(PREFIX)/include

.PHONY: all clean install
