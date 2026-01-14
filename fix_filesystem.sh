#!/bin/bash

# Script para reemplazar 'std::filesystem' con 'fs::' (compatibilidad GCC 7.5 / Ubuntu 18.04)
# Busca recursivamente en el directorio ecu_atc8110

SEARCH_DIR="ecu_atc8110"

echo "=== Iniciando corrección de std::filesystem en $SEARCH_DIR ==="

if [ ! -d "$SEARCH_DIR" ]; then
    echo "Error: No se encuentra el directorio $SEARCH_DIR"
    echo "Ejecuta este script desde la raíz del proyecto."
    exit 1
fi

# Contadores
count=0

# Buscar archivos .cpp y .hpp que contengan 'std::filesystem'
while IFS= read -r file; do
    echo "Actualizando: $file"
    # Reemplazo in-place
    sed -i 's/std::filesystem::/fs::/g' "$file"
    ((count++))
done < <(find "$SEARCH_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec grep -l "std::filesystem" {} +)

echo ""
echo "=== Completado ==="
echo "Archivos modificados: $count"
