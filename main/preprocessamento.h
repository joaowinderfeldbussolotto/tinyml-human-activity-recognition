#pragma once

#include <cstdint>

#include "har_parametros.h"

// Normaliza cada canal com a média e o desvio do treino e quantiza para int8, com a escala e
// o ponto zero que o conversor escolheu para a entrada do modelo.
void preprocessar(const float janela[HAR_JANELA][HAR_CANAIS], int8_t saida[HAR_JANELA * HAR_CANAIS]);
