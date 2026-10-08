#pragma once

#include "har_parametros.h"

struct AmostraImu {
  float v[HAR_CANAIS];  // aceleração x,y,z em g; giroscópio x,y,z em rad/s
};

// Acorda o MPU6050 e configura as faixas. Devolve false se o sensor não responder.
bool sensor_iniciar();

// Lê uma amostra. Devolve false se a leitura I2C falhar.
bool sensor_ler(AmostraImu* amostra);
