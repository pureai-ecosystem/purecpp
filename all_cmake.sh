#!/bin/bash

# Marca o início
START_TIME=$(date +%s)

# Execuções dos módulos
./module_cmake.sh 1
./module_cmake.sh 2
./module_cmake.sh 3
./module_cmake.sh 4
./module_cmake.sh 5
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
