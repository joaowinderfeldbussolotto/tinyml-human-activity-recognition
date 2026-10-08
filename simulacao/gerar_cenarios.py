"""Gera cenarios do wokwi-cli que reproduzem janelas reais do UCI HAR no MPU6050 simulado.

Para cada atividade pega a primeira janela de demonstracao dela (treinamento/janelas_demo.npz)
e escreve um YAML que muda os controles do MPU6050 a cada 20 ms, o mesmo periodo de
amostragem do firmware.

Uso, na pasta har-esp32s3:
    python simulacao/gerar_cenarios.py
"""
import math
import pathlib

import numpy as np

RAIZ = pathlib.Path(__file__).resolve().parent.parent
SAIDA = RAIZ / "simulacao" / "cenarios"
NOMES = ["andando", "subindo_escada", "descendo_escada", "sentado", "em_pe", "deitado"]
CONTROLES = ["accelX", "accelY", "accelZ", "rotationX", "rotationY", "rotationZ"]
PERIODO_MS = 20

dados = np.load(RAIZ / "treinamento" / "janelas_demo.npz")
x, y = dados["x"], dados["y"]
SAIDA.mkdir(parents=True, exist_ok=True)

for classe, nome in enumerate(NOMES):
    i = int(np.flatnonzero(y == classe)[0])
    janela = x[i]
    def ajustar(t):
        passos = []
        for c, controle in enumerate(CONTROLES):
            valor = float(janela[t, c])
            if c >= 3:
                valor = math.degrees(valor)
            passos += [
                "  - set-control:",
                "      part-id: imu1",
                f"      control: {controle}",
                f"      value: {valor:.5f}",
            ]
        return passos

    linhas = [
        f"# Janela {i} das 60 de demonstracao (atividade real: {nome.replace('_', ' ')}).",
        "# Aceleracao em g; giroscopio convertido de rad/s para graus/s.",
        "name: " + nome,
        "version: 1",
        "steps:",
        "  # a primeira amostra entra antes do firmware ler o sensor pela primeira vez",
        *ajustar(0),
        "  - wait-serial: 'MODO LIVE'",
    ]
    for t in range(1, janela.shape[0]):
        linhas += ajustar(t)
        linhas.append(f"  - delay: {PERIODO_MS}ms")
    linhas += ["  - wait-serial: '(prob'", "  - delay: 200ms"]
    (SAIDA / f"{classe}_{nome}.yaml").write_text("\n".join(linhas) + "\n")
    print(f"{nome}: janela {i}, {janela.shape[0]} amostras")
