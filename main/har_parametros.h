#pragma once
// Gerado por treinamento/har_treino.ipynb. Não editar a mão.

#define HAR_JANELA 128
#define HAR_CANAIS 6
#define HAR_CLASSES 6

static const float HAR_MEDIA[HAR_CANAIS] = {8.03511322e-01f, 3.39222401e-02f, 8.72843936e-02f, -1.97473331e-03f, -3.34464276e-04f, 9.36466444e-04f};
static const float HAR_DESVIO[HAR_CANAIS] = {4.17423725e-01f, 3.99392217e-01f, 3.47354263e-01f, 4.12593693e-01f, 3.87178540e-01f, 2.59813368e-01f};

static const float HAR_ENTRADA_ESCALA = 7.3985725641e-02f;
static const int HAR_ENTRADA_ZERO = -2;
static const float HAR_SAIDA_ESCALA = 3.9062500000e-03f;
static const int HAR_SAIDA_ZERO = -128;

static const char* const HAR_NOMES[HAR_CLASSES] = {"andando", "subindo escada", "descendo escada", "sentado", "em pe", "deitado"};
