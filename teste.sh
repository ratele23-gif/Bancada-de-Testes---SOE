#!/bin/sh
set -eu
saida=$(mktemp)
trap 'rm -f "$saida"' EXIT
./bancada > "$saida"
awk '
/início da medição/ { if (ativa) exit 1; ativa=$4; inicios++ }
/fim da medição/ { if (ativa != $4) exit 1; ativa=""; fins++ }
END { if (ativa || inicios != 3 || fins != 3) exit 1 }
' "$saida"
grep -Fxq 'P01 | 5020 mV | APROVADA' "$saida"
grep -Fxq 'P02 | 4620 mV | REPROVADA por subtensão' "$saida"
grep -Fxq 'P03 | 5190 mV | APROVADA' "$saida"
grep -Fxq 'Total: aprovadas=2, reprovadas=1.' "$saida"
echo 'OK: laudos corretos e medições exclusivas.'
printf '%s\n' 'abc' '4,749' '5.02' '5,251' | ./bancada --interativo > "$saida" 2>&1
grep -Fq 'Digite um valor' "$saida"
grep -Fxq 'P01 | 4749 mV | REPROVADA por subtensão' "$saida"
grep -Fxq 'P02 | 5020 mV | APROVADA' "$saida"
grep -Fxq 'P03 | 5251 mV | REPROVADA por sobretensão' "$saida"
printf '%s\n' '4,750' '5.250' '5' | ./bancada --interativo > "$saida"
grep -Fxq 'Total: aprovadas=3, reprovadas=0.' "$saida"
if printf '5\n' | ./bancada --interativo > "$saida" 2>&1; then exit 1; fi
grep -Fq 'Lote cancelado' "$saida"
echo 'OK: entrada interativa, limites inclusivos, três classificações e cancelamento.'
