all:
	gcc -g -Wall -c bson_parser.c hashmap/hashmap.c test.c
	gcc bson_parser.o hashmap.o test.o -o debug
