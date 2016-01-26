all: serveur2.o serveur2_thread.o client2.o
 
serveur2.o: serveur2.c 
	gcc serveur2.c -o serveur2.o 

serveur2_thread.o: serveur2_thread.c
	gcc serveur2_thread.c -o serveur2_thread.o -pthread

client2.o: client2.c
	gcc client2.c -o client2.o
       
clean: 
	rm -rf *.o 

