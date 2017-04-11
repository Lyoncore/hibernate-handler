hibernate-handler: hibernate-handler.c hibernate-handler.h modprobe_file_parse.c modprobe_file_parse.h
	gcc -o hibernate-handler hibernate-handler.c modprobe_file_parse.c

.PHONY: clean

clean:
		rm -f hibernate-handler
