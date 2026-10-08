#pragma once
#include <cstdint>
#include "har_parametros.h"

#define HAR_DEMO_N 60

extern const float JANELAS_DEMO[HAR_DEMO_N][HAR_JANELA][HAR_CANAIS];
extern const uint8_t DEMO_ROTULOS[HAR_DEMO_N];
extern const int8_t DEMO_REFERENCIA_PC[HAR_DEMO_N][HAR_CLASSES];
