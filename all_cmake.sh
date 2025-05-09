#!/bin/bash

# Marca o início
START_TIME=$(date +%s)

# Execuções dos módulos
cd CMAKE_CHUNKS_CLEAN/
./o.sh
cd ..
cd CMAKE_CHUNKS/
./o.sh
cd ..
cd CMAKE_META/
./o.sh
cd ..
cd CMAKE_EXTRACT/
./o.sh
cd ..
cd CMAKE_LIBS/
./o.sh
cd ..

# Marca o fim
END_TIME=$(date +%s)

# Calcula o tempo total
ELAPSED_TIME=$((END_TIME - START_TIME))

echo "============================================================"
printf "\n"
echo "Tempo total de execução: $ELAPSED_TIME segundos"
printf "\n"
echo "============================================================"
echo "============================================================"
echo ""
echo "        ALL MODULES HAVE BEEN SUCCESSFULLY COMPILED!"
echo ""
echo "============================================================"
