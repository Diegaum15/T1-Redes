OBJS = raw_socket.o define.o janela.o crc.o protocolo.o
CC = gcc 
FLAGS = -g -Wall

all: $(OBJS) client.o server.o
	$(CC) $(OBJS) client.o -o client $(FLAGS) 
	$(CC) $(OBJS) server.o -o server $(FLAGS) 

client: $(OBJS) client.o
	$(CC) $(OBJS) client.o -o client $(FLAGS)

server: $(OBJS) server.o
	$(CC) $(OBJS) server.o -o server $(FLAGS)

define.o : define.c define.h
	$(CC) -c $(FLAGS)  define.c

protocolo.o: protocolo.c protocolo.h
	$(CC) -c $(FLAGS)  protocolo.c

raw_socket.o: raw_socket.c 
	$(CC) -c $(FLAGS) raw_socket.c

crc.o: crc.c
	$(CC) -c $(FLAGS)  crc.c

janela.o: janela.c
	$(CC) -c $(FLAGS)  janela.c

client.o: client.c 
	$(CC) -c $(FLAGS)  client.c

server.o: server.c
	$(CC) -c $(FLAGS)  server.c
	
clean:
	rm -f $(OBJS) server.o client.o server client