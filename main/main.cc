#include <cstdint>
#include <cstdio>

#include "classificador.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "janela.h"
#include "janelas_demo.h"
#include "preprocessamento.h"
#include "sensor_mpu6050.h"

namespace {

constexpr int kPeriodoAmostraMs = 20;  // 50 Hz, a mesma taxa do dataset
// Em 1, imprime cada amostra lida (só serve para conferir o sensor simulado).
constexpr bool kImprimirAmostras = false;

// Roda as janelas reais embutidas no firmware. Não depende do sensor: serve para medir a
// acurácia do modelo dentro do chip.
void modo_replay() {
  printf("\n=== MODO REPLAY: %d janelas reais do conjunto de teste ===\n", HAR_DEMO_N);
  printf("  # | real             | previsto         | prob | pre(us) | inf(us) | igual ao PC\n");

  int acertos = 0, iguais = 0;
  int64_t soma_pre = 0, soma_inf = 0;
  static int8_t entrada[HAR_JANELA * HAR_CANAIS];

  for (int i = 0; i < HAR_DEMO_N; ++i) {
    const int64_t t0 = esp_timer_get_time();
    preprocessar(JANELAS_DEMO[i], entrada);
    const int64_t t1 = esp_timer_get_time();
    Resultado r;
    if (!classificar(entrada, &r)) {
      printf("Invoke falhou na janela %d\n", i);
      return;
    }
    const int64_t t2 = esp_timer_get_time();

    bool igual = true;
    for (int k = 0; k < HAR_CLASSES; ++k) igual &= (r.bruto[k] == DEMO_REFERENCIA_PC[i][k]);
    acertos += (r.classe == DEMO_ROTULOS[i]);
    iguais += igual;
    soma_pre += t1 - t0;
    soma_inf += t2 - t1;

    printf("%3d | %-16s | %-16s | %.2f | %7lld | %7lld | %s\n", i, HAR_NOMES[DEMO_ROTULOS[i]],
           HAR_NOMES[r.classe], static_cast<double>(r.probabilidade), static_cast<long long>(t1 - t0),
           static_cast<long long>(t2 - t1), igual ? "sim" : "nao");
    // sem isso a tarefa ociosa nunca roda e o watchdog reinicia o chip
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  printf("Acuracia no chip: %d/%d (%.1f%%)\n", acertos, HAR_DEMO_N, 100.0 * acertos / HAR_DEMO_N);
  printf("Saida int8 identica a do PC: %d/%d janelas\n", iguais, HAR_DEMO_N);
  printf("Tempo medio: pre-processamento %lld us, inferencia %lld us\n",
         static_cast<long long>(soma_pre / HAR_DEMO_N), static_cast<long long>(soma_inf / HAR_DEMO_N));
}

// A inferência leva centenas de milissegundos, bem mais que os 20 ms entre amostras. Por isso a
// leitura do sensor fica numa tarefa própria, de prioridade maior, e só avisa a tarefa principal
// quando uma janela está pronta. Assim nenhuma amostra se perde enquanto o modelo roda.
float g_janela_pronta[HAR_JANELA][HAR_CANAIS];
TaskHandle_t g_tarefa_principal = nullptr;

void tarefa_sensor(void*) {
  static JanelaDeslizante janela;
  TickType_t ultimo = xTaskGetTickCount();
  int64_t n = 0;
  while (true) {
    AmostraImu a;
    if (!sensor_ler(&a)) {
      printf("Falha ao ler o MPU6050\n");
    } else {
      if (kImprimirAmostras) {
        printf("S,%lld,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n", static_cast<long long>(n), static_cast<double>(a.v[0]),
               static_cast<double>(a.v[1]), static_cast<double>(a.v[2]), static_cast<double>(a.v[3]),
               static_cast<double>(a.v[4]), static_cast<double>(a.v[5]));
      }
      ++n;
      // A janela só é sobrescrita 1,28 s depois, muito mais que o tempo que a tarefa principal
      // leva para copiá-la no pré-processamento.
      if (janela.adicionar(a)) {
        janela.copiar(g_janela_pronta);
        xTaskNotifyGive(g_tarefa_principal);
      }
    }
    vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(kPeriodoAmostraMs));
  }
}

// Lê o MPU6050 a 50 Hz e classifica a cada janela completa.
void modo_live() {
  printf("\n=== MODO LIVE: lendo o MPU6050 a %d Hz ===\n", 1000 / kPeriodoAmostraMs);
  fflush(stdout);

  g_tarefa_principal = xTaskGetCurrentTaskHandle();
  // Dá tempo de quem alimenta o sensor (o cenário do Wokwi, que reage a esta mensagem) ajustar a
  // primeira amostra antes da primeira leitura.
  vTaskDelay(pdMS_TO_TICKS(5));
  xTaskCreate(tarefa_sensor, "sensor", 4096, nullptr, 5, nullptr);

  static int8_t entrada[HAR_JANELA * HAR_CANAIS];
  int64_t classificacoes = 0;
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    preprocessar(g_janela_pronta, entrada);
    Resultado r;
    if (classificar(entrada, &r)) {
      ++classificacoes;
      printf("[live] janela %lld: %-16s (prob %.2f)\n", static_cast<long long>(classificacoes),
             HAR_NOMES[r.classe], static_cast<double>(r.probabilidade));
      fflush(stdout);
    }
  }
}

}  // namespace

extern "C" void app_main(void) {
  printf("\n=== HAR no ESP32-S3: reconhecimento de atividade com acelerometro ===\n");

  if (!classificador_iniciar()) {
    printf("Erro ao iniciar o modelo\n");
    return;
  }
  printf("Modelo int8 carregado. Arena usada: %d bytes\n", classificador_arena_usada());

  modo_replay();

  if (!sensor_iniciar()) {
    printf("MPU6050 nao encontrado. Verifique o diagram.json.\n");
    return;
  }
  modo_live();
}
