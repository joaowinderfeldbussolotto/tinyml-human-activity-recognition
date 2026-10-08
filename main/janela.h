#pragma once

#include "sensor_mpu6050.h"

// Junta amostras e avisa quando há uma janela completa. Depois da primeira janela (128
// amostras), uma nova fica pronta a cada 64 amostras, ou seja, 50% de sobreposição como no
// dataset de treino.
class JanelaDeslizante {
 public:
  static constexpr int kPasso = HAR_JANELA / 2;

  // Devolve true quando uma janela nova ficou pronta.
  bool adicionar(const AmostraImu& amostra);

  // Copia a janela mais recente, da amostra mais antiga para a mais nova.
  void copiar(float saida[HAR_JANELA][HAR_CANAIS]) const;

 private:
  float buffer_[HAR_JANELA][HAR_CANAIS] = {};
  int proxima_ = 0;     // onde a próxima amostra entra no buffer circular
  int total_ = 0;       // amostras recebidas até encher a primeira janela
  int desde_ultima_ = 0;
};
