#pragma once

// Petición de compra generada por el Dispatcher y consumida por un Worker.
struct Request {
    int id      = 0;  // número correlativo de la petición (#1, #2, ...)
    int userId  = 0;  // usuario que solicita las entradas
    int tickets = 0;  // cantidad de entradas solicitadas (1..4)
};
