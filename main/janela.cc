#include "janela.h"

bool JanelaDeslizante::adicionar(const AmostraImu& amostra) {
  for (int c = 0; c < HAR_CANAIS; ++c) buffer_[proxima_][c] = amostra.v[c];
  proxima_ = (proxima_ + 1) % HAR_JANELA;

  if (total_ < HAR_JANELA) {
    ++total_;
    if (total_ == HAR_JANELA) {
      desde_ultima_ = 0;
      return true;
    }
    return false;
  }
  if (++desde_ultima_ == kPasso) {
    desde_ultima_ = 0;
    return true;
  }
  return false;
}

void JanelaDeslizante::copiar(float saida[HAR_JANELA][HAR_CANAIS]) const {
  // proxima_ aponta para a amostra mais antiga quando o buffer está cheio
  for (int t = 0; t < HAR_JANELA; ++t) {
    const int origem = (proxima_ + t) % HAR_JANELA;
    for (int c = 0; c < HAR_CANAIS; ++c) saida[t][c] = buffer_[origem][c];
  }
}
