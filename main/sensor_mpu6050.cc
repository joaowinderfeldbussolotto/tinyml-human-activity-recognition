#include "sensor_mpu6050.h"

#include <cstdint>
#include <cstdio>

#include "driver/i2c_master.h"

namespace {

constexpr gpio_num_t kPinoSda = GPIO_NUM_8;
constexpr gpio_num_t kPinoScl = GPIO_NUM_9;
constexpr uint8_t kEnderecoMpu = 0x68;

constexpr uint8_t kRegGiroConfig = 0x1B;
constexpr uint8_t kRegAcelConfig = 0x1C;
constexpr uint8_t kRegAcelInicio = 0x3B;
constexpr uint8_t kRegPowerMgmt = 0x6B;
constexpr uint8_t kRegWhoAmI = 0x75;

// O UCI HAR passa de 2 g (máximo 2,2 g) e chega a 6 rad/s (342 graus/s), mais que as faixas
// padrão do sensor (+-2 g e +-250 graus/s). Por isso uso +-4 g e +-500 graus/s.
constexpr uint8_t kAcelFaixa4g = 0x08;
constexpr uint8_t kGiroFaixa500 = 0x08;
constexpr float kAcelLsbPorG = 8192.0f;
constexpr float kGiroLsbPorGrausPorS = 65.5f;
constexpr float kGrausParaRad = 3.14159265f / 180.0f;

i2c_master_dev_handle_t g_dev = nullptr;

bool escrever(uint8_t reg, uint8_t valor) {
  const uint8_t dados[2] = {reg, valor};
  return i2c_master_transmit(g_dev, dados, sizeof(dados), 100) == ESP_OK;
}

}  // namespace

bool sensor_iniciar() {
  i2c_master_bus_config_t cfg_bus = {};
  cfg_bus.i2c_port = I2C_NUM_0;
  cfg_bus.sda_io_num = kPinoSda;
  cfg_bus.scl_io_num = kPinoScl;
  cfg_bus.clk_source = I2C_CLK_SRC_DEFAULT;
  cfg_bus.glitch_ignore_cnt = 7;
  cfg_bus.flags.enable_internal_pullup = true;

  i2c_master_bus_handle_t bus = nullptr;
  if (i2c_new_master_bus(&cfg_bus, &bus) != ESP_OK) return false;

  i2c_device_config_t cfg_dev = {};
  cfg_dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg_dev.device_address = kEnderecoMpu;
  cfg_dev.scl_speed_hz = 400000;
  if (i2c_master_bus_add_device(bus, &cfg_dev, &g_dev) != ESP_OK) return false;

  uint8_t quem = 0;
  const uint8_t reg = kRegWhoAmI;
  if (i2c_master_transmit_receive(g_dev, &reg, 1, &quem, 1, 100) != ESP_OK) return false;
  printf("MPU6050: WHO_AM_I = 0x%02x\n", quem);

  return escrever(kRegPowerMgmt, 0x00) && escrever(kRegAcelConfig, kAcelFaixa4g) &&
         escrever(kRegGiroConfig, kGiroFaixa500);
}

bool sensor_ler(AmostraImu* amostra) {
  uint8_t b[14];
  const uint8_t reg = kRegAcelInicio;
  if (i2c_master_transmit_receive(g_dev, &reg, 1, b, sizeof(b), 100) != ESP_OK) return false;

  auto s16 = [&](int i) { return static_cast<int16_t>((b[i] << 8) | b[i + 1]); };
  // b[6..7] é a temperatura, que o modelo não usa
  amostra->v[0] = s16(0) / kAcelLsbPorG;
  amostra->v[1] = s16(2) / kAcelLsbPorG;
  amostra->v[2] = s16(4) / kAcelLsbPorG;
  amostra->v[3] = s16(8) / kGiroLsbPorGrausPorS * kGrausParaRad;
  amostra->v[4] = s16(10) / kGiroLsbPorGrausPorS * kGrausParaRad;
  amostra->v[5] = s16(12) / kGiroLsbPorGrausPorS * kGrausParaRad;
  return true;
}
