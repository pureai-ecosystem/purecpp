#!/bin/bash

print_m() {
    local num="$1"     # Captura o primeiro argumento (versão do Python)
    local modulo="$2"  # Captura o segundo argumento (nome do módulo)

    # Determina o comprimento máximo da string
    local titulo_modulo="MODULE ${modulo}"
    
    # Calcula o tamanho máximo necessário
    if [ ${#titulo_modulo} -gt $max_len ]; then
        max_len=${#titulo_modulo}
    fi

    # Adiciona margem para manter uma borda visualmente agradável
    local total_len=$((max_len + 4))

    # Gera a linha de separação com base no tamanho necessário
    local separator=$(printf "%*s" "$total_len" | tr ' ' '-')

    # Imprime a saída formatada
    printf "\n"
    echo "$separator"
    printf "%s%*s%s\n" "| " $((total_len - 4)) "$titulo_modulo" " |"
    echo "$separator"
}

if [ "$#" -eq 0 ]; then
    echo "MÓDULO inválido. Use 1, 2, 3, 4, 5"
    echo "--------------------------------------------------------------------------"
    echo "  1 => purecpp_libs"
    echo "  2 => purecpp_meta"
    echo "  3 => purecpp_embed"
    echo "  4 => purecpp_extract"
    echo "  5 => purecpp_chunks_clean"
    echo "--------------------------------------------------------------------------"
else
    case "$1" in
        1)  
            cd "CMAKE_LIBS"
            print_m "CMAKE_LIBS"
            ./o.sh 
            ;;
        2)
            cd "CMAKE_META"
            print_m "CMAKE_META"
            ./o.sh 
            ;;
        3)  
            cd "CMAKE_EMBED"
            print_m "CMAKE_EMBED"
            ./o.sh 
            cd ..
            ;;
        4)
            cd "CMAKE_EXTRACT"
            print_m "CMAKE_EXTRACT"
            ./o.sh
            cd ..
            ;;
        5)  
            cd "CMAKE_CHUNKS_CLEAN"
            print_m "CMAKE_CHUNKS_CLEAN"
            ./o.sh
            cd ..
            ;;
        *)
            echo "MÓDULO inválido. Use 1, 2, 3, 4, 5"
            echo "--------------------------------------------------------------------------"
            echo "  1 => purecpp_libs"
            echo "  2 => purecpp_meta"
            echo "  3 => purecpp_embed"
            echo "  4 => purecpp_extract"
            echo "  5 => purecpp_chunks_clean"
            echo "--------------------------------------------------------------------------"
            ;;
            
    esac
fi
# #-----------------------------------------------------------------------------------------------------------------------------------
