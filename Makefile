
CFLAGS = -O3 -Wall

CCOMP = gcc

FCOMP = gfortran

hash_dict.o : hash_dict.c
	$(CCOMP) -c -I. $(CFLAGS) hash_dict.c

test:
	$(CCOMP) -I. $(CFLAGS) -DSELF_TEST hash_dict.c -o fdict_test.Abs

ftest: hash_dict.o
	$(FCOMP) -I. ftest.F90 hash_dict.o  -o ftest.Abs

clean:
	rm -f *.o *.Abs
