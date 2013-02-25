all:
	gcc -g -Wextra -Wall -c bson_parser.c hashmap/hashmap.c test.c
	gcc -g bson_parser.o hashmap.o test.o -o debug
