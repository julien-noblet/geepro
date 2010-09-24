## $Revision: 1.4 $

debug = -g

opts = $(debug) -Wall -O2 

gtk_conf=`pkg-config --cflags gtk+-2.0 cairo libxml-2.0`
gtk_version=$(pkg-config --modversion gtk+-2.0)

#if $(gtk_version) < 2.10
extra=-D NO_PRINTER_SUPPORT=1
#echo "No printer"
#endif

OBJS = gui.o bineditor.o checksum.o gui_xml.o

.PHONY:clean

ALL: libgui.a

libgui.a: $(OBJS)
	ar rcs libgui.a $(OBJS)

gui.o: gui.c gui.h icons_xpm.c
	gcc -c $(opts) $(gtk_conf) gui.c

bineditor.o: bineditor.c bineditor.h checksum.h
	gcc -c $(opts) $(gtk_conf) bineditor.c $(extra)

checksum.o: checksum.c checksum.h 
	gcc -c $(opts) checksum.c

gui_xml.o: gui_xml.c gui_xml.h 
	gcc -c $(opts) $(gtk_conf) gui_xml.c

clean:
	rm -f *.o
	rm -f *.a

