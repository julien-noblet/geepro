## $Revision: 1.2 $

.PHONY:clean

ALL:
	make -C ../
	ln -s ../geepro ./geepro



clean:
	make -C ../ clean
	rm -f ./geepro

run_memcheck:
	valgrind --leak-check=yes ./geepro


