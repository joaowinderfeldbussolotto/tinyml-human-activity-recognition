#include "preprocessamento.h"

#include <cmath>

void preprocessar(const float janela[HAR_JANELA][HAR_CANAIS], int8_t saida[HAR_JANELA * HAR_CANAIS]) {
  for (int t = 0; t < HAR_JANELA; ++t) {
    for (int c = 0; c < HAR_CANAIS; ++c) {
      const float normalizado = (janela[t][c] - HAR_MEDIA[c]) / HAR_DESVIO[c];
      // roundf arredonda para longe do zero, igual ao notebook
      int q = static_cast<int>(roundf(normalizado / HAR_ENTRADA_ESCALA)) + HAR_ENTRADA_ZERO;
      if (q < -128) q = -128;
      if (q > 127) q = 127;
      saida[t * HAR_CANAIS + c] = static_cast<int8_t>(q);
    }
  }
}
