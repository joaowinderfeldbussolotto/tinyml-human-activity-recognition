# Reconhecimento de atividade humana (HAR) no ESP32-S3

Projeto final de IA Embarcada e Modelos Compactos. Um acelerômetro e um giroscópio (MPU6050) medem o movimento, uma rede neural pequena decide se a pessoa está andando, subindo escada, descendo escada, sentada, em pé ou deitada, e tudo isso roda dentro do ESP32-S3, simulado no Wokwi.

O relatório com os resultados está em [RELATORIO.md](RELATORIO.md) e os slides da apresentação estão em `slides/slides.pdf`.

## 1. Como funciona

O caminho do dado, do sensor até a resposta, tem cinco etapas. Cada uma tem um arquivo em `main/`.

1. `sensor_mpu6050.cc` lê o MPU6050 por I2C, 50 vezes por segundo, e converte para g (aceleração) e rad/s (giroscópio).
2. `janela.cc` guarda as últimas 128 amostras (2,56 s). Depois da primeira janela, uma nova fica pronta a cada 64 amostras.
3. `preprocessamento.cc` normaliza cada um dos 6 canais (tira a média e divide pelo desvio do treino) e converte para int8.
4. `classificador.cc` roda o modelo com o TensorFlow Lite Micro.
5. `main.cc` junta tudo e imprime a atividade na serial.

O treino é separado: o notebook `treinamento/har_treino.ipynb` treina a rede com o dataset UCI HAR, comprime para int8 e gera os arquivos que o firmware usa.

## 2. Estrutura de pastas

```
har-esp32s3/
  treinamento/        notebook, modelo int8, resultados e janelas de demonstração
  main/               firmware (ESP-IDF, C++)
  simulacao/          cenários do Wokwi (movimento real e posturas fixas), o script que gera
                      eles e os logs seriais
  slides/             slides.html, slides.pdf e o script que gera o PDF
  docs/               figuras do relatório
  RELATORIO.md/.pdf   relatório com os resultados
  diagram.json        placa ESP32-S3 + MPU6050 para o Wokwi
  wokwi.toml          configuração do Wokwi
  sdkconfig.defaults  configuração do ESP-IDF
```

Os arquivos `main/modelo_har.cc`, `main/har_parametros.h` e `main/janelas_demo.*` são gerados pelo notebook. Não edite à mão.

## 3. Os dados

Uso o UCI HAR (Anguita et al., 2013, licença CC BY 4.0): 30 voluntários com um celular na cintura, 6 atividades, 50 Hz, janelas de 128 amostras. Pego 6 canais, que são os mesmos que o MPU6050 entrega: aceleração total (x, y, z) e giroscópio (x, y, z). O notebook baixa o dataset sozinho para `dados/` (pasta fora do git). Os 9 voluntários de teste não aparecem no treino.

## 4. Como rodar

### 4.1 Treinar (opcional)

O modelo e os arquivos do firmware já estão no repositório. Para refazer:

```sh
pip install tensorflow numpy matplotlib jupyter
cd treinamento
jupyter nbconvert --to notebook --execute --inplace har_treino.ipynb
```

O notebook usa semente fixa. Nas minhas duas execuções o resultado foi o mesmo, mas outra máquina ou versão do TensorFlow pode dar números um pouco diferentes. Rodei com TensorFlow 2.21 e NumPy 2.4.

### 4.2 Compilar

Precisa do ESP-IDF 5.1 ou mais novo (compilei com 5.5.5). Na pasta `har-esp32s3`:

```sh
idf.py set-target esp32s3
idf.py build
```

Na primeira vez o ESP-IDF baixa o componente `espressif/esp-tflite-micro`, então precisa de internet. No VS Code dá para usar `ESP-IDF: Set Espressif Device Target` e `ESP-IDF: Build Project`. Abra a pasta `har-esp32s3`, não a raiz do repositório.

### 4.3 Simular no Wokwi

Pela extensão do VS Code, com a pasta aberta: `Wokwi: Start Simulator`. A serial mostra primeiro o modo replay (60 janelas reais embutidas no firmware) e depois o modo live (lendo o MPU6050).

No modo live o MPU6050 do Wokwi fica parado nos valores que você colocar nos controles dele. Para ele reproduzir uma gravação real, use um dos cenários do `wokwi-cli`:

```sh
export WOKWI_CLI_TOKEN=...   # token em wokwi.com/dashboard/ci
wokwi-cli --scenario simulacao/cenarios/0_andando.yaml .
```

Cada cenário muda os controles do MPU6050 a cada 20 ms com uma janela real do dataset (uma para cada atividade). Para gerar de novo, rode `python simulacao/gerar_cenarios.py` (precisa de NumPy). Em `simulacao/posturas/` há três cenários mais simples, com valores fixos de aceleração para sentado, em pé e deitado. Os logs de todas as execuções estão em `simulacao/logs/`.

### 4.4 Placa real

Em hardware, apague a linha `CONFIG_NN_ANSI_C=y` do `sdkconfig.defaults` para usar os kernels otimizados do ESP32-S3 (veja a seção 6) e ligue o MPU6050 em SDA = GPIO 8 e SCL = GPIO 9. Eu não testei em placa real. Na placa, o eixo x do sensor deve apontar para cima quando a pessoa está em pé, como no celular do dataset.

## 5. Resultados

Todos os números abaixo foram medidos nas execuções descritas e estão detalhados no relatório.

| Modelo | Tamanho | Acurácia no teste (2947 janelas) |
| --- | --- | --- |
| Float32 | 21860 B | 89,45% |
| Faixa dinâmica | 10120 B | 89,41% |
| Int8 (o do chip) | 9856 B | 89,48% |

No Wokwi, rodando as 60 janelas de demonstração, o chip acertou 58 e a saída int8 foi idêntica à do PC nas 60. A arena do TFLite Micro usa 4524 bytes. O ponto mais fraco do modelo é separar sentado de em pé (73% de acerto para sentado).

## 6. Limitações

- O MPU6050 do Wokwi não se mexe sozinho. Só o cenário (ou o replay) coloca movimento de verdade nele.
- Com o padrão do exemplo do TFLite Micro, o Wokwi não executou certo os kernels em assembly do ESP32-S3 nos meus testes (a saída do modelo ficava travada num valor). Por isso o projeto usa `CONFIG_NN_ANSI_C=y`. O tempo de inferência medido no simulador (cerca de 279 ms) vale para esses kernels em C, e não diz quanto leva numa placa real.
- Não testei em placa física nem com um MPU6050 real.
- O dataset foi gravado com um celular na cintura. Com o sensor em outro lugar ou em outra orientação o modelo precisaria de novos dados.

## 7. Fluxo de trabalho (git flow)

O repositório segue o git flow. A `main` guarda só versões prontas, a `develop` junta o trabalho em andamento e cada tarefa foi feita numa branch `feature/...` que sai da `develop` e volta para ela com `git merge --no-ff`. As branches são `feature/treino` (notebook e modelo), `feature/firmware` (firmware e cenários do Wokwi) e `feature/docs` (relatório e slides). A versão final recebeu a tag `v1.0`. Cada integrante fez commits com o próprio usuário.

## 8. Créditos

- Dataset: Anguita, Ghio, Oneto, Parra e Reyes-Ortiz, *A Public Domain Dataset for Human Activity Recognition Using Smartphones*, ESANN 2013. Licença CC BY 4.0.
- TensorFlow Lite Micro para ESP32 pelo componente [espressif/esp-tflite-micro](https://github.com/espressif/esp-tflite-micro) (Apache 2.0).
