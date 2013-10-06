all: http client

http:
	gcc http.c -o ./build/http

client:
	gcc client.c -o ./build/client
