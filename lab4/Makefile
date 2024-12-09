all: temp_logger emul

temp_logger: temp_logger.c
	gcc -g temp_logger.c -o temp_logger -I.

emul: temp_emul.c
	gcc -g temp_emul.c -o emul -I.