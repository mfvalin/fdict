
CFLAGS = -O3 -Wall

CCOMP = gcc

FCOMP = gfortran

DEBUG = 0

hash_dict.o : hash_dict.c
	$(CCOMP) -DDEBUG=$(DEBUG) -c -I. $(CFLAGS) hash_dict.c

test:
	$(CCOMP) -DDEBUG=$(DEBUG) -I. $(CFLAGS) -DSELF_TEST hash_dict.c -o fdict_test.Abs

ftest: hash_dict.o
	$(CCOMP) -DDEBUG=$(DEBUG) -c -I. $(CFLAGS) hash_dict.c
	$(FCOMP) -DDEBUG=$(DEBUG) -I. ftest.F90 hash_dict.o  -o ftest.Abs

clean:
	rm -f *.o *.Abs
