## $Revision: 1.6 $

progname=geepro

gtk_libs=`pkg-config --libs gtk+-2.0 cairo libxml-2.0`

plugins=make_drivers make_plugins
local_libs = -L./gui  -lgui
statlibs= libgui.a 

objs=files.o buffer.o chip.o dummy.o iface.o timer.o parport.o main.o storings.o
objs_lok= ./src/files.o ./src/buffer.o ./src/chip.o ./src/dummy.o ./src/iface.o ./src/timer.o ./src/parport.o ./src/main.o ./src/storings.o
MAKE_EXT= make -C

UDEV_RULE= "KERNEL==\\\"parport[0-9]*\\\", GROUP=\\\"lp\\\", MODE=\\\"0666\\\""
TARGET_RULE = /etc/udev/rules.d/10-local.rules
TMP_FILE = _rule_


.PHONY: clean
.PHONY: install
.PHONY: dist
.PHONY: parport_udev_rule

ALL: lang.h $(progname) documentation

lang.h: eng

$(progname): $(plugins) $(statlibs) $(objs)
	gcc -Wall -o $(progname) $(objs_lok)  $(gtk_libs) $(local_libs) -ldl -rdynamic

documentation:
	$(MAKE_EXT) ./doc

eng: 
	$(MAKE_EXT) ./intl eng

pl: 
	$(MAKE_EXT) ./intl pl

files.o:
	$(MAKE_EXT) ./src files.o

buffer.o:
	$(MAKE_EXT) ./src buffer.o

chip.o:
	$(MAKE_EXT) ./src chip.o

dummy.o:
	$(MAKE_EXT) ./src dummy.o

iface.o:
	$(MAKE_EXT) ./src iface.o

timer.o:
	$(MAKE_EXT) ./src timer.o

parport.o:
	$(MAKE_EXT) ./src parport.o

main.o:
	$(MAKE_EXT) ./src main.o

storings.o:
	$(MAKE_EXT) ./src storings.o

libgui.a:
	$(MAKE_EXT) ./gui

make_drivers:
	$(MAKE_EXT) ./drivers

make_plugins:
	$(MAKE_EXT) ./plugins

clean:
	$(RM) $(progname)
	$(MAKE_EXT) ./drivers clean
	$(MAKE_EXT) ./plugins clean
	$(MAKE_EXT) ./gui clean
	$(MAKE_EXT) ./src clean
	$(MAKE_EXT) ./doc clean
	$(MAKE_EXT) ./intl clean
	$(RM) ./debug/$(progname)

install:
	echo "No install available yet"

dist:
	echo "No dist package build implemented yet"

parport_udev_rule:
	echo "echo \"$(UDEV_RULE)\" >> $(TARGET_RULE)" > /tmp/$(TMP_FILE)
	chmod +x /tmp/$(TMP_FILE)
	sudo sh /tmp/$(TMP_FILE)
	sudo udevadm control --reload-rules
	sudo udevadm trigger
	rm /tmp/$(TMP_FILE)
