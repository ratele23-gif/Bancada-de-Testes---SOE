#include "bancada.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Um único lote com até PLACAS ordens. A retirada avança o índice "lidas".
static Ordem ordens[PLACAS];
static int recebidas, lidas;
static bool encerrada;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t trabalho = PTHREAD_COND_INITIALIZER;
static sem_t instrumento;
static pthread_t threads[POSTOS];
static int numeros[POSTOS], leituras[PLACAS];

static void conferir(int erro, const char *chamada) {
    if (erro) { errno = erro; perror(chamada); exit(EXIT_FAILURE); }
}

static void pausa(long ms) {
    struct timespec tempo = {ms / 1000, (ms % 1000) * 1000000L};
    while (nanosleep(&tempo, &tempo) < 0 && errno == EINTR) {}
}

static void *testar(void *arg) {
    int posto = *(int *)arg;
    for (;;) {
        pthread_mutex_lock(&mutex);
        while (lidas == recebidas && !encerrada)
            pthread_cond_wait(&trabalho, &mutex);
        if (lidas == recebidas) {
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        Ordem placa = ordens[lidas++];
        pthread_mutex_unlock(&mutex);

        printf("Posto %d | P%02d | inspeção visual simulada\n", posto, placa.id);
        pausa(300);
        printf("Posto %d | P%02d | solicitou o multímetro\n", posto, placa.id);
        int rc;
        do { rc = sem_wait(&instrumento); } while (rc < 0 && errno == EINTR);
        if (rc < 0) conferir(errno, "sem_wait");
        printf("Posto %d | P%02d | início da medição\n", posto, placa.id);
        pausa(650);
        leituras[placa.id - 1] = placa.milivolts;
        printf("Posto %d | P%02d | fim da medição (%d mV)\n", posto, placa.id, placa.milivolts);
        if (sem_post(&instrumento) < 0) conferir(errno, "sem_post");
    }
}

void iniciar_postos(void) {
    if (sem_init(&instrumento, 0, 1) < 0) conferir(errno, "sem_init");
    for (int i = 0; i < POSTOS; i++) {
        numeros[i] = i + 1;
        conferir(pthread_create(&threads[i], NULL, testar, &numeros[i]), "pthread_create");
    }
}

void distribuir(Ordem placa) {
    pthread_mutex_lock(&mutex);
    ordens[recebidas++] = placa;
    pthread_cond_signal(&trabalho);
    pthread_mutex_unlock(&mutex);
}

void encerrar_postos(void) {
    pthread_mutex_lock(&mutex);
    encerrada = true;
    pthread_cond_broadcast(&trabalho);
    pthread_mutex_unlock(&mutex);
    for (int i = 0; i < POSTOS; i++)
        conferir(pthread_join(threads[i], NULL), "pthread_join");

    int aprovadas = 0;
    puts("\nLAUDO DO LOTE | faixa inclusiva de 4750 a 5250 mV");
    for (int i = 0; i < recebidas; i++) {
        int mv = leituras[ordens[i].id - 1];
        const char *resultado = "APROVADA";
        if (mv < MIN_MV) resultado = "REPROVADA por subtensão";
        else if (mv > MAX_MV) resultado = "REPROVADA por sobretensão";
        else aprovadas++;
        printf("P%02d | %d mV | %s\n", ordens[i].id, mv, resultado);
    }
    printf("Total: aprovadas=%d, reprovadas=%d.\n", aprovadas, recebidas - aprovadas);
    sem_destroy(&instrumento);
    pthread_cond_destroy(&trabalho);
    pthread_mutex_destroy(&mutex);
}
