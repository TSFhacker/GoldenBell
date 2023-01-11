all:
	gcc -rdynamic -o client `pkg-config --cflags mysqlclient` `pkg-config --cflags gtk+-3.0` client.c `pkg-config --libs mysqlclient` `pkg-config --libs gtk+-3.0`
	gcc -o server `pkg-config --cflags mysqlclient` server.c `pkg-config --libs mysqlclient`

clean:
	rm ./client ./server