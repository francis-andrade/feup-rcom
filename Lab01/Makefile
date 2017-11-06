#TODO THIS
all: nserial

nserial: alarm.h alarm.c applicationlayer.c applicationlayer.h datalink.c datalink.h main.c
	gcc -Wall -pg alarm.c alarm.h applicationlayer.c applicationlayer.h datalink.c datalink.h main.c -o nserial

clean:
	rm -f nserial

from-git:
	rm -f nserial
	git pull && make
