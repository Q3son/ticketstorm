#!/usr/bin/env bash
# Plan de pruebas de TicketStorm (Tabla 4). Uso: bash scripts/run_tests.sh [./ticketstorm]
BIN="${1:-./ticketstorm}"
PASS=0
FAIL=0

report() {  # report <0|1> <descripción>
    if [ "$1" -eq 0 ]; then
        echo "  [PASS] $2"; PASS=$((PASS + 1))
    else
        echo "  [FAIL] $2"; FAIL=$((FAIL + 1))
    fi
}

echo "T1. Ejecución base (3 Workers, cola de 5)"
out=$("$BIN" --seed 42); code=$?
echo "$out" | grep -q "Verificación: OK"
report $(( code != 0 || $? != 0 )) "salida coherente y verificación OK (código $code)"

echo "T2. Cola pequeña con pocos Workers (el Dispatcher se bloquea y se reanuda)"
out=$("$BIN" -w 1 -q 1 -r 12 -t 30 --min-ms 20 --max-ms 40 --seed 7); code=$?
echo "$out" | grep -q "Cola llena, esperando espacio"; a=$?
echo "$out" | grep -q "Espacio liberado"; b=$?
report $(( code != 0 || a != 0 || b != 0 )) "aparecen 'Cola llena' y 'Espacio liberado'"

echo "T3. Muchos Workers (esperan pasivamente sin consumir CPU)"
TIMEFORMAT="    tiempo real=%R s | CPU usuario=%U s | CPU sistema=%S s"
out=$( { time "$BIN" -w 16 -q 5 -r 20 -t 50 --seed 3; } 2>&1 ); code=$?
echo "$out" | grep -q "Cola vacía, esperando trabajo"; a=$?
echo "$out" | grep "tiempo real"
report $(( code != 0 || a != 0 )) "Workers ociosos se bloquean en la condition_variable"

echo "T4. Versión SIN mutex en el contador (debe aparecer una venta de más)"
found=1
for i in $(seq 1 20); do
    "$BIN" --no-sync --quiet -w 8 -q 5 -t 100 -r 2000 --min-ms 0 --max-ms 0 > /tmp/ts_nosync.out
    if [ $? -eq 1 ] && grep -q "CONDICIÓN DE CARRERA DETECTADA" /tmp/ts_nosync.out; then
        found=0; echo "    carrera observada en el intento $i:"
        grep -E "Entradas vendidas|Verificación" /tmp/ts_nosync.out | sed 's/^/    /'
        break
    fi
done
report $found "se detectó condición de carrera sin sincronización"

echo "T5. Versión CON mutex (las entradas vendidas nunca superan las disponibles)"
ok=0
for i in $(seq 1 5); do
    "$BIN" --quiet -w 8 -q 5 -t 100 -r 2000 --min-ms 0 --max-ms 0 > /tmp/ts_sync.out || ok=1
done
report $ok "5 ejecuciones intensivas sin ventas de más"

echo "T6. Repetir 10 ejecuciones (resultado consistente)"
ok=0
for i in $(seq 1 10); do
    "$BIN" --quiet --seed 42 --min-ms 5 --max-ms 20 > /tmp/ts_rep.out || ok=1
    grep -q "Verificación: OK" /tmp/ts_rep.out || ok=1
done
report $ok "10 ejecuciones con verificación OK"

echo
echo "Resultado: $PASS pruebas correctas, $FAIL fallidas."
[ "$FAIL" -eq 0 ]
