#ifndef BANCADA_H
#define BANCADA_H
#include <stdbool.h>

enum { PLACAS = 3, POSTOS = 3, MIN_MV = 4750, MAX_MV = 5250 };
typedef struct { int id, milivolts; } Ordem;

int gerar_lote(const char *nome, bool interativo);
void iniciar_postos(void);
void distribuir(Ordem placa);
void encerrar_postos(void);

#endif
