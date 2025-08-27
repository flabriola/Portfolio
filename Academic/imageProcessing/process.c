/* This coursework specification, and the example code provided during the
 * course, is Copyright 2024 Heriot-Watt University.
 * Distributing this coursework specification or your solution to it outside
 * the university is academic misconduct and a violation of copyright law. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define IMG_FORMAT "HS16"
#define MAX_FILENAME 255

/* The RGB values of a pixel. */
struct Pixel {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
};

/* An image loaded from a file. */
struct Image {
    int width;
    int height;
    struct Pixel **pixels;
    struct Image *next;
};

struct Image *fip;   // Pointer to first input Image struct
struct Image *fop;   // Pointer to first output Image struct

/* Free a struct Image */
void free_image(struct Image *img)
{
    /* Inner pointers must be freed before the structure holding it */
    /* As both the array of pointers and the individual rows were allocated with calloc, they must be freed individually */
    for (int i = 0; i < img->height; i++)
        free(img->pixels[i]);
    free(img->pixels);
    free(img);
}

/* Free Images */
void free_list (struct Image *fp)
{
    while(fp != NULL){

        if(fp->next != NULL){
            struct Image *temp = fp->next;
            int w = fp->width;
            free_image(fp);
            fp = temp;
        }
        int w = fp->width;
        free_image(fp);
        fp = NULL;

    }
}

/* Create a dinamically allocated Pixel bitmap that can 
 * decide the numbers of rows at run-time.  */
struct Pixel **makeBitmap(int m, int n)
{
    // pointer to array of pointers to Pixels array
    struct Pixel **newb = calloc(m, sizeof(struct Pixel *));
    for (int i = 0; i < m; i++)
        newb[i] = calloc(n, sizeof(struct Pixel));
    // returns an array of m arrays each containing n Pixels
    return newb;
}

/* Read data from file into Pixel bitmap. */
int readBitmap(FILE * f, struct Pixel **B, int m, int n)
{
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            /* Read one 16-bit unsigned integer from file f into value and 
             * insert into each color inside of each Pixel */
            for (int c = 0; c < 3; c++){

                uint16_t val;

                /* Check for error, EOF would also be an error as the logic does not allow EOF 
                 * to be reached, as iteration accounts for exact amount of reads the 
                 * method should perform (m*n). 
                 * On Error function returns one, which is dealt with when function is called,
                 * as memory for pointers unreacheable in this scope would have to be freed. 
                 * fread is used instead of fscanf so uint16_t data type can be used, 
                 * rather than short int. */
                if(fread(&val, sizeof(uint16_t), 1, f) != 1 ){
                    return 1;
                }

                switch(c){
                    case 0: (B[i][j]).red = val; break;
                    case 1: (B[i][j]).green = val; break;
                    case 2: (B[i][j]).blue = val; break;
                }

            }

    return 0;
}

/* Opens and reads an image file, returning a pointer to a new struct Image.
 * On error, prints an error message and returns NULL. */
struct Image *load_image(const char *filename)
{
    /* Open the file for reading */
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "File %s could not be opened.\n", filename);
        return NULL;
    }

    /* Allocate the Image object, and read the image from the file. */

    /* Check that image file is the correct image format. */
    /* Allocate format of image in file, extra char is needed in memory allocation of string. */
    char format[5];
    if(fscanf(f, "%4s", format) != 1 || strncmp(format, IMG_FORMAT, 4) != 0){
        fprintf(stderr, "File %s is not in HS16 format.\n", filename);
        fclose(f);
        return NULL;
    }

    /* Check that width and height are in the correct format. */
    int width, height;
    if(fscanf(f, "%d %d", &width, &height) != 2){
        fprintf(stderr, "File %s does not provide appropiate width and height dimensions.\n", filename);
        fclose(f);
        return NULL;
    }

    /* Dinamically allocate the Image struct and dimension fields. */
    struct Image *img = malloc(sizeof *img);
    if (img == NULL) {
        fprintf(stderr, "Unable to allocate memory for Image struct.\n");
        fclose(f);
        return NULL;
    }

    img->width = width;
    img->height = height;

    /* Allocate memory for Pixel bitmap */
    img->pixels = makeBitmap(height, width);
    if (img->pixels == NULL) {
        fprintf(stderr, "Unable to allocate memory for pixel data.\n");
        free(img); // Free the previously allocated Image struct
        fclose(f);
        return NULL;
    }

    /* Read pixel data into Pixel bitmap */
    int read_data = readBitmap(f, img->pixels, height, width);
    if (read_data == 1) {
        fprintf(stderr, "Failed to read pixel data from file %s.\n", filename);
        free(img);
        fclose(f);
        return NULL;
    }

    /* Close the file */
    fclose(f);
    return img;
}

