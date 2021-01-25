FALSECLIENTS = 3
CFLAGS = -std=c18

.PHONY: test1
test1: server client client-fs server-fs
	for i in `seq $(FALSECLIENTS)`; do\
		mkfifo tests/client-fs$$i/client_pipe; \
		mkfifo tests/client-fs$$i/gui_pipe;\
		rsync tests/testfile* build/client build/clientui.py build/gui.py \
		src/run_client.sh tests/client-fs$$i; \
		sync;\
		done

server: src/scherzo-server.c fstrie.o fshandler.o reqzo.o \
	scherzo-util.o src/kidpool.h src/szdcue.h
	gcc $(CFLAGS) -pthread -o server src/scherzo-server.c fshandler.o reqzo.o \
		fstrie.o scherzo-util.o
	mv server build

client: src/scherzo-client.c reqzo.o scherzo-util.o
	gcc $(CFLAGS) -o client src/scherzo-client.c reqzo.o scherzo-util.o
	mv client build
	mkfifo build/client_pipe
	cp src/clientui.py build
	cp src/gui.py build


fstest: tests/fstest.c fshandler.o fstrie.o scherzo-fs
	gcc $(CFLAGS) -o fstest tests/fstest.c fshandler.o fstrie.o
	mv fstest tests

reqzo.o: src/reqzo.c src/reqzo.h
	gcc $(CFLAGS) -c src/reqzo.c

fshandler.o: src/fshandler.c src/fshandler.h src/szdcue.h
	gcc $(CFLAGS) -c src/fshandler.c

fstrie.o: src/fstrie.c src/fstrie.h
	gcc $(CFLAGS) -c src/fstrie.c

scherzo-util.o: src/scherzo-util.c src/scherzo-util.h
	gcc $(CFLAGS) -c src/scherzo-util.c


.PHONY: clean-all
clean-all: cleano scherzo-fs client-fs server-fs
	rm -rf build
	mkdir build
	rm -f tests/fstest

.PHONY: client-fs
client-fs:
	for i in `seq $(FALSECLIENTS)`; do \
		rm -rf tests/client-fs$$i; \
		mkdir tests/client-fs$$i; \
		done

.PHONY: server-fs
server-fs:
	rm -rf tests/server-fs
	mkdir tests/server-fs

.PHONY: scherzo-fs
scherzo-fs:
	rm -rf tests/scherzo-fs
	mkdir tests/scherzo-fs

.PHONY: cleano
cleano:
	rm -f *.o




