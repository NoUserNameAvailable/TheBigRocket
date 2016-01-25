all: serveur2.o serveur2_thread.o client2.o
 
serveur2.o: serveur2.c 
	gcc serveur2.c -o serveur2 

serveur2_thread.o: serveur2_thread.c
	gcc serveur2_thread.c -o serveur2_thread -pthread

client2.o: client2.c
	gcc client2.c -o client2
       
clean: 
	rm -rf *.o 

mrproper: clean 
	rm -rf moon_project