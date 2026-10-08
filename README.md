# Reconhecimento de atividade humana (HAR) no ESP32-S3

Projeto final de IA Embarcada e Modelos Compactos. Um acelerômetro e um giroscópio (MPU6050) medem o movimento, uma rede neural pequena decide se a pessoa está andando, subindo escada, descendo escada, sentada, em pé ou deitada, e tudo isso roda dentro do ESP32-S3, simulado no Wokwi.

Os slides da apresentação estão em `slides/slides.pdf`.

## Sumário

- [1. Como funciona](#1-como-funciona)
- [2. Estrutura de pastas](#2-estrutura-de-pastas)
- [3. Os dados](#3-os-dados)
- [4. Como rodar](#4-como-rodar)
  - [4.1 Treinar (opcional)](#41-treinar-opcional)
  - [4.2 Compilar](#42-compilar)
  - [4.3 Simular no Wokwi](#43-simular-no-wokwi)
- [5. Resultados](#5-resultados)
- [6. Limitações](#6-limitações)
- [7. Wokwi CLI](#7-wokwi-cli)
  - [Instalação](#instalação)
  - [Token](#token)
  - [Como usar](#como-usar)
  - [Como construir um cenário](#como-construir-um-cenário)
  - [Documentação](#documentação)
- [8. Créditos](#8-créditos)

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
  docs/               figuras do treino (janelas, curvas e matriz de confusão)
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

No modo live o MPU6050 do Wokwi fica parado nos valores que você colocar nos controles dele. Para ele reproduzir uma gravação real, use um dos cenários do `wokwi-cli` (veja a seção 7):

```sh
export WOKWI_CLI_TOKEN=...   # token em wokwi.com/dashboard/ci
wokwi-cli --scenario simulacao/cenarios/0_andando.yaml .
```

Cada cenário muda os controles do MPU6050 a cada 20 ms com uma janela real do dataset (uma para cada atividade). Para gerar de novo, rode `python simulacao/gerar_cenarios.py` (precisa de NumPy). Em `simulacao/posturas/` há três cenários mais simples, com valores fixos de aceleração para sentado, em pé e deitado. Os logs de todas as execuções estão em `simulacao/logs/`.

## 5. Resultados

Todos os números abaixo foram medidos nas execuções descritas.

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

## 7. Wokwi CLI

O `wokwi-cli` roda a simulação do Wokwi pelo terminal, sem a interface gráfica. Serve para testes automáticos e para executar cenários.

**O que são cenários.** Um cenário é um arquivo YAML com uma lista de passos que o `wokwi-cli` executa durante a simulação, como "espere esta mensagem na serial, mude o valor deste sensor, espere 500 ms". Eles simulam o mundo em volta do chip: um sensor simulado fica parado no valor inicial até alguém mudá-lo, e o cenário faz isso sem precisar mexer na interface.

### Instalação

```sh
# Linux e macOS
curl -L https://wokwi.com/ci/install.sh | sh

# Windows (PowerShell)
iwr https://wokwi.com/ci/install.ps1 -useb | iex
```

Também dá para baixar o executável na página de releases do GitHub e colocá-lo numa pasta do `PATH`. Confira com `wokwi-cli --version`.

### Token

Crie um token em [wokwi.com/dashboard/ci](https://wokwi.com/dashboard/ci) e defina a variável `WOKWI_CLI_TOKEN`:

```sh
export WOKWI_CLI_TOKEN=wok_...        # Linux e macOS
$env:WOKWI_CLI_TOKEN="wok_..."        # Windows (PowerShell)
```

O token é uma senha. Não coloque num arquivo do repositório.

### Como usar

Na pasta do projeto, que precisa ter `wokwi.toml` e `diagram.json` (o comando `wokwi-cli init` cria os dois):

```sh
wokwi-cli .
```

| Opção | O que faz |
| --- | --- |
| `--timeout <ms>` | tempo máximo da simulação (padrão 30000) |
| `--scenario <arquivo>` | executa um cenário |
| `--serial-log-file <arquivo>` | salva a saída serial em um arquivo |
| `--expect-text <texto>` | falha se o texto não aparecer na serial |
| `--fail-text <texto>` | falha se o texto aparecer na serial |

### Como construir um cenário

Um cenário tem `name`, `version: 1` e a lista `steps`:

```yaml
name: exemplo
version: 1
steps:
  - wait-serial: 'Pronto'
  - set-control:
      part-id: dht
      control: humidity
      value: 39
  - delay: 500ms
```

| Passo | O que faz |
| --- | --- |
| `delay` | espera um tempo (`500ms`, `2s`) |
| `wait-serial` | espera um texto aparecer na serial |
| `write-serial` | envia texto para a serial |
| `set-control` | muda o controle de uma peça (`part-id`, `control`, `value`) |
| `expect-pin` | confere o valor de um pino |

O `part-id` é o `id` da peça no `diagram.json`. Os nomes dos controles de cada peça estão na página dela na documentação. Para passos repetitivos, como reproduzir uma gravação amostra por amostra, vale gerar o YAML com um script. A documentação marca os cenários como alfa, e os tempos são tempos simulados.

### Documentação

- [Primeiros passos](https://docs.wokwi.com/wokwi-ci/getting-started)
- [Instalação do CLI](https://docs.wokwi.com/wokwi-ci/cli-installation)
- [Uso do CLI](https://docs.wokwi.com/wokwi-ci/cli-usage)
- [Cenários de automação](https://docs.wokwi.com/wokwi-ci/automation-scenarios)
- [Exemplo de página de peça (MPU6050)](https://docs.wokwi.com/parts/wokwi-mpu6050)

## 8. Créditos

- Dataset: Anguita, Ghio, Oneto, Parra e Reyes-Ortiz, *A Public Domain Dataset for Human Activity Recognition Using Smartphones*, ESANN 2013. Licença CC BY 4.0.
- TensorFlow Lite Micro para ESP32 pelo componente [espressif/esp-tflite-micro](https://github.com/espressif/esp-tflite-micro) (Apache 2.0).
