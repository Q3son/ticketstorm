# Guion del expositor — TicketStorm (una sola persona)

**Duración objetivo:** 13 minutos + preguntas · **Archivo:** `exponer.html`
**Formato:** `▶ ACCIÓN` = lo que presionas · `🗣` = lo que dices · `⏱` = tiempo acumulado

> Regla de oro: **la pantalla muestra, tú explicas el porqué.** No leas lo que ya está escrito en
> la ventana; di la idea en tus palabras y señala con la mano.

---

## 0. Preparación (10 minutos antes)

- [ ] Laptop **conectada a la corriente**, notificaciones silenciadas (Windows: *No molestar*).
- [ ] Doble clic en `exponer.html` → **`F`** (pantalla completa). Si el proyector lava los colores: **`T`**.
- [ ] Verifica que se vea la historia «Viernes, 10:00 a. m.». **No presiones nada más.**
- [ ] Ten abierto el PDF del informe en otra ventana por si piden un dato (Alt+Tab).
- [ ] Un vaso de agua. Respira. Sabes de esto más que nadie en el salón.

**Teclas que vas a usar:** `Espacio` (reproducir) · `Enter` (continuar) · `←` (repetir un paso) · `C` (conclusiones)

---

## 1. Presentación y gancho · ⏱ 0:00 – 1:15

**Pantalla:** historia de TicketClásica.

🗣 «Buenos días, profesor, compañeros. Soy Edson Subia y les presento **TicketStorm**, el proyecto
de Sistemas Operativos de nuestro equipo: Giovanni Mejía, Daniel Guillen, Juan Manuel Cari,
Brigitte Mengoa y yo.»

🗣 «Quiero empezar con una historia. Es viernes, diez de la mañana, y abre la preventa de un
concierto. La ticketera se llama **TicketClásica**: es una ticketera de las de antes, con **un solo
servidor**. Un hilo recibe los pedidos y **cuatro hilos** cobran en paralelo. Quedan **veinte
entradas**.»

👉 Señala el recuadro rojo con el **0**.

🗣 «Y un detalle: para que vaya más rápido, alguien quitó el candado del contador de entradas.»

🗣 (pausa de 2 segundos, mira al público) «**¿Qué puede salir mal** si varios compradores llegan
en el mismo milisegundo?»

▶ Clic en **«Ver lo que pasó →»**

🗣 (mientras arranca) «Lo que van a ver **no es una animación inventada**: es la ejecución real de
nuestro programa en C++. Cada evento quedó registrado y la web lo reproduce paso a paso.»

---

## 2. El problema en vivo · ⏱ 1:15 – 3:30

**Escenario:** Sin sincronización. La animación avanza sola y **se detiene** en la carrera.

### Parada 1 — «¡Condición de carrera!» (pregunta al público)
👉 Señala la caja roja: **«¡2 hilos dentro a la vez!»**

🗣 «Aquí está. W2 y W3 están **al mismo tiempo** en la parte del código que descuenta entradas.
Los dos miraron el stock y los dos vieron veinte.»

🗣 «Les pregunto: los dos vieron veinte entradas, ¿qué puede pasar? ¿A, B o C? Levanten la mano.»
(cuenta manos rápido, 5 segundos)

▶ **Revelar** → 🗣 «Correcto, la B: **venden las mismas entradas.**»

▶ **`Enter`**

### Parada 2 — «Sobreventa»
👉 Señala el número grande del inventario.

🗣 «Y aquí lo confirmamos: se vendieron **más entradas de las que existían**.»

▶ **`Enter`**

### Parada 3 — «Verificación fallida»
🗣 «Resultado final: **veintiún entradas vendidas habiendo solo veinte.** Una persona pagó por un
asiento que no existe.»

🗣 (más lento) «Y lo más peligroso: esto **no pasa siempre**. En nuestras mediciones pasó en
**una de cada cuatro ejecuciones**. Es decir, TicketClásica podía pasar todas sus pruebas… y fallar
justo el día de la preventa.»

---

## 3. ¿Por qué pasó? · ⏱ 3:30 – 4:30

👉 Señala la línea de código del panel derecho.

🗣 «¿Por qué pasa esto? Vender tiene **dos pasos**: primero *verificar* si hay stock, y después
*descontarlo*. Si dos hilos verifican antes de que cualquiera descuente, los dos venden lo mismo.»

🗣 «En Sistemas Operativos esto se llama **condición de carrera**, y el patrón se conoce como
*verificar y luego actuar*. La solución es convertir esos dos pasos en **una sola operación
indivisible**. Para entender cómo, primero veamos cómo se organiza el sistema.»

▶ **`Enter`** (pasa solo al escenario Productor-consumidor)

---

## 4. La base: productor-consumidor · ⏱ 4:30 – 7:30

👉 Recorre la pantalla de izquierda a derecha con la mano **una sola vez**:

