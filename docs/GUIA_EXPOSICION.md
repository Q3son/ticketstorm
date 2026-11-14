# Guía de exposición — TicketStorm

**Duración:** 12–15 minutos + preguntas · **Equipo:** 5 integrantes · **Material:** `gui/exponer.html`

> **Idea central del pitch:** «Una ticketera de las de antes vendió entradas que no existían.
> Mostramos por qué pasó, lo arreglamos con herramientas del sistema operativo y explicamos cómo
> las ticketeras de hoy usan la misma idea a gran escala.»

---

## Antes de empezar (5 minutos antes)

1. Abrir **`exponer.html`** con doble clic (funciona sin internet).
2. Presionar **`F`** para pantalla completa.
3. Si el proyector se ve oscuro, presionar **`T`** (tema claro).
4. Dejar la pantalla en la **historia de TicketClásica**. No presionar nada todavía.

| Tecla | Uso durante la exposición |
|---|---|
| `Espacio` | Reproducir hasta la siguiente parada |
| `Enter` | Continuar al siguiente momento |
| `←` `→` | Retroceder / avanzar un evento (para repetir algo) |
| `C` | Abrir las conclusiones en cualquier momento |
| `1`–`4` | Saltar a un escenario si se pierden |

---

## Guion

### 1. Gancho · la historia (1 min) — *Edson*
**Pantalla:** «Viernes, 10:00 a. m. Abre la preventa».

> «Les presentamos a **TicketClásica**, una ticketera de las de antes: un solo servidor, cuatro hilos
> que cobran en paralelo y 20 entradas. Para que fuera más rápido, alguien quitó el candado del
> contador. La pregunta es simple: **¿qué puede salir mal si varios compradores llegan en el mismo
> milisegundo?**»

Dejar 2–3 segundos de silencio y presionar **«Ver lo que pasó →»**.

### 2. El problema en vivo (2 min) — *Edson*
**Pantalla:** escenario *Sin sincronización*. La animación se detiene sola.

- **Parada «¡Condición de carrera!»:** leer la pregunta al público y pedir que voten con la mano (A, B o C). Presionar **Revelar**.
  > «Dos hilos están dentro de la sección crítica a la vez. Los dos vieron 20 entradas.»
- `Enter` → **Sobreventa**.
  > «Esto no es un dibujo: es lo que **realmente hizo** nuestro programa en C++.»
- `Enter` → **Resultado: 21 entradas vendidas de 20.**
  > «Una persona pagó por una entrada que no existe. Y lo peor: solo pasa **en 1 de cada 4
  > ejecuciones**. El sistema puede pasar muchas pruebas y fallar el día de la preventa.»

### 3. ¿Por qué pasó? (1 min) — *Edson*
Señalar la línea de código del panel derecho (`if (available_ >= qty) // ¡sin mutex!`).

> «Vender tiene dos pasos: **verificar** que haya stock y **descontar**. Si dos hilos verifican
> antes de que alguno descuente, ambos venden lo mismo. Se llama *verificar-y-luego-actuar*, y es
> una **condición de carrera**. La solución es que esos dos pasos sean indivisibles.»

`Enter` → pasa solo al siguiente escenario.

### 4. La base: productor-consumidor (3 min) — *Giovanni, Daniel y Juan Manuel*
**Pantalla:** escenario *Productor-consumidor* (8 paradas).

| Parada | Quién | Qué decir |
|---|---|---|
| Cola vacía | Giovanni | «El Worker no pregunta en un bucle: llama a `wait(notEmpty)` y el SO lo **bloquea**. No gasta CPU.» (pregunta al público) |
| Primer push | Daniel | «El Dispatcher mete la petición y hace `notify_one` para despertar a un consumidor.» |
| Cola llena | Giovanni | «Es un **buffer acotado**: si está lleno, el productor espera en `notFull`.» (pregunta al público) |
| Despertar | Juan Manuel | «El Worker despierta, recupera el mutex y **vuelve a comprobar** la condición.» |
| sleep | Juan Manuel | «Procesar es dormir: el hilo pasa a *Durmiendo* y la CPU queda libre.» Señalar el **diagrama de estados**. |
| close() | Daniel | «Al final, `notify_all` despierta a todos para que terminen limpio.» |

> Si el tiempo aprieta, se puede pasar rápido con `Enter` desde «Despertar».

