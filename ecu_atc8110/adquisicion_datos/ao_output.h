#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa el subsistema de salidas analógicas (PEX-DA16).
void ao_init(void);

// Escribe un canal AO individual (voltaje en V, rango 0..10V).
void ao_set_channel(int channel, float voltage);

// Escribe múltiples canales AO de una vez.
void ao_set_all(const float *values, int count);

#ifdef __cplusplus
}
#endif