/* Write img to file filename. Return true on success, false on error. */
bool save_image(const struct Image *img, const char *filename)
{
    /* Open the file for writing */
    FILE *f = fopen(filename, "w");
    if (f == NULL)
        return false;

    /* Write header */

    fprintf(f, "%s\t", IMG_FORMAT);
    fprintf(f, "%i\t", img->width);
    fprintf(f, "%i ", img->height);

    /* Write Pixel values */

    for (int i = 0; i < img->height; i++)
        for (int j = 0; j < img->width; j++)
            /* Allocate 16-bit unsigned integer from each color inside of each Pixel 
             * to value and write value in file */
            for (int c = 0; c < 3; c++){

                uint16_t value;
                switch(c){
                    case 0: value = (*img).pixels[i][j].red; break;
                    case 1: value = (*img).pixels[i][j].green; break;
                    case 2: value = (*img).pixels[i][j].blue; break;
                }
                
                /* fwrite is used instead of fprintf because it can specify the data type and size 
                 * (fprintf would use short unsigned int, which in some machines may not be 16-bits)*/
                if(fwrite(&value, sizeof(uint16_t), 1, f) != 1 ){
                    return false;
                }

            }

    return true;
}

/* Allocate a new struct Image and copy an existing struct Image's contents
 * into it. On error, returns NULL. 
 * This function has similar functionality to save_image, but it rather copies 
 * Image content to another Image struct instead of file. */
struct Image *copy_image(const struct Image *source)
{
    if(source == NULL || source->pixels == NULL){
        return NULL;
    }
    
    /* Allocate space for new Image struct */
    struct Image *img_copy = malloc(sizeof *img_copy);
    if (img_copy == NULL) {
        return NULL; // Memory allocation failed
    }
    /* Copy width, height and pixel data to Image struct */

    img_copy->width = source->width;
    img_copy->height = source->height;

    /* Allocate memory for the pixel data */
    img_copy->pixels = makeBitmap(img_copy->height, img_copy->width);
    if (img_copy->pixels == NULL) {
        free(img_copy); // Free the Image struct if pixel allocation fails
        return NULL;
    }
    
    for (int i = 0; i < source->height; i++)
        for (int j = 0; j < source->width; j++)
            for (int c = 0; c < 3; c++){
                switch(c){
                    case 0: (*img_copy).pixels[i][j].red = (*source).pixels[i][j].red; break;
                    case 1: (*img_copy).pixels[i][j].green = (*source).pixels[i][j].green; break;
                    case 2: (*img_copy).pixels[i][j].blue = (*source).pixels[i][j].blue; break;
                }
            }
    
    return img_copy;
}

/* Perform your first task.
 * Returns a new struct Image containing equal width and height and pixel bit map converted
 * from colour to monochrome. Each pixel value is converted to the weighted sum of the red, 
 * green and blue components: 0.299R + 0.587G + 0.114B. On error returns NULL. */
struct Image *apply_MONO(const struct Image *source)
{
    if(source == NULL || source->pixels == NULL){
        return NULL;
    }

    struct Image *mono_image = copy_image(source);

    /* Iterate through pixel data and set all colors in each pixel to the calculated grey value */
    for (int i = 0; i < source->height; i++)
        for (int j = 0; j < source->width; j++){

            // Caluclate the weighted sum of the RGB components
            float greyValue = 0.299 * (*source).pixels[i][j].red + 0.587 * (*source).pixels[i][j].green + 0.114 * (*source).pixels[i][j].blue;

            // Convert the floating-point grey value to an integer
            uint16_t grey = (uint16_t)greyValue;

            // Set each RGB component to the grey value
            (*mono_image).pixels[i][j].red = (*mono_image).pixels[i][j].green = (*mono_image).pixels[i][j].blue = grey;

        }
    
