## $Revision: 1.2 $

progname=geepro

gtk_libs=`pkg-config --libs gtk+-2.0 cairo`

plugins=make_drivers make_plugins
local_libs = -L./gui  -lgui
statlibs= libgui.a 

objs=files.o buffer.o chip.o dummy.o iface.o timer.o parport.o main.o
objs_lok= ./src/files.o ./src/buffer.o ./src/chip.o ./src/dummy.o ./src/iface.o ./src/timer.o ./src/parport.o ./src/main.o


ALL: lang.h $(progname) documentation

lang.h: pl

$(progname): $(plugins) $(statlibs) $(objs)
	gcc -Wall -o $(progname) $(objs_lok)  $(gtk_libs) $(local_libs) -ldl -rdynamic

documentation:
	make -C ./doc

eng: 
	make -C ./intl eng

pl: 
	make -C ./intl pl

files.o:
	make -C ./src files.o

buffer.o:
	make -C ./src buffer.o

chip.o:
	make -C ./src chip.o

dummy.o:
	make -C ./src dummy.o

iface.o:
	make -C ./src iface.o

timer.o:
	make -C ./src timer.o

parport.o:
	make -C ./src parport.o

main.o:
	make -C ./src main.o

libgui.a:
	make -C ./gui

make_drivers:
	make -C ./drivers

make_plugins:
	make -C ./plugins

clean:
	rm -f $(progname)
	make -C ./drivers clean
	make -C ./plugins clean
	make -C ./gui clean
	make -C ./src clean
	make -C ./doc clean
	make -C ./intl clean
	rm -f ./debug/$(progname)

