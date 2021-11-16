#!/bin/bash

clear

sudo apt-get install libpng-dev

gcc black-and-white-effect-png-rgb.c -o black-and-white-effect-png-rgb -l png
./black-and-white-effect-png-rgb Gato720.png BnWGato720 >> tiemposEjecucion.txt
./black-and-white-effect-png-rgb Gato1080.png BnWGato1080 >> tiemposEjecucion.txt
./black-and-white-effect-png-rgb Gato4k.png BnWGato4k >> tiemposEjecucion.txt
gcc blur-effect-png-rgb.c -o blur-effect-png-rgb -l png
./blur-effect-png-rgb Gato720.png BlurGato720 >> tiemposEjecucion.txt
./blur-effect-png-rgb Gato1080.png BlurGato1080 >> tiemposEjecucion.txt
./blur-effect-png-rgb Gato4k.png BlurGato4k >> tiemposEjecucion.txt



