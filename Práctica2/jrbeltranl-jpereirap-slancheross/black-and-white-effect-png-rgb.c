#define _GNU_SOURCE
// Tomado y adaptado de http://zarb.org/~gc/html/libpng.html

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <omp.h>
#include <png.h>
#include <sched.h>

#define PNG_DEBUG 3



void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;
png_bytep * my_row_pointers;
struct timeval tval_before, tval_after, tval_result, tval_before1, tval_after1, tval_result1, tval_before2, tval_after2, tval_result2;

void read_png_file(char* file_name)
{
        gettimeofday(&tval_before, NULL);
        char header[8];    // 8 is the maximum size that can be checked

        // Lee el archivo y verifica si es un PNG
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8))
                abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


        //Inicializa variables necesarias para libpng
        png_ptr =   png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

        //Inicializa el input/output para el archivo PNG
        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        //Lee la información anterior a los datos de los píxeles como tal
        png_read_info(png_ptr, info_ptr);

        //Almacena información del archivo PNG 
        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);


        // Lectura del archivo PNG
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        // Reserva el espacio necesario para almacenar los datos del archivo PNG por filas
        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));


        png_read_image(png_ptr, row_pointers);
        
        fclose(fp);
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        printf("BnW Read: %ld.%06ld (s) \n ",  (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
}


void write_png_file(char* file_name)
{
        gettimeofday(&tval_before, NULL);
        //printf("Usando CPU %d para guardar", sched_getcpu());
        // Crea el archivo
        FILE *fp = fopen(file_name, "wb");
        if (!fp)
                abort_("[write_png_file] File %s could not be opened for writing", file_name);


        //Inicializa variables necesarias para libpng
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[write_png_file] png_create_write_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[write_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during init_io");

        png_init_io(png_ptr, fp);


        // Escribe el header del archivo PNG
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing header");

        png_set_IHDR(png_ptr, info_ptr, width, height,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        // Escribe los bytes del archivo PNG
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing bytes");

        png_write_image(png_ptr, row_pointers);


        // Termina la escritura
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during end of write");

        png_write_end(png_ptr, NULL);

        // Libera el espacio reservado previamente
        for (y=0; y<height; y++)
                free(row_pointers[y]);
        free(row_pointers);

        fclose(fp);
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        printf("BnW Write: %ld.%06ld (s) \n ",  (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
}


void process_file(int ID, int threads_num)
{
        
        // Se realizan los cambios deseados en la imagen
        int start, end;
        int x = 0;
        int y = 0;
        int channels = 3;
        int rgb_total;  
        float rgb_average;


        png_byte *row, *ptr;

        start = (height/threads_num) * ID;
        end = (height/threads_num) * (ID+1);
        //printf("Hilo %d en Ejecución en el CPU: %d . Empieza en %d y termina en %d \n \n", ID, sched_getcpu(), start, end);

        

        
        for (y=start; y<end; y++) {

                for (x=0; x<width; x++) {

                        rgb_total = 0;
                        rgb_average = 0;

                        row             = row_pointers[y];
                        printf("Row = %hhn", row);
                        ptr             = &(row[x*channels]);
                        
                        
                        // printf("Pixel  %d - %d, Rgb values: %d - %d - %d \n", x, y, ptr[0], ptr[1], ptr[2]); 
                        rgb_total      += ptr[0] + ptr[1] + ptr[2];
                        
                        // Calculando el promedios RGB
                        rgb_average = rgb_total / 3;
                        // printf("Average: %d \n", (int)rgb_average);
                        
                        
                        ptr[0]  = (int)rgb_average;
                        ptr[1]  = (int)rgb_average;
                        ptr[2]  = (int)rgb_average;
                        
                        // printf("Changed to  %d - %d - %d \n",ptr[0], ptr[1], ptr[2]); 
                        // printf("Pixel  %d - %d done\n",x,y);  
                }

        }
        // printf("HILO %d Para la imagen de resolución: %d x %d - ", ID, width, height);
 
}


int main(int argc, char **argv)
{       
        // Verifica los parámetros para ejecutar el programa
        if (argc != 4)
                abort_("Uso: ./Nombre_del_Programa <file_in> <file_out>");

        int threads_num = atoi(argv[3]);
        
        gettimeofday(&tval_before1, NULL);

        read_png_file(argv[1]);

        gettimeofday(&tval_before2, NULL);
                #pragma omp parallel num_threads(threads_num)
                {
                        
                        int ID = omp_get_thread_num();
                        process_file(ID, threads_num);
                }
        gettimeofday(&tval_after2, NULL);
        timersub(&tval_after2, &tval_before2, &tval_result2);
        write_png_file(argv[2]);
        printf("BnW Process:  %ld.%06ld\n ", (long int)tval_result2.tv_sec, (long int)tval_result2.tv_usec);

        gettimeofday(&tval_after1, NULL);
        timersub(&tval_after1, &tval_before1, &tval_result1);
        printf("Para la imagen %d x %d - Con %d Hilos - Tiempo Ejecución: %ld.%06ld (s) \n \n", width, height, threads_num, (long int)tval_result1.tv_sec, (long int)tval_result1.tv_usec);

        return 0;
}