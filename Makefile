hibernate-handler: hibernate-handler.c
	gcc -o hibernate-handler hibernate-handler.c

.PHONY: clean

clean:
		rm -f hibernate-handler
