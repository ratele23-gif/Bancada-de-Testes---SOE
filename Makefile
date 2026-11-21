CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
.PHONY: demo interativo clean test
bancada: src/main.c src/gerador.c src/postos.c src/bancada.h
	$(CC) -std=c11 -D_POSIX_C_SOURCE=200809L $(CFLAGS) -pthread src/main.c src/gerador.c src/postos.c -lrt -o $@
demo: bancada
	./bancada
interativo: bancada
	./bancada --interativo
test: bancada
	sh teste.sh
clean:
	$(RM) bancada
