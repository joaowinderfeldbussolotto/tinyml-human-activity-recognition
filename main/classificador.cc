#include "classificador.h"

#include <cstdio>

#include "modelo_har.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

namespace {

constexpr int kTamanhoArena = 20 * 1024;
alignas(16) uint8_t g_arena[kTamanhoArena];

tflite::MicroInterpreter* g_interpretador = nullptr;
TfLiteTensor* g_entrada = nullptr;
TfLiteTensor* g_saida = nullptr;

}  // namespace

bool classificador_iniciar() {
  const tflite::Model* modelo = tflite::GetModel(modelo_har);
  if (modelo->version() != TFLITE_SCHEMA_VERSION) {
    printf("Versao do modelo (%lu) diferente da suportada (%d)\n",
           static_cast<unsigned long>(modelo->version()), TFLITE_SCHEMA_VERSION);
    return false;
  }

  static tflite::MicroMutableOpResolver<5> resolvedor;
  if (resolvedor.AddReshape() != kTfLiteOk || resolvedor.AddConv2D() != kTfLiteOk ||
      resolvedor.AddMaxPool2D() != kTfLiteOk || resolvedor.AddFullyConnected() != kTfLiteOk ||
      resolvedor.AddSoftmax() != kTfLiteOk) {
    return false;
  }

  static tflite::MicroInterpreter interpretador(modelo, resolvedor, g_arena, kTamanhoArena);
  g_interpretador = &interpretador;
  if (g_interpretador->AllocateTensors() != kTfLiteOk) {
    printf("AllocateTensors() falhou: arena de %d bytes pequena demais\n", kTamanhoArena);
    return false;
  }
  g_entrada = g_interpretador->input(0);
  g_saida = g_interpretador->output(0);
  return true;
}

int classificador_arena_usada() { return static_cast<int>(g_interpretador->arena_used_bytes()); }

bool classificar(const int8_t* entrada, Resultado* resultado) {
  for (int i = 0; i < HAR_JANELA * HAR_CANAIS; ++i) g_entrada->data.int8[i] = entrada[i];
  if (g_interpretador->Invoke() != kTfLiteOk) return false;

  int melhor = 0;
  for (int k = 0; k < HAR_CLASSES; ++k) {
    resultado->bruto[k] = g_saida->data.int8[k];
    if (resultado->bruto[k] > resultado->bruto[melhor]) melhor = k;
  }
  resultado->classe = melhor;
  resultado->probabilidade = (resultado->bruto[melhor] - HAR_SAIDA_ZERO) * HAR_SAIDA_ESCALA;
  return true;
}