🗣 «El sistema es el problema clásico **productor-consumidor**: a la izquierda, el **Dispatcher**
genera pedidos; al centro, una **cola de tamaño limitado**; a la derecha, los **Workers** que los
atienden; y abajo, el **inventario**, que es el recurso compartido.»

▶ **`Espacio`**

### «La cola está vacía» (pregunta al público)
🗣 «Al inicio la cola está vacía. ¿Qué hace un Worker que no tiene trabajo? ¿Pregunta en un
bucle, se bloquea o termina?» → ▶ **Revelar**

🗣 «Se bloquea con **`wait`** sobre una variable de condición. El sistema operativo lo duerme y
**no gasta CPU**. Esto es la **espera pasiva**.» → ▶ **`Enter`**

### «Entra la primera petición»
🗣 «El Dispatcher mete el primer pedido y hace **`notify_one`**: es como tocar la puerta para
despertar a un consumidor.» → ▶ **`Enter`**

### «El buffer se llenó» (pregunta al público)
🗣 «Ahora el caso contrario: la cola se llenó. ¿Qué hace el productor?» → ▶ **Revelar**

🗣 «Espera en **`notFull`**. No se pierde ningún pedido y no se gasta CPU. Esto es el **buffer
acotado** que vimos en clase.» → ▶ **`Enter`**

### «notify despierta a un Worker»
🗣 «Al despertar, el hilo **vuelve a comprobar** la condición antes de seguir, porque puede haber
despertares espurios. Es la semántica Mesa.» → ▶ **`Enter`**

### «Procesar es dormir»
👉 Señala el **diagrama de estados** (derecha).

🗣 «Miren el diagrama de estados: el hilo pasó a *Durmiendo*. Es el mismo diagrama de procesos
del libro, pero en vivo: Nuevo, Ejecutable, Bloqueado y Terminado.» → ▶ **`Enter`**

### «Hay espacio otra vez» → ▶ **`Enter`** (no hace falta comentar mucho)
🗣 «El ciclo se repite: el consumidor libera espacio y despierta al productor.»

### «Cierre ordenado»
🗣 «Al terminar, el Dispatcher hace **`notify_all`**: despierta a todos para que vacíen la cola y
terminen limpio. Nadie se queda bloqueado para siempre.» → ▶ **`Enter`**

### «Verificación correcta» → ▶ **`Enter`**
🗣 «Ahora sí: con todo esto sincronizado, veamos cómo se arregla el problema de TicketClásica.»

---

## 5. Solución 1: el mutex · ⏱ 7:30 – 9:30

### «Entrar a la sección crítica»
🗣 «Esta es la **sección crítica**. Un hilo toma el **mutex**, el candado, y desde ese momento
verificar y descontar son una sola operación: nadie puede meterse en medio.» → ▶ **`Enter`**

### «Dos hilos, un solo mutex» (pregunta al público)
🗣 «W1 tiene el candado. ¿Qué hace W2?» → ▶ **Revelar**

👉 Señala los círculos amarillos en «Cola del mutex».

🗣 «Espera **bloqueado**. El sistema operativo lo pone en la cola del mutex. Esto es la
**exclusión mutua**.» → ▶ **`Enter`**

### «El mutex pasa al siguiente»
🗣 «Cuando W1 suelta el candado, el sistema operativo despierta a W2. **Siempre hay un solo hilo
dentro.**» → ▶ **`Enter`**

### «Verificación correcta»
🗣 (con énfasis) «**Veinte entradas, veinte ventas.** Le pusimos el candado a TicketClásica y nunca
más vendió de más: cero sobreventas en veinte ejecuciones intensivas.» → ▶ **`Enter`**

---

## 6. Solución 2: sin candado · ⏱ 9:30 – 10:45

### «Varios hilos a la vez, sin cerrojo»
🗣 «Hay otra forma de hacerlo, **sin candado**. Aquí varios hilos trabajan al mismo tiempo, pero
cada uno descuenta **solo si nadie cambió el stock mientras tanto**. Se llama *compare-and-swap*.»
→ ▶ **`Enter`**

### «El CAS falló… y no pasa nada» (pregunta al público)
🗣 «Falló el intento. ¿El hilo se bloquea?» → ▶ **Revelar**

🗣 «No: relee el valor nuevo y **reintenta**. Nadie se bloquea nunca.» → ▶ **`Enter`**

### «Verificación correcta»
🗣 «También es correcto. Pero aquí viene algo interesante: lo medimos, y con dieciséis hilos
compitiendo fue **más lento que el mutex**, 118 nanosegundos contra 68 por compra. Los reintentos
cuestan. **No hay solución gratis**: la técnica depende de la carga.»

▶ **`Enter`** → se abren las **Conclusiones**

---

## 7. Conclusiones · ⏱ 10:45 – 13:00

**Pantalla:** «De TicketClásica a las ticketeras de hoy».

