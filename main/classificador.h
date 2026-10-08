#pragma once

#include <cstdint>

#include "har_parametros.h"

struct Resultado {
  int classe;                       // índice da atividade mais provável
  float probabilidade;              // probabilidade dessa atividade (0 a 1)
  int8_t bruto[HAR_CLASSES];        // saída int8 do modelo, antes de desquantizar
};

// Carrega o modelo e aloca a arena do TFLite Micro. Devolve false se falhar.
bool classificador_iniciar();

// Quantos bytes da arena o modelo realmente usa.
int classificador_arena_usada();

// Roda o modelo numa janela já quantizada (HAR_JANELA * HAR_CANAIS valores).
bool classificar(const int8_t* entrada, Resultado* resultado);
