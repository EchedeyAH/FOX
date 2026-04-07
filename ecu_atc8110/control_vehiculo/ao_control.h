#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Actualiza las salidas analogicas a partir de la logica de control.
// acelerador[4] y freno[4] se mapean a 8 canales logicos AO.
void ao_update_from_control(const float *acelerador, const float *freno);

#ifdef __cplusplus
}
#endif