### 5. Solución 1: el mutex (2 min) — *Edson*
**Pantalla:** escenario *Contención con mutex*.

- **lock():** «W1 toma el mutex: verificar y descontar ahora son una sola operación.»
- **Contención** (pregunta al público): «W2 quiere entrar, pero el mutex lo tiene W1. El SO lo pone en la cola del mutex.»
- **Relevo:** «W1 hace `unlock` y el SO despierta a W2. Siempre hay un solo hilo dentro.»
- **Resultado:** ✓ *Verificación OK*. «Con el candado puesto, TicketClásica nunca vende de más.»

### 6. Solución 2: sin cerrojo (1,5 min) — *Edson*
**Pantalla:** escenario *Atómico (CAS)*.

- «Otra forma: nadie se bloquea. Cada hilo intenta descontar **solo si nadie cambió el stock**; si falla, reintenta.»
- Dato del informe: «También es correcto, pero con mucha competencia fue **más lento** que el mutex (118 ns contra 68 ns por compra con 16 hilos). No hay solución gratis.»

### 7. Evidencia (1,5 min) — *Brigitte*
Puede mostrarse el informe (Figura 5 y Tabla 10) o decirlo de palabra:

- Sin mutex: **5 de 20** ejecuciones vendieron de más. Con mutex: **0 de 20**.
- **10 pruebas de integración + 10 pruebas unitarias** (más de 10 000 comprobaciones), todas OK.
- Con 16 hilos esperando, solo **0,015 s de CPU**: la espera es pasiva.
- En la revisión encontramos y corregimos un error real de portabilidad en Windows (`long` de 32 bits) y una carrera en nuestro propio código de pruebas.

### 8. Conclusiones (1,5 min) — *Brigitte*
**Pantalla:** «De TicketClásica a las ticketeras de hoy» (aparece sola al final, o con `C`).

Recorrer la tabla de arriba hacia abajo:

> «Las ticketeras actuales atienden a millones de personas con muchos servidores, pero tienen
> los mismos problemas. Su **sala de espera virtual** es nuestra cola acotada; el **bloqueo de la
> fila en la base de datos** es nuestro mutex; el **control optimista** es nuestro CAS.
> **Cambia la escala, no la idea.**»

Cerrar con la conclusión 5:

> «Para TicketClásica, nuestra solución es suficiente. Para una ticketera masiva, es el
> fundamento. La lección de Sistemas Operativos es esta: **la corrección no se prueba con suerte,
> se diseña.**»

---

## Preguntas probables del docente

| Pregunta | Respuesta corta |
|---|---|
| ¿Esto sirve para Ticketmaster? | No tal cual: un `std::mutex` protege hilos de **un proceso**, no varios servidores. Pero la idea es la misma; a gran escala se usan transacciones y bloqueos en la base de datos. |
| ¿Los datos son reales o simulados? | Reales. El programa registra cada evento con `--trace` y la web los reproduce. Solo la duración del cobro se simula con `sleep_for`. |
| ¿Por qué no usar siempre atómicos? | Porque con mucha contención el CAS reintenta mucho y fue más lento que el mutex en nuestras mediciones. |
| ¿Qué es un despertar espurio? | Un hilo puede salir de `wait` sin que nadie haya hecho `notify`. Por eso la espera usa un predicado y vuelve a comprobar la condición. |
| ¿Cómo evitan el interbloqueo? | Con orden fijo de cerrojos: primero la cola y luego la consola, nunca al revés. El inventario nunca se bloquea junto con otro. |
| ¿Por qué `notify` después de soltar el mutex? | Para no despertar a un hilo que tendría que volver a bloquearse esperando ese mismo mutex. |
| ¿Por qué la carrera no pasa siempre? | Depende de cómo el planificador intercale los hilos. Por eso las pruebas repiten ejecuciones. |
| ¿Cómo terminan los hilos? | `close()` activa `closed_` y hace `notify_all`; los Workers vacían la cola y salen. Luego el hilo principal hace `join()`. |

---

## Si algo falla

- **No avanza:** presionar `Espacio`. Si hay una ventana abierta, `Enter`.
- **Se perdieron:** teclas `1`–`4` para ir a un escenario, `C` para conclusiones.
- **Sin proyector o sin navegador:** usar las figuras 2 y 3 del informe (capturas del visualizador).
