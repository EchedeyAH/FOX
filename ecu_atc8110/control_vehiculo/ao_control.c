#include "ao_control.h"
#include "../adquisicion_datos/ao_output.h"

#include <string.h>

// Canales logicos (legacy)
enum {
    AO_CH_ACL_M1 = 0,
    AO_CH_ACL_M2 = 1,
    AO_CH_ACL_M3 = 2,
    AO_CH_ACL_M4 = 3,
    AO_CH_FRN_M1 = 4,
    AO_CH_FRN_M2 = 5,
    AO_CH_FRN_M3 = 6,
    AO_CH_FRN_M4 = 7,
    AO_LOGICAL_COUNT = 8
};

// === MODO TEST AO ===
// Definir AO_TEST_MODE para activar el modo test.
// AO_TEST_CHANNEL: canal 0..7 a activar.
// AO_TEST_VALUE: voltaje fijo en V para ese canal.
//#define AO_TEST_MODE
#define AO_TEST_CHANNEL 0
#define AO_TEST_VALUE   2.0f

// Mapeo logico -> fisico (PEX-DA16)
// Suposicion: 4 canales fisicos disponibles (AO0..AO3).
static const int k_logical_to_hw[AO_LOGICAL_COUNT] = {
    0, 1, 2, 3,   // Acelerador M1..M4
   -1, -1, -1, -1 // Freno M1..M4 (no mapeado en HW actual)
};

void ao_update_from_control(const float *acelerador, const float *freno)
{
    float logical[AO_LOGICAL_COUNT];
    float hw_vals[4];

    for (int i = 0; i < AO_LOGICAL_COUNT; ++i) logical[i] = 0.0f;
    for (int i = 0; i < 4; ++i) hw_vals[i] = 0.0f;

    if (acelerador) {
        logical[AO_CH_ACL_M1] = acelerador[0];
        logical[AO_CH_ACL_M2] = acelerador[1];
        logical[AO_CH_ACL_M3] = acelerador[2];
        logical[AO_CH_ACL_M4] = acelerador[3];
    }
    if (freno) {
        logical[AO_CH_FRN_M1] = freno[0];
        logical[AO_CH_FRN_M2] = freno[1];
        logical[AO_CH_FRN_M3] = freno[2];
        logical[AO_CH_FRN_M4] = freno[3];
    }

#ifdef AO_TEST_MODE
    // MODO TEST: solo un canal activo, resto a 0
    for (int i = 0; i < AO_LOGICAL_COUNT; ++i) logical[i] = 0.0f;
    if (AO_TEST_CHANNEL >= 0 && AO_TEST_CHANNEL < AO_LOGICAL_COUNT) {
        logical[AO_TEST_CHANNEL] = AO_TEST_VALUE;
    }
#endif

    for (int i = 0; i < AO_LOGICAL_COUNT; ++i) {
        int hw = k_logical_to_hw[i];
        if (hw >= 0 && hw < 4) {
            hw_vals[hw] = logical[i];
        }
    }

    ao_set_all(hw_vals, 4);
}

