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
#include "number_helper.h"


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
