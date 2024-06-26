OBJS =  client.o raw_socket.o define.o janela.o
SAIDA = client
CC = gcc 
FLAGS = -g -Wall

all: $(OBJS)
	$(CC) $(OBJS) -o $(SAIDA) $(FLAGS) 

define.o : define.c define.h
	$(CC) -c $(FLAGS)  define.c

raw_socket.o: raw_socket.c 
	$(CC) -c $(FLAGS) raw_socket.c

janela.o: janela.c
	$(CC) -c $(FLAGS)  janela.c

client.o: client.c 
	$(CC) -c $(FLAGS)  client.c
	
clean:
	rm -f $(OBJS) $(SAIDA)