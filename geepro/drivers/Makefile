## $Revision: 1.1.1.1 $
# 

CC  = gcc
OPT = -Wall -fPIC -g -c

ALL:
	for k in $$(ls *.c | sed 's/\.c/\.plug/g'); do make $$k; done

%.plug:%.o
	gcc -g -shared -Wl,-soname,\$*.plug -o \$*.plug \$*.o -lc

%.o: %.c
	\$(CC) \$(OPT) \$*.c

clean:
	rm -f *.o
	rm -f *.plug

