#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

// Formato de Ponto Fixo: 17.14 
#define F (1 << 14) // Fator de conversão (2^14 = 16384)

// CONVERSOES

// Converte o inteiro 'n' para ponto fixo
#define INT_TO_FP(n) ((n) * F)

// Converte o ponto fixo 'x' para inteiro
#define FP_TO_INT_ZERO(x) ((x) / F)

// Converte o ponto fixo 'x' para inteiro
#define FP_TO_INT_NEAR(x) ((x) >= 0 ? ((x) + F / 2) / F : ((x) - F / 2) / F)


// ADICAO E SUBTRACAO

// Adiciona dois números de ponto fixo (x + y)
#define ADD_FP(x, y) ((x) + (y))

// Subtrai dois números de ponto fixo (x - y)
#define SUB_FP(x, y) ((x) - (y))

// Adiciona um ponto fixo 'x' e um inteiro 'n'
#define ADD_INT(x, n) ((x) + (n) * F)

// Subtrai um inteiro 'n' de um ponto fixo 'x'
#define SUB_INT(x, n) ((x) - (n) * F)


// MULTIPLICACAO E DIVISAO

// Multiplica dois números de ponto fixo (x * y)
#define MULT_FP(x, y) ( ((int64_t)(x)) * (y) / F )

// Multiplica um ponto fixo 'x' por um inteiro 'n'
#define MULT_INT(x, n) ((x) * (n))

// Divide dois números de ponto fixo (x / y)
#define DIV_FP(x, y) ( ((int64_t)(x)) * F / (y) )

// Divide um ponto fixo 'x' por um inteiro 'n'
#define DIV_INT(x, n) ((x) / (n))

#endif // threads/fixed-point.h
