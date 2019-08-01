
CFLAGS = -O3 -Wall

CCOMP = gcc

hash_dict.o : hash_dict.c
	$(CCOMP) -c -I. $(CFLAGS) hash_dict.c

clean:
	rm -f *.o
