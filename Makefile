usersmanager: um.c
	gcc -o um um.c `pkg-config --cflags --libs gtk+-3.0 gio-2.0`
