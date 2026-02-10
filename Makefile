CC=gcc
PKGS=glib-2.0 cairo pango pangocairo gdk-pixbuf-2.0 sdl3
#CFLAGS=-Wall -g -pthread $(shell pkg-config --cflags $(PKGS))
CFLAGS=-Wall -g -pthread 
#LDFLAGS=-pthread -lgc -lgmp $(shell pkg-config --libs $(PKGS)) -lGL
LDFLAGS=-pthread -lgc -lgmp 
LEX=flex
YACC=bison
YFLAGS=-d

safelisp:  main.o safelisp_parser.yy.o safelisp_parser.tab.o safelisp.o printer.o rb-tree.o
	$(CC) $(CFLAGS) -o safelisp main.o safelisp_parser.yy.o safelisp_parser.tab.o safelisp.o printer.o $(LDFLAGS)

main.o: main.c safelisp_parser.tab.c safelisp_parser.tab.h safelisp_parser.yy.o
	$(CC) $(CFLAGS) -c main.c -o main.o 

rb-tree.o: rb-tree.c rb-tree.h rbtree_template.c rbtree_template.h 
	$(CC) $(CFLAGS) -c rb-tree.c -o rb-tree.o
%.o: %.c %.h 
	$(CC) $(CFLAGS) -c $< -o $@

safelisp_parser.yy.o: safelisp_parser.yy.c safelisp_parser.tab.h
	$(CC) $(CFLAGS) -c safelisp_parser.yy.c

safelisp_parser.tab.o: safelisp_parser.tab.c
	$(CC) $(CFLAGS) -c safelisp_parser.tab.c

safelisp_parser.yy.c: safelisp_parser.l
	$(LEX) -o safelisp_parser.yy.c --header-file=safelisp_parser.yy.h safelisp_parser.l

safelisp_parser.tab.c safelisp_parser.tab.h: safelisp_parser.y 
	$(YACC) $(YFLAGS) safelisp_parser.y

clean:
	rm -f safelisp safelisp_parser.tab.* safelisp_parser.yy.* main.o *.o
