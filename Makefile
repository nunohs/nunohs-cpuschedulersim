EXE=allocate
CC=gcc
CFLAGS=-Wall

$(EXE): allocate.c
	$(CC) $(CFLAGS) -o $(EXE) allocate.c -lm

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o $(EXE)

format:
	clang-format -i *.c *.h