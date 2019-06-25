

.PHONY: all run clean

ifdef DEBUG
CFLAGS += -g
endif

all: a

run: a
	./a
clean:
	rm -f a

run-bind: a
	#numactl --physcpubind=1 ./a
	taskset -c 1 ./a

