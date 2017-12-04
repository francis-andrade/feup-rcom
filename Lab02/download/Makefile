all: download

download: download.c sockets.c url.c utils.c sockets.h url.h utils.h
	gcc -Wall -pg download.c sockets.c url.c utils.c sockets.h url.h utils.h -o download

clean:
	rm -f download

from-git:
	rm -f download
	git pull && make

