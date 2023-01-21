all:
	gcc -rdynamic -g -o client `pkg-config --cflags mysqlclient` `pkg-config --cflags gtk+-3.0` client.c protocol.c `pkg-config --libs mysqlclient` `pkg-config --libs gtk+-3.0`
	gcc -g -o server `pkg-config --cflags mysqlclient` server.c protocol.c `pkg-config --libs mysqlclient`

clean:
	rm ./client ./server