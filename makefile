OBJS = protocolo.o raw_socket.o janela.o crc.o
CC = gcc 
FLAGS = -g -Wall

all: $(OBJS) client.o server.o
	$(CC) $(OBJS) client.o -o client $(FLAGS) 
	$(CC) $(OBJS) server.o -o server $(FLAGS) 

client: $(OBJS) client.o
	$(CC) $(OBJS) client.o -o client $(FLAGS)

server: $(OBJS) server.o
	$(CC) $(OBJS) server.o -o server $(FLAGS)

protocolo.o: protocolo.c protocolo.h
	$(CC) -c $(FLAGS) protocolo.c

raw_socket.o: raw_socket.c raw_socket.h
	$(CC) -c $(FLAGS) raw_socket.c

crc.o: crc.c crc.h
	$(CC) -c $(FLAGS) crc.c

janela.o: janela.c janela.h
	$(CC) -c $(FLAGS) janela.c

client.o: client.c client.h
	$(CC) -c $(FLAGS) client.c

server.o: server.c server.h
	$(CC) -c $(FLAGS) server.c
	
clean:
	rm -f $(OBJS) server.o client.o server client