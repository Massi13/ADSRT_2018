CC=gcc
CFLAGS=arm-linux-gnueabi


all: compilar
compilar: capturaV5.o enviar.o conectar.o
	
conectar.o:
	ssh root@172.28.10.5

capturaV5.o: capturaV5.c
	$(CC) $(CFLAGS) capturaV5.c -o capturaV5 -L. -lsqlite3 -I/usr/include -lrt

enviar.o: 
	scp capturaV5 root@172.28.10.5:/tmp 
