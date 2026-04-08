#include "adquisicion_datos/ao_output.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    ao_init();

    printf("TEST AO MOTOR M1\n");

    while (1) {
        ao_set_channel(0, 1.0f); // M1 -> 1V
        usleep(100000); // 100 ms
    }

    return 0;
}
