usersmanager: um.c
	gcc -o um um.c groups.c users.c manager.c `pkg-config --cflags --libs gtk+-3.0 gio-2.0`
#	cd inform &&	pdflatex inform.tex
