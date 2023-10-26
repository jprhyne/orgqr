include make.inc

default: test.exe driver test_optBlas.exe

testDorgqr: test_mklDorgqr.exe test_refDorgqr.exe

test_optBlas.exe: my_dorgqr.o test.o test.c
	$(FC) $(CFLAGS) my_dorgqr.o test.o $(OPTBLAS) -o $@

my_dorgqr.o: my_dorgqr.f
	$(FC) -c $(CFLAGS) -I$(LAPACKEINC) -I$(CBLASINC) $^

my_dorgqr_v1.o: my_dorgqr_v1.f
	$(FC) -c $(CFLAGS) -I$(LAPACKEINC) -I$(CBLASINC) $^

my_dorgqr_v2.o: my_dorgqr_v2.f
	$(FC) -c $(CFLAGS) -I$(LAPACKEINC) -I$(CBLASINC) $^

test.o: test.c 
	$(CC) -c $(CFLAGS) -I$(LAPACKEINC) -I$(CBLASINC) $^

test.exe: my_dorgqr.o test.o test.c
	$(FC) $(CFLAGS) my_dorgqr.o test.o $(LAPACKELIB) $(LAPACKLIB) $(CBLASLIB) $(BLASLIB) -o $@

dorgqr.o: dorgqr.f
	$(FC) -c $(CFLAGS) -I$(LAPACKEINC) -I$(CBLASINC) $^

test_refDorgqr.exe: test.o dorgqr.o test.c
	$(FC) $(CFLAGS) test.o $(OPTBLAS) dorgqr.o -o $@

test_mklDorgqr.exe: test.o test.c
	$(FC) $(CFLAGS) test.o $(OPTBLAS) -o $@

test_v2.exe: test.o my_dorgqr_v2.o test.c
	$(FC) $(CFLAGS) test.o my_dorgqr_v2.o $(OPTBLAS) -o $@

test_v1.exe: test.o my_dorgqr_v1.o test.c
	$(FC) $(CFLAGS) test.o my_dorgqr_v1.o $(OPTBLAS) -o $@

driver: driver.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm *o driver test*exe
