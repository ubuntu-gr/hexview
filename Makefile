hexview: hexview.h con_color.h hexview.c
	gcc -g -Wall -Wextra hexview.c -o hexview

clean:
	rm -f hexview
