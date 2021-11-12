// Tomado y adaptado de http://zarb.org/~gc/html/libpng.html

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

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

void read_png_file(char* file_name)
{
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
}

void read_png_file1(char* file_name)
{
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

        
        my_row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
                my_row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, my_row_pointers);

        fclose(fp);
}

void write_png_file(char* file_name)
{
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
}


void process_file()
{
        // Se realizan los cambios deseados en la imagen
        
        // Verifica que la imagen tenga el tipo de color correcto
        if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
                abort_("[process_file] input file is PNG_COLOR_TYPE_RGB but must be PNG_COLOR_TYPE_RGBA "
                       "(lacks the alpha channel)");

        if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA)
                abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)",
                       PNG_COLOR_TYPE_RGBA, png_get_color_type(png_ptr, info_ptr));
        
        
        int red_total, blue_total, green_total;  
        float red_average, blue_average, green_average;
        

        png_byte *row, *ptr_izq, *ptr, *ptr_der;

        // memcpy (destination, source, sizeof(source));


        // printf("Height: %d, widht: %d \n", height, width);

        for (y=0; y<height-1; y++) {
                
                for (x=0; x<width; x++) {

                        red_average     = 0.0;
                        blue_average    = 0.0;
                        green_average   = 0.0;
                        red_total = 0;
                        green_total =0;
                        blue_total = 0;

                        if(y==0 && x==0){
                                // Para el central
                                row             = my_row_pointers[y];
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr[0] + ptr_der[0];
                                green_total     += ptr[1] + ptr_der[1];
                                blue_total      += ptr[2] + ptr_der[2];
                                
                                // Para el de abajo
                                row             = my_row_pointers[y+1];
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr[0] + ptr_der[0];
                                green_total     += ptr[1] + ptr_der[1];
                                blue_total      += ptr[2] + ptr_der[2];

                                // Calculando los promedios RGB
                                red_average     = (int)red_total/4;
                                blue_average    = (int)blue_total/4;
                                green_average   = (int)green_total/4;

                        }else if (y==0 && x==width){
                                // Para el central
                                row             = my_row_pointers[y];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                red_total       += ptr_izq[0] + ptr[0];
                                green_total     += ptr_izq[1] + ptr[1];
                                blue_total      += ptr_izq[2] + ptr[2];

                                // Para el de abajo
                                row             = my_row_pointers[y+1];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                red_total       += ptr_izq[0] + ptr[0];
                                green_total     += ptr_izq[1] + ptr[1];
                                blue_total      += ptr_izq[2] + ptr[2];

                                // Calculando los promedios RGB
                                red_average     = (int)red_total/4;
                                blue_average    = (int)blue_total/4;
                                green_average   = (int)green_total/4;

                        }else if(y==0){

                                // Para el central
                                row             = my_row_pointers[y];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr_izq[0] + ptr[0] + ptr_der[0];
                                green_total     += ptr_izq[1] + ptr[1] + ptr_der[1];
                                blue_total      += ptr_izq[2] + ptr[2] + ptr_der[2];

                                // Para el de abajo
                                row             = my_row_pointers[y+1];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr_izq[0] + ptr[0] + ptr_der[0];
                                green_total     += ptr_izq[1] + ptr[1] + ptr_der[1];
                                blue_total      += ptr_izq[2] + ptr[2] + ptr_der[2];

                                // Calculando los promedios RGB
                                red_average     = (int)red_total/6;
                                blue_average    = (int)blue_total/6;
                                green_average   = (int)green_total/6;

                        }else if(x==0 && y==height){

                                // Para el de arriba
                                row             = my_row_pointers[y-1];
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr[0] + ptr_der[0];
                                green_total     += ptr[1] + ptr_der[1];
                                blue_total      += ptr[2] + ptr_der[2];

                                // Para el central
                                row             = my_row_pointers[y];
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr[0] + ptr_der[0];
                                green_total     += ptr[1] + ptr_der[1];
                                blue_total      += ptr[2] + ptr_der[2];

                                // Calculando los promedios RGB
                                red_average     = (int)red_total/4;
                                blue_average    = (int)blue_total/4;
                                green_average   = (int)green_total/4;

                        }else if (x==0){

                                // Para el de arriba
                                row             = my_row_pointers[y-1];
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr[0] + ptr_der[0];
                                green_total     += ptr[1] + ptr_der[1];
                                blue_total      += ptr[2] + ptr_der[2];

                                // Para el central
                                row             = my_row_pointers[y];
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr[0] + ptr_der[0];
                                green_total     += ptr[1] + ptr_der[1];
                                blue_total      += ptr[2] + ptr_der[2];

                                // Para el de abajo
                                row             = my_row_pointers[y+1];
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr[0] + ptr_der[0];
                                green_total     += ptr[1] + ptr_der[1];

                                // Calculando los promedios RGB
                                red_average     = (int)red_total/6;
                                blue_average    = (int)blue_total/6;
                                green_average   = (int)green_total/6;

                        }else if (x==width){

                                // Para el de arriba
                                row             = my_row_pointers[y-1];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                red_total       += ptr_izq[0] + ptr[0];
                                green_total     += ptr_izq[1] + ptr[1];
                                blue_total      += ptr_izq[2] + ptr[2];

                                // Para el central
                                row             = my_row_pointers[y];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                red_total       += ptr_izq[0] + ptr[0];
                                green_total     += ptr_izq[1] + ptr[1];
                                blue_total      += ptr_izq[2] + ptr[2];

                                // Para el de abajo
                                row             = my_row_pointers[y+1];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                red_total       += ptr_izq[0] + ptr[0];
                                green_total     += ptr_izq[1] + ptr[1];
                                blue_total      += ptr_izq[2] + ptr[2];

                                // Calculando los promedios RGB
                                red_average     = (int)red_total/6;
                                blue_average    = (int)blue_total/6;
                                green_average   = (int)green_total/6;

                        }else if (y==height){

                                // Para el de arriba
                                row             = my_row_pointers[y-1];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr_izq[0] + ptr[0] + ptr_der[0];
                                green_total     += ptr_izq[1] + ptr[1] + ptr_der[1];
                                blue_total      += ptr_izq[2] + ptr[2] + ptr_der[2];

                                // Para el central
                                row             = my_row_pointers[y];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr_izq[0] + ptr[0] + ptr_der[0];
                                green_total     += ptr_izq[1] + ptr[1] + ptr_der[1];
                                blue_total      += ptr_izq[2] + ptr[2] + ptr_der[2];

                                // Calculando los promedios RGB
                                red_average     = (int)red_total/6;
                                blue_average    = (int)blue_total/6;
                                green_average   = (int)green_total/6;

                        }else if (x==width && y==height){

                                // Para el de arriba
                                row             = my_row_pointers[y-1];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                red_total       += ptr_izq[0] + ptr[0];
                                green_total     += ptr_izq[1] + ptr[1];
                                blue_total      += ptr_izq[2] + ptr[2];

                                // Para el central
                                row             = my_row_pointers[y];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                red_total       += ptr_izq[0] + ptr[0];
                                green_total     += ptr_izq[1] + ptr[1];
                                blue_total      += ptr_izq[2] + ptr[2];

                                // Calculando los promedios RGB
                                red_average     = (int)red_total/4;
                                blue_average    = (int)blue_total/4;
                                green_average   = (int)green_total/4;

                        }else {
                        
                                // Para el de arriba
                                row             = my_row_pointers[y-1];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr_izq[0] + ptr[0] + ptr_der[0];
                                green_total     += ptr_izq[1] + ptr[1] + ptr_der[1];
                                blue_total      += ptr_izq[2] + ptr[2] + ptr_der[2];

                                // Para el central
                                row             = my_row_pointers[y];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr_izq[0] + ptr[0] + ptr_der[0];
                                green_total     += ptr_izq[1] + ptr[1] + ptr_der[1];
                                blue_total      += ptr_izq[2] + ptr[2] + ptr_der[2];

                                // Para el de abajo
                                row             = my_row_pointers[y+1];
                                ptr_izq         = &(row[(x-1)*4]);
                                ptr             = &(row[x*4]);
                                ptr_der         = &(row[(x+1)*4]);
                                red_total       += ptr_izq[0] + ptr[0] + ptr_der[0];
                                green_total     += ptr_izq[1] + ptr[1] + ptr_der[1];
                                blue_total      += ptr_izq[2] + ptr[2] + ptr_der[2];
                                

                                // printf("Izq: %d, %d, %d. Centro: %d, %d, %d. Der: %d, %d, %d \n",
                                //         ptr_izq[0], ptr_izq[1], ptr_izq[2],
                                //         ptr[0], ptr[1], ptr[2],
                                //         ptr_der[0], ptr_der[1], ptr_der[2] );

                                
                                // Calculando los promedios RGB
                                red_average     = (int)red_total/9;
                                blue_average    = (int)blue_total/9;
                                green_average   = (int)green_total/9;
                        }

                        // Volvemos al pixel "original" y modificamos los valores
                        row     = row_pointers[y];
                        ptr     = &(row[x*4]);
                        
                        ptr[0]  = red_average;
                        ptr[1]  = green_average;
                        ptr[2]  = blue_average;
                                
                        // printf("Average  %f - %f - %f \n",red_average, green_average, blue_average);
                        // printf("Changed to  %d - %d - %d \n",ptr[0], ptr[1], ptr[2]); 
                        // printf("Pixel  %d - %d done\n",x,y);  
                             
                }
        }
        
}


int main(int argc, char **argv)
{       
        // Verifica los parámetros para ejecutar el programa
        if (argc != 3)
                abort_("Uso: Nombre_del_Programa <file_in> <file_out>");

        read_png_file(argv[1]);
        read_png_file1(argv[1]);
        process_file();
        write_png_file(argv[2]);

        return 0;
}