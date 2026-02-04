CC=gcc
CFLAGS=-Wall -g -pthread $(shell pkg-config --cflags glib-2.0)
LDFLAGS=-pthread -lgc -lgmp $(shell pkg-config --libs glib-2.0)
LEX=flex
YACC=bison
YFLAGS=-d

taco:  main.o taco_parser.yy.o taco_parser.tab.o taco.o printer.o
	$(CC) $(CFLAGS) -o taco main.o taco_parser.yy.o taco_parser.tab.o taco.o printer.o $(LDFLAGS)

main.o: main.c taco_parser.tab.c taco_parser.tab.h taco_parser.yy.o
	$(CC) $(CFLAGS) -c main.c -o main.o 

%.o: %.c %.h 
	$(CC) $(CFLAGS) -c $< -o $@

taco_parser.yy.o: taco_parser.yy.c taco_parser.tab.h
	$(CC) $(CFLAGS) -c taco_parser.yy.c

taco_parser.tab.o: taco_parser.tab.c
	$(CC) $(CFLAGS) -c taco_parser.tab.c

taco_parser.yy.c: taco_parser.l
	$(LEX) -o taco_parser.yy.c --header-file=taco_parser.yy.h taco_parser.l

taco_parser.tab.c taco_parser.tab.h: taco_parser.y 
	$(YACC) $(YFLAGS) taco_parser.y

clean:
	rm -f taco taco_parser.tab.* taco_parser.yy.* main.o *.o