    return mono_image;
}

/* Transform 16-bit integers into 8-bit representation to fit RGB range of 0-255 */
uint8_t eightBits(uint16_t color){

    uint8_t reduced = (uint8_t)((color * 255) / 65535);
    return reduced;

}

/* Perform your second task.
 * Function accepts an Image struct, printing it's dimensions and pixel data as C source code.
 * Returns false on error. */
bool apply_CODE(const struct Image *source)
{
    if(source == NULL || source->pixels == NULL){
        return false;
    }

    fprintf(stdout, "const int image_width = %d;\n", source->width);
    fprintf(stdout, "const int image_height = %d;\n", source->height);
    fprintf(stdout, "const struct Pixel image_data[%d][%d] = {\n", source->height, source->width);

    /* Each line starts with whitespace to represent indentation */
    char line[70] = "    ";

    for (int i = 0; i < source->height; i++) {

        for (int j = 0; j < source->width; j++) {
            // Allocate pixel data to local struct Pixel for quick access
            struct Pixel p = source->pixels[i][j];

            // Allocate pixel data to string, containing initial space and comma at the end
            char pix[18];
            // Add values to string in curly brackets and a comma
            snprintf(pix, sizeof(pix), "{%d, %d, %d}, ", eightBits(p.red), eightBits(p.green), eightBits(p.blue));

            // Check if adding pix to line would exceed maximum line size 
            if(strlen(line) + strlen(pix) > sizeof(line) - 1){
                fprintf(stdout, "%s\n", line);
                strcpy(line, "    "); // Reset line with indentation
            }
            strcat(line, pix); // Append pix to line

        }
    }

    line[strlen(line)-2] = '\0';    // Remove comma from last array value
    fprintf(stdout, "%s\n", line);
    fprintf(stdout, "};\n");
    return true;
}

/* Allocate new Image struct to linked list */
void push(struct Image **list, struct Image *img)
{
    if (*list == NULL) {
        *list = img;
        return;
    }

    struct Image *f = *list;
    while (f->next != NULL) {
        f = f->next; 
    }
    f->next = img; 
}


int main(int argc, char *argv[])
{

    /* Check command-line arguments (exclusing the program name arguments should be even. 
     * For every input file there should be an output file) */
    if (argc < 3 || (argc - 1) % 2 != 0) {
        fprintf(stderr, "Usage: process INPUTFILE₁...INPUTFILEn OUTPUTFILE₁...OUTPUTFILEn\n");
        return 1;
    }

    /* Load input images to linked list */
    for (int i = 0; i < (argc -1)/2; i++){

        /* Load the input image */
        struct Image *in_img = load_image(argv[i+1]);
        if(in_img == NULL)
            return 1;
        push(&fip, in_img);

    }
    
    /* Apply the first process and load images to output linked list */

    struct Image *img = fip;
    for (int i = 0; i < (argc - 1) / 2; i++){

        struct Image *out_img = apply_MONO(img);
        if (out_img == NULL) {
            fprintf(stderr, "First process failed for file %s.\n", argv[i+1]);
            free_list(fip);
            return 1;
        }
        push(&fop, out_img);
        img = img->next;

    }

    /* Iterate through output linked list and apply second and third processes */

    img = fop;
    for (int i = 0; i < (argc - 1) / 2; i++){

        /* Apply the second process  */
        if (!apply_CODE(img)) {
            fprintf(stderr, "Second process failed for file %s .\n", argv[(argc-1)/2+i+1]);
            free_list(fip);
            free_list(fop);
            return 1;
        }

        printf("\n");   // line between images code

        /* Save the output image */
        if (!save_image(img, argv[(argc-1)/2+i+1])) {
            fprintf(stderr, "Saving image to %s failed.\n", argv[(argc-1)/2+i+1]);
            free_list(fip);
            free_list(fop);
            return 1;
        }

        img = img->next;

    }

    free_list(fip);
    free_list(fop);
    return 0;
}
