#!/bin/bash

# Descargar archivos clave del driver Radeon de FreeBSD
BASE_URL="https://cgit.freebsd.org/src/plain/sys/dev/drm2/radeon"

echo "Descargando archivos del driver Radeon de FreeBSD..."

# Archivos principales
FILES=(
    "radeon.h"
    "radeon_drv.h"
    "radeon_reg.h"
    "radeon_cp.c"
    "radeon_irq.c"
    "radeon_mem.c"
    "radeon_object.c"
    "radeon_state.c"
    "radeon_gem.c"
    "radeon_pm.c"
    "r600_blit.c"
    "r600_reg.h"
    "rv770_dpm.c"
    "atom.c"
    "atom.h"
    "atombios.h"
)

for file in "${FILES[@]}"; do
    echo "Descargando $file..."
    curl -s -L "$BASE_URL/$file" -o "$file" && echo "✓ $file" || echo "✗ $file (no encontrado)"
done

echo ""
echo "Descarga completada"
ls -lh *.c *.h 2>/dev/null | wc -l
echo "archivos descargados"
