/*
 * MorceNOX-ASTRO™
 * Copyright (C) 2026 Amilcar Antonio Mesquita Rizk amilcar.rizk@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://gnu.org>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "number_helper.h"


// Função que converte GMS (inteiros) para Graus Decimais (double)
double get_decimal_degrees(Grau c) {
    // 1. Identifica se a coordenada é negativa pelo campo dos graus
    double sinal = (c.grau < 0) ? -1.0 : 1.0;
    
    // 2. Obtém o valor absoluto dos graus para não errar a soma
    double grausAbsolutos = abs(c.grau);
    
    // 3. Calcula as frações decimais usando divisores do tipo double (60.0 e 3600.0)
    double fracaoMinutos = c.min / 60.0;
    double fracaoSegundos = c.sec / 3600.0;
    
    // 4. Junta as partes e aplica o sinal correto
    return sinal * (grausAbsolutos + fracaoMinutos + fracaoSegundos);
}


void get_degree_str(double grauDecimal, char* resultado) {
    // Define o sinal (+ ou -)
    char sinal = (grauDecimal >= 0) ? '+' : '-';
    
    // Trabalha com o valor absoluto para extrair graus e minutos
    double valorAbsoluto = fabs(grauDecimal);
    
    // Extrai a parte inteira dos graus
    int graus = (int)valorAbsoluto;
    
    // Calcula os minutos (o que sobrou multiplicado por 60) e arredonda
    int minutos = (int)round((valorAbsoluto - graus) * 60.0);
    
    // Se o arredondamento dos minutos der 60, ajusta para 0 e soma 1 grau
    if (minutos == 60) {
        minutos = 0;
        graus += 1;
    }
    
    // Formata a string no padrão desejado: sinal seguido de 2 dígitos para graus e minutos
    sprintf(resultado, "%c%02d:%02d", sinal, graus, minutos);
}





Hora get_fmt_hour(double h) {
    Hora hfmt = {0, 0, 0};

    h += 0.5 / 3600000.0;

    int hours = (int)h;
    int minutes = (int)((h - hours) * 60.0);
    double sec_decimal = (((h - hours) * 60.0) - minutes) * 60.0;
    int seconds = (int)sec_decimal;

    hfmt.hora = hours;
    hfmt.min = minutes;
    hfmt.sec = seconds;

    return hfmt;
}


int mod_a(int n, int div) {
    int x = n % div;
    return x == 0 ? div: x; 
}


char* intToRoman(int num) {
    // Dynamically allocate memory for the string (max length for 1-3999 is 15 chars + null terminator)
    char* result = (char*)malloc(16 * sizeof(char));
    if (result == NULL) return NULL; // Handle allocation failure
    
    result[0] = '\0'; // Initialize as empty string

    int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    char* symbols[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};

    for (int i = 0; i < 13 && num > 0; i++) {
        while (num >= values[i]) {
            strcat(result, symbols[i]);
            num -= values[i];
        }
    }
    return result; // Caller is now responsible for freeing this memory
}


int romanToInt(char *s) {
    int values[256] = {0};
    values['I'] = 1; values['V'] = 5; values['X'] = 10;
    values['L'] = 50; values['C'] = 100; values['D'] = 500; values['M'] = 1000;

    int total = 0, prevValue = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        int currentValue = values[(unsigned char)s[i]];
        total += currentValue;
        if (currentValue > prevValue) {
            total -= 2 * prevValue; // Subtraction rule (e.g., IV = 1 + 5 - 2*1 = 4)
        }
        prevValue = currentValue;
    }
    return total;
}
