#!/usr/bin/env bash
# Genera las trazas de demostración del visualizador (gui/traces.js).
# Uso: bash scripts/gen_gui_traces.sh [./ticketstorm]
set -u
BIN="${1:-./ticketstorm}"
OUT=gui/traces
mkdir -p "$OUT"
CONT=(-w 4 -q 4 -r 16 -t 20 --min-ms 1 --max-ms 4 --window-us 2500)

"$BIN" --quiet --seed 42 -w 3 -q 3 -r 12 -t 30 --min-ms 30 --max-ms 90 --trace "$OUT/basico.json" > /dev/null
"$BIN" --quiet --seed 7 "${CONT[@]}" --trace "$OUT/mutex.json" > /dev/null

# Modo atómico: se busca una semilla donde se vean reintentos de CAS.
for s in $(seq 1 60); do
    "$BIN" --quiet --seed "$s" "${CONT[@]}" --atomic --trace "$OUT/atomic.json" > /dev/null
    [ "$(grep -c '"cas_fail"' "$OUT/atomic.json")" -ge 3 ] && break
done

# Sin sincronización: se busca una semilla donde ocurra la sobreventa.
for s in $(seq 1 200); do
    "$BIN" --quiet --seed "$s" "${CONT[@]}" --no-sync --trace "$OUT/nosync.json" > /dev/null
    [ $? -eq 1 ] && break
done

{
    echo "// Generado por scripts/gen_gui_traces.sh — no editar a mano."
    echo "window.TS_TRACES = {"
    for k in basico mutex atomic nosync; do
        printf '"%s": ' "$k"; cat "$OUT/$k.json"; echo ","
    done
    echo "};"
} > gui/traces.js
echo "Trazas generadas en gui/traces.js"
