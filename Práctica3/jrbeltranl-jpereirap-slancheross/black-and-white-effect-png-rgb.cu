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
// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>


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
int * d_width, *d_height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;
struct timeval tval_before, tval_after, tval_result, tval_before1, tval_after1, tval_result1;
int * image_rgb_averages;
int n;
int * d_n;

void read_png_file(char* file_name)
{
        gettimeofday(&tval_before, NULL);
        
        
        char header[8];    // 8 is the maximum size that can be checked

        // Lee el archivo y verifica si es un PNG
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        fread(header, 1, 8, fp);
        // if (png_sig_cmp(header, 0, 8))
        //         abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


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
        //printf("BnW Read: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
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

        // Libera el espacio reservado previamente
        for (y=0; y<height; y++)
                free(row_pointers[y]);
        free(row_pointers);

        fclose(fp);

        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        //printf("BnW Write: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
}


__global__ void process_file(int * d_image_as_array, int * d_image_rgb_averages, int * d_width, int * d_height, int * d_n)
{
        // Se realizan los cambios deseados en la imagen
        //Verificar los datos recibidos
        //printf("Width = %d , Height = %d ", *d_width, *d_height);
        //printf("n = %d \n", *d_n );
        int array_size = (*d_width)*(*d_height-1)*3;
        int block_size = array_size / *d_n;
        int counter = (threadIdx.x * block_size) / 3;
        //printf("Thread number %d working from %d to %d, storing initiates at %d \n", threadIdx.x, threadIdx.x * block_size, (threadIdx.x*block_size) + block_size, counter );
        

        for (int i=(threadIdx.x * block_size); i<((threadIdx.x*block_size) + block_size); i=i+3) {
                d_image_rgb_averages[counter] = (d_image_as_array[i] + d_image_as_array[i+1] + d_image_as_array[i+2]) / 3;
                counter++;
        }
        
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

int main(int argc, char **argv)
{       
        // Verifica los parámetros para ejecutar el programa
        if (argc != 4)
                abort_("Uso: ./Nombre_del_Programa <file_in> <file_out> <num_hilos>");

        // Timer de inicio total
        gettimeofday(&tval_before1, NULL);
        n = atoi(argv[3]);

        // Lee la imagen y pasa los datos a un array para pasar este parámetro al kernel
        read_png_file(argv[1]);

        printf("Para la imagen de resolución %d x %d, con %d hilos: \n", width, height, n);
        int * image_as_array;
        int * d_image_as_array;
        image_as_array = image_to_array(width, height);
        

        // Creamos un array que va a recibir la respuesta del kernel y su copia para el device
        int * d_image_rgb_averages;
        int image_rgb_averages_size = sizeof(int) * width*height;
        image_rgb_averages = (int *)malloc(image_rgb_averages_size);

        // CUDA
        // Tiempo de inicio para el procesamiento de la imagen
        gettimeofday(&tval_before, NULL);
        // Reservar el espacio para las copias en el device
        int int_size = sizeof(int);
        int image_as_array_size = sizeof(int)*width*height*3;
        
        cudaMalloc((void **)&d_image_as_array, image_as_array_size);
        cudaMalloc((void **)&d_width, int_size);        
        cudaMalloc((void **)&d_height, int_size);
        cudaMalloc((void **)&d_n, int_size);
        cudaMalloc((void **)&d_image_rgb_averages, image_rgb_averages_size);

        // Copiar los inputs al device
        cudaMemcpy(d_image_as_array, image_as_array, image_as_array_size, cudaMemcpyHostToDevice);
        cudaMemcpy(d_width, &width, int_size, cudaMemcpyHostToDevice);
        cudaMemcpy(d_height, &height, int_size, cudaMemcpyHostToDevice); 
        cudaMemcpy(d_n, &n, int_size, cudaMemcpyHostToDevice);        

        // Lanzar el kernel
        process_file<<<1,n>>>(d_image_as_array, d_image_rgb_averages, d_width, d_height, d_n);

        // Copiar los resultados de vuelta al host
        cudaMemcpy(image_rgb_averages, d_image_rgb_averages, image_rgb_averages_size, cudaMemcpyDeviceToHost);              
        // Para verificar los valores devueltos
        // for (int i =0; i<8294400; i++){
        //         printf("%d, ", image_rgb_averages[i]);
        // }

        // Tiempo de fin para el procesamiento de la imagen
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);
        printf("BnW Process: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);

        // Se pasa del array al pngbyte nuevamente
        from_array_to_bnw_png(image_rgb_averages);

        // Escritura de la imagen con los resultados
        write_png_file(argv[2]);

        // Tiempo de fin total
        gettimeofday(&tval_after1, NULL);
        timersub(&tval_after1, &tval_before1, &tval_result1);
        printf("Tiempo de ejecución total: %ld.%06ld\n \n", (long int)tval_result1.tv_sec, (long int)tval_result1.tv_usec);

        // Limpieza
        cudaFree(d_width);
        cudaFree(d_height);
        cudaFree(d_n);
        cudaFree(d_image_as_array);
        cudaFree(d_image_rgb_averages);
        
        return 0;
}