🗣 «Para cerrar: ¿esto sirve para las ticketeras de verdad? Las de hoy atienden a millones de
personas con muchos servidores, así que no usan exactamente nuestro mutex. Pero **enfrentan los
mismos problemas**.»

👉 Recorre la tabla fila por fila con la mano:

🗣 «Cuando hay demasiados compradores, nosotros usamos una **cola acotada**; ellas usan una **sala
de espera virtual**. Cuando dos personas quieren la misma entrada, nosotros usamos un **mutex**;
ellas **bloquean la fila del asiento en la base de datos**. Y nuestro **CAS** es lo que ellas llaman
**control optimista**. **Cambia la escala, no la idea.**»

👉 Señala las conclusiones numeradas (léelas resumidas, no completas):

🗣 «Nos quedamos con cinco conclusiones:
**uno**, el problema es real y silencioso, porque solo aparece a veces;
**dos**, la causa es que verificar y descontar no eran indivisibles;
**tres**, la solución funciona: cero sobreventas y espera sin gastar CPU;
**cuatro**, no hay solución gratis: lo atómico fue más lento con contención;
y **cinco**, la idea escala.»

🗣 (pausa, mira al profesor) «Para TicketClásica, nuestra solución es suficiente. Para una
ticketera masiva, es el fundamento. Y la lección de Sistemas Operativos es esta: **la corrección no
se prueba con suerte, se diseña.** Muchas gracias.»

(Quédate en silencio y sonríe. No digas «eso es todo».)

---

## Preguntas: cómo responder

**Técnica:** escucha completa → repite la pregunta en voz alta → responde en **una frase** → ofrece
mostrarlo en la web o el informe.

| Si preguntan… | Responde |
|---|---|
| ¿Los datos son reales? | «Sí. El programa registra cada evento con `--trace` y la web lo reproduce. Solo el tiempo de cobro se simula con `sleep_for`.» |
| ¿Sirve para Ticketmaster? | «No tal cual: un mutex protege hilos de un proceso, no varios servidores. Pero la idea es la misma; a gran escala se hace con transacciones en la base de datos.» |
| ¿Por qué no siempre atómicos? | «Porque con mucha contención reintentan mucho. Lo medimos: fueron más lentos.» → Alt+Tab al informe, Tabla 10. |
| ¿Despertar espurio? | «Un hilo puede salir de `wait` sin `notify`. Por eso esperamos con un predicado y volvemos a comprobar.» |
| ¿Cómo evitan el interbloqueo? | «Con orden fijo de candados: primero la cola, después la consola, nunca al revés.» |
| ¿Por qué `notify` fuera del candado? | «Para no despertar a un hilo que de inmediato se volvería a bloquear esperando el mismo mutex.» |
| ¿Por qué la carrera no pasa siempre? | «Depende de cómo el planificador intercale los hilos. Por eso repetimos cada prueba varias veces.» |
| ¿Cómo terminan los hilos? | «`close()` activa una bandera y hace `notify_all`; los Workers vacían la cola, salen y el hilo principal hace `join`.» |
| ¿Qué hizo cada integrante? | «Cada uno tuvo un módulo: Giovanni la cola, Daniel el Dispatcher, Juan Manuel los Workers, Brigitte la configuración y las pruebas, y yo el inventario, la consola y el visualizador.» |

**Si no sabes algo:** «Buena pregunta; no lo medimos, pero lo esperable sería… y lo podemos agregar
como trabajo futuro.» Nunca inventes un dato.

---

## Plan de ensayo

1. **Ensayo 1 (con el guion en la mano):** recorre todo despacio y marca dónde te trabas.
2. **Ensayo 2 (cronometrado):** objetivo ≤ 13 min. Si te pasas, en el escenario 1 avanza rápido
   desde «Despertar» hasta «Cierre ordenado» con `Enter`.
3. **Ensayo 3 (sin guion, frente a alguien):** solo memoriza el **gancho**, las **frases en negrita**
   y el **cierre**. Lo demás lo dirá la pantalla.

## Lenguaje corporal y voz

- Párate **al lado** de la pantalla, no delante. Señala con la mano abierta.
- Mira al **público**, no a la pantalla, mientras hablas. Mira la pantalla solo para señalar.
- Baja la velocidad en tres momentos: **«¿Qué puede salir mal?»**, **«21 de 20»** y el **cierre**.
- En las preguntas al público, **espera de verdad** a que levanten la mano: eso engancha.

## Si algo falla en vivo

| Problema | Solución |
|---|---|
| Se queda quieto | `Espacio`; si hay una ventana abierta, `Enter` |
| Te pasaste un paso | `←` para retroceder un evento |
| Te perdiste | `1`–`4` cambia de escenario; `C` abre las conclusiones |
| No funciona la laptop o el proyector | Usa el PDF del informe: Figuras 2 y 3 (capturas) y Figura 5 (gráfico de la carrera) |
