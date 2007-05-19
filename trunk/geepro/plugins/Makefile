## $Revision: 1.2 $

CC  = gcc
OPT = -Wall -fPIC -g -c

.PHONY: clean

ALL:
	for k in $$(ls *.c | sed 's/\.c/\.module/g'); do make $$k; done

%.module:%.o
	gcc -g -shared -Wl,-soname,\$*.module -o \$*.module \$*.o -lc

%.o: %.c
	\$(CC) \$(OPT) \$*.c

clean:
	rm -f *.o
	rm -f *.module

