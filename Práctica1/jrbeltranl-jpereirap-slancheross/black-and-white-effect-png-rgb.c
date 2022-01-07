// Tomado y adaptado de http://zarb.org/~gc/html/libpng.html

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

#define PNG_DEBUG 3
#include <png.h>

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
struct timeval tval_before, tval_after, tval_result, tval_before1, tval_after1, tval_result1;
int * image_as_array;

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
        printf("BnW Read: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
}


void write_png_file(char* file_name)
{
        gettimeofday(&tval_before, NULL);
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

        

        fclose(fp);

        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        printf("BnW Write: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
}


void process_file()
{
        gettimeofday(&tval_before, NULL);
        // Se realizan los cambios deseados en la imagen
        
        int channels = 3;
        int rgb_total;  
        float rgb_average;
        

        png_byte *row, *ptr_izq, *ptr, *ptr_der;

        

        for (y=0; y<height-1; y++) {

                for (x=0; x<width; x++) {
                
                        rgb_total = 0;
                        rgb_average = 0;

                        row             = row_pointers[y];
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
        printf("Para la imagen de resolución: %d x %d - ", width, height);
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        printf("BnW Process: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
}

int * image_to_array(int width, int height){

        static int image_as_array[24883200];
        int x = 0;
        int y = 0;
        int k = 0;
        int counter = 0;
        png_byte *row;
        png_byte *ptr;


        for (y=0; y<height-1; y++) {

                for (x=0; x<width; x++) {
                
                        row             = row_pointers[y];
                        ptr             = &(row[x*3]);
                        
                        for(k=0; k<3; k++){
                                image_as_array[counter] = ptr[k];
                                counter++;
                        }
                }

        }      
        return image_as_array;
}

void from_array_to_bnw_png()
{ 
        printf("final");
        //Se toman los valores de promedio RGB y se guardan en la estructura pngbyte nuevamente
        int x = 0;
        int y = 0;
        int channels = 3;
        png_byte *row, *ptr;
        int counter = 0;

        for (y=0; y<height-1; y++) {
                //printf("%d", y);
                for (x=0; x<width; x++) {

                        row             = row_pointers[y];
                        ptr             = &(row[x*channels]);
                        
                        ptr[0]  = image_as_array[counter];
                        counter++;
                        ptr[1]  = image_as_array[counter];
                        counter++;
                        ptr[2]  = image_as_array[counter];
                        counter++;
                }
        }
        printf("Final2");
}

int main(int argc, char **argv)
{       
        
        // Verifica los parámetros para ejecutar el programa
        if (argc != 4)
                abort_("Uso: ./Nombre_del_Programa <file_in> <file_out>");

        
        gettimeofday(&tval_before1, NULL);

        printf("1 \n");
        read_png_file(argv[1]);
        image_as_array = image_to_array(width, height);
        // for (int i =0; i<8294400; i++){
        //         printf("%d, ", image_as_array[i]);
        // }
        
        printf("2 \n");
        process_file();
        write_png_file(argv[2]);

        printf("3 \n");
        from_array_to_bnw_png();
        
        printf("4 \n");
        write_png_file(argv[3]);

        
        gettimeofday(&tval_after1, NULL);
        timersub(&tval_after1, &tval_before1, &tval_result1);
        printf("Tiempo de ejecución de efecto blanco y negro: %ld.%06ld\n \n", (long int)tval_result1.tv_sec, (long int)tval_result1.tv_usec);


        // Libera el espacio reservado previamente
        for (y=0; y<height; y++)
                free(row_pointers[y]);
        free(row_pointers);


        return 0;
}
