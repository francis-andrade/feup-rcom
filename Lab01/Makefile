#TODO THIS
all: nserial

nserial: alarm.h alarm.c applicationlayer.c applicationlayer.h datalink.c datalink.h main.c utils.c utils.h #receiver.c sender.c
	gcc -Wall -pg alarm.c alarm.h applicationlayer.c applicationlayer.h datalink.c datalink.h main.c utils.c utils.h -o nserial

clean:
	rm -f nserial

from-git:
	rm -f nserial
	git pull && make
