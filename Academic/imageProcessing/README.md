# F28HS Coursework 1

H00411696: HS16 MONO CODE

## Compilation Instructions

To compile the program, ensure that you have a C compiler installed on your system (e.g., GCC). Use the following command in the terminal:

```sh
gcc -o process process.c
```

This command will compile `process.c` into an executable named `process`.

## Running the Program

Once compiled, you can run the program with the following syntax:

```sh
./process INPUTFILE₁...INPUTFILEn OUTPUTFILE₁...OUTPUTFILEn
```

Here, `INPUTFILE₁...INPUTFILEn` represents one or more input image filenames in HS16 format, and `OUTPUTFILE₁...OUTPUTFILEn` represents the corresponding output filenames where the processed images will be saved. INPUTFILEs should be a filename in the same directory, OUTPUTFILEs are the desired filename.

### Example Usage

To process a single image:

```sh
./process wildcat.hs16 wildcat.hs16
```

To process multiple images:

```sh
./process wildcat.hs16 redh.hs16 processed_wildcat.hs16 processed_redh.hs16
```

For Task CODE, the program will print the C source code representation of the image data to the standard output. You may want to redirect this output to a file by running the command i.e:

```sh
./process wildcat.hs16 redh.hs16 processed_wildcat.hs16 processed_redh.hs16 > output_code.c
```