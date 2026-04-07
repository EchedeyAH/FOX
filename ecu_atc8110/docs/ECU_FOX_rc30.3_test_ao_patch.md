# Modo Test AO (ECU_FOX_rc30.3)

Este documento contiene el bloque **BEFORE/AFTER** para habilitar un modo de test que fuerza **un solo canal AO** a un valor fijo y deja el resto a 0 V.

**Importante:** En `C:\Users\ahech\Desktop\FOX\ecu_atc8110` no existe el código fuente de `ECU_FOX_rc30.3` ni el archivo `tadAO_fox.c`, por lo que no puedo dar números de línea exactos dentro de ese archivo. En cuanto me indiques la ruta del `tadAO_fox.c` real dentro de `ecu_atc8110`, te actualizo este documento con líneas exactas o aplico el cambio directamente.

## Macros y helper (añadir en `tadAO_fox.c`)

Pegar cerca de los `#include` (antes de las variables globales):

```c
/* === MODO TEST AO ===
 * Definir TEST_SINGLE_CHANNEL para activar el modo test.
 * TEST_CHANNEL_INDEX: canal 0..7 a activar.
 * TEST_CHANNEL_VALUE_V: voltaje fijo en V para ese canal.
 */
//#define TEST_SINGLE_CHANNEL
#define TEST_CHANNEL_INDEX   0
#define TEST_CHANNEL_VALUE_V 2.0f
```

Pegar antes de `adq_ao`:

```c
static void set_test_outputs(float *acelerador, float *freno)
{
	int i;
	/* Pone todas las salidas a 0V */
	for (i = 0; i < NUM_MOTORES; i++)
	{
		acelerador[i] = 0.0f;
		freno[i] = 0.0f;
	}

	/* Activa solo el canal seleccionado */
	switch (TEST_CHANNEL_INDEX)
	{
		case ANLG_OUT_ACL_M1: acelerador[POS_M1] = TEST_CHANNEL_VALUE_V; break;
		case ANLG_OUT_ACL_M2: acelerador[POS_M2] = TEST_CHANNEL_VALUE_V; break;
		case ANLG_OUT_ACL_M3: acelerador[POS_M3] = TEST_CHANNEL_VALUE_V; break;
		case ANLG_OUT_ACL_M4: acelerador[POS_M4] = TEST_CHANNEL_VALUE_V; break;
		case ANLG_OUT_FRN_M1: freno[POS_M1] = TEST_CHANNEL_VALUE_V; break;
		case ANLG_OUT_FRN_M2: freno[POS_M2] = TEST_CHANNEL_VALUE_V; break;
		case ANLG_OUT_FRN_M3: freno[POS_M3] = TEST_CHANNEL_VALUE_V; break;
		case ANLG_OUT_FRN_M4: freno[POS_M4] = TEST_CHANNEL_VALUE_V; break;
		default: break;
	}
}
```

## BEFORE / AFTER (en `adq_ao`)

Inserta el bloque **justo antes** de:

```c
// Preparamos los valores de las salidas analogicas
```

### BEFORE

```c
pthread_mutex_lock(&mut_potencias);
memcpy(acelerador,potencias.acelerador,sizeof(acelerador));
memcpy(freno,potencias.freno,sizeof(freno));
pthread_mutex_unlock(&mut_potencias);

// Preparamos los valores de las salidas analogicas
```

### AFTER

```c
pthread_mutex_lock(&mut_potencias);
memcpy(acelerador,potencias.acelerador,sizeof(acelerador));
memcpy(freno,potencias.freno,sizeof(freno));
pthread_mutex_unlock(&mut_potencias);

#ifdef TEST_SINGLE_CHANNEL
/* MODO TEST AO:
 * Solo un canal activo con TEST_CHANNEL_VALUE_V.
 * El resto a 0V.
 * Para desactivar: comentar #define TEST_SINGLE_CHANNEL.
 * Para cambiar canal: modificar TEST_CHANNEL_INDEX (0..7).
 */
set_test_outputs(acelerador, freno);
#endif

// Preparamos los valores de las salidas analogicas
```

## Activación / desactivación

- Activar: descomenta `#define TEST_SINGLE_CHANNEL`
- Seleccionar canal: ajusta `TEST_CHANNEL_INDEX` (0..7)
- Fijar voltaje: ajusta `TEST_CHANNEL_VALUE_V`

## Nota final

Cuando confirmes la ruta exacta del `tadAO_fox.c` dentro de `ecu_atc8110`, puedo aplicar el cambio directamente y añadir los números de línea exactos.
