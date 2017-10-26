#TODO THIS
all: nserial

nserial: alarm.c alarm.h applicationlayer.c applicationlayer.h datalink.c datalink.h main.c receiver.c sender.c utils.c utils.h
	gcc -Wall alarm.c alarm.h applicationlayer.c applicationlayer.h datalink.c datalink.h main.c receiver.c sender.c utils.c utils.h -o nserial

clean:
	rm -f nserial
