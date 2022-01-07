#!/bin/bash

clear

sudo apt-get install libpng-dev

gcc black-and-white-effect-png-rgb.c -o ompBnW -fopenmp -l png
./ompBnW Input/Gato720.png Output/OMPBnWGato720 2 >> tiemposEjecucion.txt
./ompBnW Input/Gato1080.png Output/OMPBnWGato1080 2 >> tiemposEjecucion.txt
./ompBnW Input/Gato4k.png Output/OMPBnWGato4k 2 >> tiemposEjecucion.txt

./ompBnW Input/Gato720.png Output/OMPBnWGato720 4 >> tiemposEjecucion.txt
./ompBnW Input/Gato1080.png Output/OMPBnWGato1080 4 >> tiemposEjecucion.txt
./ompBnW Input/Gato4k.png Output/OMPBnWGato4k 4 >> tiemposEjecucion.txt

./ompBnW Input/Gato720.png Output/OMPBnWGato720 8 >> tiemposEjecucion.txt
./ompBnW Input/Gato1080.png Output/OMPBnWGato1080 8 >> tiemposEjecucion.txt
./ompBnW Input/Gato4k.png Output/OMPBnWGato4k 8 >> tiemposEjecucion.txt

./ompBnW Input/Gato720.png Output/OMPBnWGato720 16 >> tiemposEjecucion.txt
./ompBnW Input/Gato1080.png Output/OMPBnWGato1080 16 >> tiemposEjecucion.txt
./ompBnW Input/Gato4k.png Output/OMPBnWGato4k 16 >> tiemposEjecucion.txt


gcc blur-effect-png-rgb.c -o ompBlur -fopenmp -l png
./ompBlur Input/Gato720.png Output/OMPBlurGato720 2 >> tiemposEjecucion.txt
./ompBlur Input/Gato1080.png Output/OMPBlurGato1080 2 >> tiemposEjecucion.txt
./ompBlur Input/Gato4k.png Output/OMPBlurGato4k 2 >> tiemposEjecucion.txt

./ompBlur Input/Gato720.png Output/OMPBlurGato720 4 >> tiemposEjecucion.txt
./ompBlur Input/Gato1080.png Output/OMPBlurGato1080 4 >> tiemposEjecucion.txt
./ompBlur Input/Gato4k.png Output/OMPBlurGato4k 4 >> tiemposEjecucion.txt

./ompBlur Input/Gato720.png Output/OMPBlurGato720 8 >> tiemposEjecucion.txt
./ompBlur Input/Gato1080.png Output/OMPBlurGato1080 8 >> tiemposEjecucion.txt
./ompBlur Input/Gato4k.png Output/OMPBlurGato4k 8 >> tiemposEjecucion.txt

./ompBlur Input/Gato720.png Output/OMPBlurGato720 16 >> tiemposEjecucion.txt
./ompBlur Input/Gato1080.png Output/OMPBlurGato1080 16 >> tiemposEjecucion.txt
./ompBlur Input/Gato4k.png Output/OMPBlurGato4k 16 >> tiemposEjecucion.txt