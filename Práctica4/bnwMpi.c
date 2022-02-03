// Tomado y adaptado de http://zarb.org/~gc/html/libpng.html

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

#define PNG_DEBUG 3
#include <png.h>

#include <mpi.h>

int x, y;
int width, height;
png_byte color_type;
png_byte bit_depth;
png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;
png_bytep * my_row_pointers;


void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

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

void from_array_to_bnw_png(int image_rgb_averages[])
{
        // Se toman los valores de promedio RGB y se guardan en la estructura pngbyte nuevamente
        int x = 0;
        int y = 0;
        int channels = 3;
        png_byte *row, *ptr;
        int counter = 0;

        for (y=0; y<height-1; y++) {

                for (x=0; x<width; x++) {

                        row             = row_pointers[y];
                        ptr             = &(row[x*channels]);

                        ptr[0]  = image_rgb_averages[counter];
                        ptr[1]  = image_rgb_averages[counter];
                        ptr[2]  = image_rgb_averages[counter];
                        counter++;
                }
        }
}

int main(int argc, char **argv) {

        // Verifica los parámetros para ejecutar el programa
        if (argc != 3)
                abort_("Uso: ./Nombre_del_Programa <file_in> <file_out>");

        int tasks, iam, i, root=0;
        struct timeval tval_before, tval_after, tval_result;
        int * image_as_array_global;
        int sizeArray, image_size;
        int * image_as_array_local;
        int scatter_size;
        int * image_rgb_averages_global;
        int * image_rgb_averages_local;
        int rgb_scatter_size;



        MPI_Status status;
        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &tasks);
        MPI_Comm_rank(MPI_COMM_WORLD, &iam);



        if (iam == 0) {
                printf("\nReading image file...");
                read_png_file(argv[1]);

                image_size = width*height;
                sizeArray = image_size*3;
                scatter_size = sizeArray/tasks;
                rgb_scatter_size = scatter_size/3;

                image_as_array_global = malloc(sizeArray * sizeof(int));
                image_as_array_local = malloc(scatter_size * sizeof(int));
                image_rgb_averages_global = malloc(image_size * sizeof(int));
                image_rgb_averages_local = malloc((scatter_size/3) * sizeof(int));

                // printf("\nThe image has %d pixels", image_size);
                // printf("\nSo the array with all the RGB values has %d elements", sizeArray);

                MPI_Bcast( &image_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

                //printf("\nSending size value: %d", image_size);
                printf("\nTurning image into array...\n");
                image_as_array_global = image_to_array(width, height);
                
                printf("\nScattering %d elements in %d nodes, so %d for each node \n", sizeArray, tasks, scatter_size);
                MPI_Scatter(image_as_array_global, scatter_size, MPI_INT, image_as_array_local, scatter_size, MPI_INT, root, MPI_COMM_WORLD);

                printf("\nGathering the arrays from other nodes\n");

                // from_array_to_bnw_png(image_rgb_averages_global);
                // write_png_file(argv[2]);

        } else {

                MPI_Comm_rank(MPI_COMM_WORLD, &iam);
                MPI_Bcast( &image_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

                sizeArray = image_size*3;
                scatter_size = sizeArray/tasks;
                rgb_scatter_size = scatter_size/3;

                image_as_array_global = malloc(sizeArray * sizeof(int));
                image_as_array_local = malloc(scatter_size * sizeof(int));
                image_rgb_averages_global = malloc(image_size * sizeof(int));
                image_rgb_averages_local = malloc((scatter_size/3) * sizeof(int));

                //printf("\n In node %d, preparing to receive %d elements in the array\n",iam, scatter_size);
                MPI_Scatter(image_as_array_global, scatter_size, MPI_INT, image_as_array_local, scatter_size, MPI_INT, root, MPI_COMM_WORLD); 
        }

        int counter = 0;
        int average = 0;

        for (int i=0; i<(scatter_size); i=i+3) {
                average = (image_as_array_local[i] + image_as_array_local[i+1] + image_as_array_local[i+2]) / 3;
                image_rgb_averages_local[counter] = average;
                //printf("\nIn node %d, Turning RGB: %d,%d,%d into %d", iam, image_as_array_local[i] ,image_as_array_local[i+1], image_as_array_local[i+2], average );
                counter++;
        }

        MPI_Gather(image_rgb_averages_local, rgb_scatter_size , MPI_INT, image_rgb_averages_global, rgb_scatter_size, MPI_INT, root, MPI_COMM_WORLD);
        
        if (iam==0){
                from_array_to_bnw_png(image_rgb_averages_global);
                printf("\nWriting image file...");
                write_png_file(argv[2]);

        }

        MPI_Finalize();


        // gettimeofday(&tval_before, NULL);
        // //printf("\nProcessing image array..");
        // process_file(image_rgb_averages_global);
        // gettimeofday(&tval_after, NULL);
        // timersub(&tval_after, &tval_before, &tval_result);

        // //printf("\nTurning array into png structure...");
        // from_array_to_bnw_png(image_rgb_averages_global);

        // printf("\nWriting image file...");
        // write_png_file(argv[2]);



        // printf("\nTiempo de procesamiento de efecto blanco y negro: %ld.%06ld\n \n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);

        return 0;
}

