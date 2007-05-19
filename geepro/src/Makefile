## $Revision: 1.2 $

#objs = buffer.o timer.o main.o chip.o parport.o iface.o dummy.o

opts = -Wall -O2

.PHONY:clean

libskel.a: $(objs)
	ar rcs libskel.a $(objs)

chip.o: chip.c chip.h
	gcc -c $(opts) chip.c

timer.o: timer.c timer.h
	gcc -c $(opts) timer.c

parport.o: parport.c parport.h
	gcc -c $(opts) parport.c 

main.o: main.c main.h
	gcc -c $(opts)  main.c

buffer.o:buffer.c buffer.h
	gcc -c $(opts) buffer.c

files.o: files.c files.h
	gcc -c $(opts) files.c

iface.o: iface.c iface.h 
	gcc -c $(opts) iface.c

dummy.o: dummy.c dummy.h 
	gcc -c $(opts) dummy.c

clean:
	rm -f *.o
	rm -f *.a

