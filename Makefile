all: komisja serwer raport

komisja: komisja.o err.o
	cc -Wall -o komisja komisja.o err.o

komisja.o: komisja.c err.h message.h
	cc -Wall -c komisja.c

serwer: serwer.o err.o
	gcc -pthread -Wall -o serwer serwer.o err.o

serwer.o: serwer.c err.h message.h
	cc -Wall -lpthread -c serwer.c

raport: raport.o err.o
	cc -Wall -o raport raport.o err.o

raport.o: raport.c err.h message.h
	cc -Wall -c raport.c

err.o: err.c err.h
	cc -Wall -c err.c

clean:
	rm -f *.o raport komisja serwer
