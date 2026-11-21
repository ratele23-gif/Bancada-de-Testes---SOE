#include "bancada.h"
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>

// Converte a representação decimal diretamente para milivolts inteiros.
static bool converter(const char *texto, int *resultado) {
    while (isspace((unsigned char)*texto)) texto++;
    if (!isdigit((unsigned char)*texto)) return false;
    char *fim;
    errno = 0;
    long volts = strtol(texto, &fim, 10);
    if (errno || volts > INT_MAX / 1000) return false;
    int fracao = 0;
    if (*fim == ',' || *fim == '.') {
        fim++;
        if (!isdigit((unsigned char)*fim)) return false;
        for (int peso = 100; peso && isdigit((unsigned char)*fim); peso /= 10)
            fracao += (*fim++ - '0') * peso;
    }
    while (isspace((unsigned char)*fim)) fim++;
    if (*fim || volts * 1000 > INT_MAX - fracao) return false;
    *resultado = (int)volts * 1000 + fracao;
    return true;
}

static bool ler_tensao(Ordem *placa) {
    char *linha = NULL;
    size_t capacidade = 0;
    for (;;) {
        printf("P%02d, tensão em volts: ", placa->id);
        fflush(stdout);
        if (getline(&linha, &capacidade, stdin) < 0) {
            free(linha);
            fputs("Entrada encerrada. Lote cancelado.\n", stderr);
            return false;
        }
        if (converter(linha, &placa->milivolts)) {
            free(linha);
            return true;
        }
        fputs("Digite um valor positivo ou zero, com até três casas decimais (ex.: 5,02).\n", stderr);
    }
}

int gerar_lote(const char *nome, bool interativo) {
    Ordem lote[PLACAS] = {{1, 5020}, {2, 4620}, {3, 5190}};
    if (interativo) {
        puts("Informe as três tensões simuladas. A faixa aprovada é de 4,75 a 5,25 V.");
        for (int i = 0; i < PLACAS; i++)
            if (!ler_tensao(&lote[i])) return 1;
        puts("Lote registrado.");
    }
    mqd_t fila = mq_open(nome, O_WRONLY);
    if (fila == (mqd_t)-1) { perror("gerador: mq_open"); return 1; }
    for (int i = 0; i < PLACAS; i++) {
        int rc;
        do {
            rc = mq_send(fila, (const char *)&lote[i], sizeof(Ordem), 0);
        } while (rc < 0 && errno == EINTR);
        if (rc < 0) { perror("mq_send"); mq_close(fila); return 1; }
    }
    mq_close(fila);
    return 0;
}
