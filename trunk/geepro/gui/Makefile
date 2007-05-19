## $Revision: 1.2 $

debug = -g

opts = $(debug) -Wall -O2 

gtk_conf=`pkg-config --cflags gtk+-2.0 cairo`
gtk_version=$(pkg-config --modversion gtk+-2.0)

#if $(gtk_version) < 2.10
extra=-D NO_PRINTER_SUPPORT=1
#echo "No printer"
#endif

OBJS = gui.o bineditor.o checksum.o

.PHONY:clean

ALL: libgui.a

libgui.a: $(OBJS)
	ar rcs libgui.a $(OBJS)

gui.o: gui.c gui.h 
	gcc -c $(opts) $(gtk_conf) gui.c

bineditor.o: bineditor.c bineditor.h checksum.h
	gcc -c $(opts) $(gtk_conf) bineditor.c $(extra)

checksum.o: checksum.c checksum.h 
	gcc -c $(opts) checksum.c


clean:
	rm -f *.o
	rm -f *.a

