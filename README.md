# Bancada de testes de placas eletrônicas

Simulação em C de uma bancada de 5 V. Três postos inspecionam placas em paralelo e compartilham um multímetro simulado. O modo automático usa um lote fixo. No modo interativo você escolhe a tensão simulada de cada placa. A ordem dos eventos depende do escalonador, mas cada placa deve produzir o mesmo laudo a cada execução.

## Execução

Em Linux com /proc, suporte a filas POSIX, compilador C e GNU Make (por exemplo, Ubuntu ou Ubuntu no WSL2), abra um terminal na pasta deste README.

```sh
make demo
make test
```

O primeiro comando compila e executa. O segundo confere os laudos e percorre o registro para detectar sobreposição de medições. Use `make clean` para remover o binário. Em Ubuntu, GCC e Make fazem parte do pacote `build-essential`.

## Modo interativo

Execute `make interativo` e digite uma tensão em volts para cada placa, pressionando Enter após cada valor. Use vírgula ou ponto e até três casas decimais. Experimente 4,62, 5,02 e 5,40 para observar subtensão, aprovação e sobretensão no mesmo lote. Uma entrada inválida permite nova tentativa. Ctrl+D durante a entrada cancela o lote.

## Organização e sincronização

| Componente | Responsabilidade |
| --- | --- |
| src/main.c | Cria uma fila POSIX exclusiva da execução. Usa fork e exec para iniciar o gerador, confere seu término com waitpid e distribui as ordens recebidas por mq_receive. |
| src/gerador.c | Envia três ordens via mq_send. Cada uma carrega identificador e tensão inteira em milivolts. O lote inteiro cabe na fila antes de o pai chamar waitpid. |
| src/postos.c | Implementa a fila interna e cria três trabalhadores com pthread_create. O mutex protege os índices e o estado de encerramento. Cada retirada atribui uma placa a um único posto. |
| src/bancada.h | Define o formato das ordens, limites de tensão e tamanho do lote. |
| teste.sh | Verifica os eventos de início e fim e o resultado individual das três placas. |

O controlador é produtor da fila interna. Cada posto consumidor verifica em um laço se há trabalho. Com a fila vazia, pthread_cond_wait libera o mutex enquanto espera. A chegada de uma ordem sinaliza a condição. Ao encerrar as entradas, o controlador acorda todos com pthread_cond_broadcast. Os trabalhadores esgotam a fila antes de sair.

A inspeção ocorre fora do mutex. Um semáforo POSIX iniciado com valor 1 limita o acesso ao multímetro entre sem_wait e sem_post. Só depois de pthread_join o controlador lê os resultados e imprime o laudo por ordem de entrada.

## Cenário de demonstração

A regra do exercício aceita de 4750 a 5250 mV, inclusive.

| Placa | Leitura simulada | Resultado esperado |
| --- | ---: | --- |
| P01 | 5020 mV | Aprovada |
| P02 | 4620 mV | Reprovada por subtensão |
| P03 | 5190 mV | Aprovada |

Execute a demonstração duas vezes. Localize uma solicitação de multímetro enquanto outra placa ocupa o instrumento. Confirme que os intervalos de medição são exclusivos mesmo quando a ordem dos postos muda. Veja o roteiro de seis minutos em ROTEIRO.md.

## Apresentação

Abra Apresentacao.html no navegador e avance com Page Down. O roteiro de seis minutos acompanha os seis slides.
