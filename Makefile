CC=gcc
CFLAGS=-Wall -g -pthread $(shell pkg-config --cflags glib-2.0)
LDFLAGS=-pthread -lgc -lgmp $(shell pkg-config --libs glib-2.0)
LEX=flex
YACC=bison
YFLAGS=-d



taco:  main.o taco.yy.o taco.tab.o taco.o
	$(CC) $(CFLAGS) -o taco main.o taco.yy.o taco.tab.o $(LDFLAGS)

main.o: main.c taco.tab.c taco.tab.h taco.yy.o
	$(CC) $(CFLAGS) -c main.c -o main.o 

%.o: %.c %.h 
	$(CC) $(CFLAGS) -c $< -o $@

taco.yy.o: taco.yy.c taco.tab.h
	$(CC) $(CFLAGS) -c taco.yy.c

taco.tab.o: taco.tab.c
	$(CC) $(CFLAGS) -c taco.tab.c

taco.yy.c: taco.l
	$(LEX) -o taco.yy.c --header-file=taco.yy.h taco.l

taco.tab.c taco.tab.h: taco.y 
	$(YACC) $(YFLAGS) taco.y

clean:
	rm -f taco taco.tab.* taco.yy.* main.o *.o
