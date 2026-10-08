idf_component_register(
    SRCS main.cc sensor_mpu6050.cc janela.cc preprocessamento.cc classificador.cc
         modelo_har.cc janelas_demo.cc
    PRIV_REQUIRES esp_driver_i2c esp_timer
    INCLUDE_DIRS ".")
