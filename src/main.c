#include "bancada.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "--gerador") == 0)
        return gerar_lote(argv[2], false);
    if (argc == 3 && strcmp(argv[1], "--gerador-interativo") == 0)
        return gerar_lote(argv[2], true);
    bool interativo = argc == 2 && strcmp(argv[1], "--interativo") == 0;
    if (argc != 1 && !interativo) { fprintf(stderr, "Uso: %s [--interativo]\n", argv[0]); return 2; }

    char nome[64];
    snprintf(nome, sizeof nome, "/bancada-%ld", (long)getpid());
    struct mq_attr atributos = {.mq_maxmsg = PLACAS, .mq_msgsize = sizeof(Ordem)};
    mqd_t fila = mq_open(nome, O_CREAT | O_EXCL | O_RDONLY, 0600, &atributos);
    if (fila == (mqd_t)-1) { perror("mq_open"); return 1; }

    pid_t filho = fork();
    if (filho == 0) {
        execl("/proc/self/exe", "bancada", interativo ? "--gerador-interativo" : "--gerador", nome, (char *)NULL);
        perror("exec");
        _exit(127);
    }
    int status = 0;
    pid_t esperado = -1;
    if (filho < 0) perror("fork");
    else {
        // O lote inteiro cabe na fila POSIX, mesmo antes da primeira leitura.
        do { esperado = waitpid(filho, &status, 0); }
        while (esperado < 0 && errno == EINTR);
        if (esperado < 0) perror("waitpid");
    }
    mq_unlink(nome);
    if (esperado < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        mq_close(fila);
        return 1;
    }

    printf("Controlador %ld recebeu o lote do gerador %ld.\n", (long)getpid(), (long)filho);
    iniciar_postos();
    int rc = 0;
    for (int i = 0; i < PLACAS; i++) {
        Ordem ordem;
        ssize_t n;
        do { n = mq_receive(fila, (char *)&ordem, sizeof ordem, NULL); }
        while (n < 0 && errno == EINTR);
        if (n != sizeof ordem) { perror("mq_receive"); rc = 1; break; }
        distribuir(ordem);
    }
    mq_close(fila);
    encerrar_postos();
    return rc;
}
