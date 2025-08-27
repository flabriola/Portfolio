/*
 * MasterMind implementation: template; see comments below on which parts need to be completed
 * CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
 * This repo: https://gitlab-student.macs.hw.ac.uk/f28hs-2021-22/f28hs-2021-22-staff/f28hs-2021-22-cwk2-sys

 * Compile:
 gcc -c -o lcdBinary.o lcdBinary.c
 gcc -c -o master-mind.o master-mind.c
 gcc -o master-mind master-mind.o lcdBinary.o
 * Run:
 sudo ./master-mind

 OR use the Makefile to build
 > make all
 and run
 > make run
 and test
 > make test

 ***********************************************************************
 * The Low-level interface to LED, button, and LCD is based on:
 * wiringPi libraries by
 * Copyright (c) 2012-2013 Gordon Henderson.
 ***********************************************************************
 * See:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
*/

/* ======================================================= */
/* SECTION: includes                                       */
/* ------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

/* --------------------------------------------------------------------------- */
/* Config settings */
/* you can use CPP flags to e.g. print extra debugging messages */
/* or switch between different versions of the code e.g. digitalWrite() in Assembler */
#define DEBUG
#undef ASM_CODE

// =======================================================
// Tunables
// PINs (based on BCM numbering)
// For wiring see CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
// GPIO pin for green LED
#define LED 13
// GPIO pin for red LED
#define LED2 5
// GPIO pin for button
#define BUTTON 19
// =======================================================
// delay for loop iterations (mainly), in ms
// in mili-seconds: 0.2s
#define DELAY 200
// in micro-seconds: 3s
#define TIMEOUT 5000000
// =======================================================
// APP constants   ---------------------------------
// number of colours and length of the sequence
#define COLS 3
#define SEQL 3
// =======================================================

// generic constants

#ifndef TRUE
#define TRUE (1 == 1)
#define FALSE (1 == 2)
#endif

#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

#define INPUT 0
#define OUTPUT 1

#define LOW 0
#define HIGH 1

// =======================================================
// Wiring (see inlined initialisation routine)

#define STRB_PIN 24
#define RS_PIN 25
#define DATA0_PIN 23
#define DATA1_PIN 10
#define DATA2_PIN 27
#define DATA3_PIN 22

/* ======================================================= */
/* SECTION: constants and prototypes                       */
/* ------------------------------------------------------- */

// =======================================================
// char data for the CGRAM, i.e. defining new characters for the display

static unsigned char newChar[8] =
    {
        0b11111,
        0b10001,
        0b10001,
        0b10101,
        0b11111,
        0b10001,
        0b10001,
        0b11111,
};

/* Constants */

static const int colors = COLS;
static const int seqlen = SEQL;

static char *color_names[] = {"R", "G", "B"};
static char *full_color_names[] = {"Red", "Green", "Blue"};

static int *theSeq = NULL;

static int *seq1, *seq2, *cpy1, *cpy2;

volatile sig_atomic_t timeOut = 0;

/* --------------------------------------------------------------------------- */

// data structure holding data on the representation of the LCD
struct lcdDataStruct
{
  int bits, rows, cols;
  int rsPin, strbPin;
  int dataPins[8];
  int cx, cy;
};

static int lcdControl;

/* ***************************************************************************** */
/* INLINED fcts from wiringPi/devLib/lcd.c: */
// HD44780U Commands (see Fig 11, p28 of the Hitachi HD44780U datasheet)

#define LCD_CLEAR 0x01
#define LCD_HOME 0x02
#define LCD_ENTRY 0x04
#define LCD_CTRL 0x08
#define LCD_CDSHIFT 0x10
#define LCD_FUNC 0x20
#define LCD_CGRAM 0x40
#define LCD_DGRAM 0x80

// Bits in the entry register

#define LCD_ENTRY_SH 0x01
#define LCD_ENTRY_ID 0x02

// Bits in the control register

#define LCD_BLINK_CTRL 0x01
#define LCD_CURSOR_CTRL 0x02
#define LCD_DISPLAY_CTRL 0x04

// Bits in the function register

#define LCD_FUNC_F 0x04
#define LCD_FUNC_N 0x08
#define LCD_FUNC_DL 0x10

#define LCD_CDSHIFT_RL 0x04

// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices

#define PI_GPIO_MASK (0xFFFFFFC0)

static unsigned int gpiobase;
static uint32_t *gpio;

static int timed_out = 0;

/* ------------------------------------------------------- */
// misc prototypes

int failure(int fatal, const char *message, ...);
void waitForEnter(void);
int waitForButton(uint32_t *gpio, int button);

/* ======================================================= */
/* SECTION: hardware interface (LED, button, LCD display)  */
/* ------------------------------------------------------- */
/* low-level interface to the hardware */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Either put them in a separate file, lcdBinary.c, and use   */
/* inline Assembler there, or use a standalone Assembler file */
/* You can also directly implement them here (inline Asm).    */
/* ********************************************************** */

/* These are just prototypes; you need to complete the code for each function */

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
void digitalWrite(uint32_t *gpio, int pin, int value);

/* set the @mode@ of a GPIO @pin@ to INPUT or OUTPUT; @gpio@ is the mmaped GPIO base address */
void pinMode(uint32_t *gpio, int pin, int mode);

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
/* can use digitalWrite(), depending on your implementation */
void writeLED(uint32_t *gpio, int led, int value);

/* read a @value@ (LOW or HIGH) from pin number @pin@ (a button device); @gpio@ is the mmaped GPIO base address */
int readButton(uint32_t *gpio, int button);

/* wait for a button input on pin number @button@; @gpio@ is the mmaped GPIO base address */
/* can use readButton(), depending on your implementation */
int waitForButton(uint32_t *gpio, int button);

/* ======================================================= */
/* SECTION: game logic                                     */
/* ------------------------------------------------------- */
/* AUX fcts of the game logic */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* initialise the secret sequence; by default it should be a random sequence */
void initSeq()
{
  srand(time(NULL)); // Start the random number generator.

  theSeq = (int *)malloc(seqlen * sizeof(int)); // Allocate memory for the sequence.
  if (theSeq == NULL)
  {
    fprintf(stderr, "Failed to allocate memory");
    exit(EXIT_FAILURE);
  }

  // Set each element in the sequence to a random number between 1 and C (number of colors) (inclusive)
  for (int i = 0; i < seqlen; i++)
  {
    theSeq[i] = rand() % colors + 1;
  }
}

/* display the sequence on the terminal window, using the format from the sample run in the spec */
void showSeq(int *seq)
{
  char str[30] = "Secret:  ";
  // Add each number in the sequence to string, then print it
  for (int i = 0; i < seqlen; i++)
  {
    char num[3] = "";
    sprintf(num, "%s ", color_names[seq[i] - 1]);
    strcat(str, num);
  }
  printf("%s\n", str);
}

#define NAN1 8
#define NAN2 9

/* counts how many entries in seq2 match entries in seq1 */
/* returns exact and approximate matches, either both encoded in one value, */
/* or as a pointer to a pair of values */
int *countMatches(int *seq1, int *seq2)
{
  int *matches = (int *)malloc(2 * sizeof(int)); // Allocate space for an array with two elements
  if (matches == NULL)
  {
    fprintf(stderr, "Failed to allocate memory");
    exit(EXIT_FAILURE);
  }

  // Copy values into local array so we can manipulate it

  int seq[seqlen];
  int seqx[seqlen];

  for (int i = 0; i < seqlen; i++)
  {
    seq[i] = seq1[i];
    seqx[i] = seq2[i];
  }

  // Remove elements that are equal in the exact position and increase exact count

  for (int i = 0; i < seqlen; i++)
  {
    if (seq1[i] == seq2[i])
    {
      matches[0]++;
      seq[i] = 0;
      seqx[i] = -1;
    }
  }

  // Remove elements that have an equal in the sequence and increase approximate 

  for (int i = 0; i < seqlen; i++)
  {
    for (int j = 0; j < seqlen; j++)
    {
      if (seqx[i] == seq[j])
      {
        matches[1]++;
        seq[j] = 0;
      }
    }
  }

  return matches;
}

/* show the results from calling countMatches on seq1 and seq1 */
void showMatches(int *seq1, int *seq2)
{
  int *matches = countMatches(seq1, seq2);

  char exc[20] = "";
  char app[20] = "";

  sprintf(exc, "%d exact", matches[0]);
  sprintf(app, "%d approximate", matches[1]);

  printf("%s\n", exc);
  printf("%s\n", app);

  free(matches);
}

/* parse an integer value as a list of digits, and put them into @seq@ */
/* needed for processing command-line with options -s or -u            */
void readSeq(int *seq, int val)
{
  // Save the value in a temporary integer so that it can be manipulated
  int temp = val;
  for (int i = seqlen; i > 0; i--)
  {
    // Extract the last digit from integer and save it to last item of array
    seq[i - 1] = temp % 10;
    // Remove last digit
    temp /= 10;
  }
}

/* read a guess sequence fron stdin and store the values in arr */
/* only needed for testing the game logic, without button input */
int readNum(int max)
{
  int num = 0;

  while (1)
  {
    scanf("%d", &num);
    if (num < 1 || num > max)
      printf("Please enter number from 1 to %d\n", max);
    else
      break;
  }

  return num;
}

/* ======================================================= */
/* SECTION: TIMER code                                     */
/* ------------------------------------------------------- */
/* TIMER code */

/* timestamps needed to implement a time-out mechanism */
static uint64_t startT, stopT;

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* you may need this function in timer_handler() below  */
/* use the libc fct gettimeofday() to implement it      */ // DELETE?******************
uint64_t timeInMicroseconds()
{
  /* ***  COMPLETE the code here  ***  */
}

/* this should be the callback, triggered via an interval timer, */
/* that is set-up through a call to sigaction() in the main fct. */
void timer_handler(int signum)
{
  // set the timeOut flag to one, which is read in the waitForButton function so it knows when to stop waiting
  timeOut = 1;
}

/* initialise time-stamps, setup an interval timer, and install the timer_handler callback */
void initITimer(uint64_t timeout)
{
  struct sigaction sa;
  struct itimerval timer;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &timer_handler;
  sigaction(SIGALRM, &sa, NULL);

  timer.it_value.tv_sec = timeout / 1000000;
  timer.it_value.tv_usec = timeout % 1000000;
  timer.it_interval = timer.it_value;

  setitimer(ITIMER_REAL, &timer, NULL);
}

/* ======================================================= */
/* SECTION: Aux function                                   */
/* ------------------------------------------------------- */
/* misc aux functions */

int failure(int fatal, const char *message, ...)
{
  va_list argp;
  char buffer[1024];

  if (!fatal) //  && wiringPiReturnCodes)
    return -1;

  va_start(argp, message);
  vsnprintf(buffer, 1023, message, argp);
  va_end(argp);

  fprintf(stderr, "%s", buffer);
  exit(EXIT_FAILURE);

  return 0;
}

/*
 * waitForEnter:
 *********************************************************************************
 */

void waitForEnter(void)
{
  printf("Press ENTER to continue: ");
  (void)fgetc(stdin);
}

/*
 * delay:
 *	Wait for some number of milliseconds
 *********************************************************************************
 */

void delay(unsigned int howLong)
{
  struct timespec sleeper, dummy;

  sleeper.tv_sec = (time_t)(howLong / 1000);
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;

  nanosleep(&sleeper, &dummy);
}

/* From wiringPi code; comment by Gordon Henderson
 * delayMicroseconds:
 *	This is somewhat intersting. It seems that on the Pi, a single call
 *	to nanosleep takes some 80 to 130 microseconds anyway, so while
 *	obeying the standards (may take longer), it's not always what we
 *	want!
 *
 *	So what I'll do now is if the delay is less than 100uS we'll do it
 *	in a hard loop, watching a built-in counter on the ARM chip. This is
 *	somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *	in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *	wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicroseconds(unsigned int howLong)
{
  struct timespec sleeper;
  unsigned int uSecs = howLong % 1000000;
  unsigned int wSecs = howLong / 1000000;

  /**/ if (howLong == 0)
    return;
#if 0
  else if (howLong  < 100)
    delayMicrosecondsHard (howLong) ;
#endif
  else
  {
    sleeper.tv_sec = wSecs;
    sleeper.tv_nsec = (long)(uSecs * 1000L);
    nanosleep(&sleeper, NULL);
  }
}

/* ======================================================= */
/* SECTION: LCD functions                                  */
/* ------------------------------------------------------- */
/* medium-level interface functions (all in C) */

/* from wiringPi:
 * strobe:
 *	Toggle the strobe (Really the "E") pin to the device.
 *	According to the docs, data is latched on the falling edge.
 *********************************************************************************
 */

void strobe(const struct lcdDataStruct *lcd)
{
  // Note timing changes for new version of delayMicroseconds ()
  digitalWrite(gpio, lcd->strbPin, 1);
  delayMicroseconds(50);
  digitalWrite(gpio, lcd->strbPin, 0);
  delayMicroseconds(50);
}

/*
 * sentDataCmd:
 *	Send an data or command byte to the display.
 *********************************************************************************
 */

void sendDataCmd(const struct lcdDataStruct *lcd, unsigned char data)
{
  register unsigned char myData = data;
  unsigned char i, d4;

  if (lcd->bits == 4)
  {
    d4 = (myData >> 4) & 0x0F;
    for (i = 0; i < 4; ++i)
    {
      digitalWrite(gpio, lcd->dataPins[i], (d4 & 1));
      d4 >>= 1;
    }
    strobe(lcd);

    d4 = myData & 0x0F;
    for (i = 0; i < 4; ++i)
    {
      digitalWrite(gpio, lcd->dataPins[i], (d4 & 1));
      d4 >>= 1;
    }
  }
  else
  {
    for (i = 0; i < 8; ++i)
    {
      digitalWrite(gpio, lcd->dataPins[i], (myData & 1));
      myData >>= 1;
    }
  }
  strobe(lcd);
}

/*
 * lcdPutCommand:
 *	Send a command byte to the display
 *********************************************************************************
 */

void lcdPutCommand(const struct lcdDataStruct *lcd, unsigned char command)
{
#ifdef DEBUG
  // fprintf(stderr, "lcdPutCommand: digitalWrite(%d,%d) and sendDataCmd(%d,%d)\n", lcd->rsPin, 0, lcd, command);
#endif
  digitalWrite(gpio, lcd->rsPin, 0);
  sendDataCmd(lcd, command);
  delay(2);
}

void lcdPut4Command(const struct lcdDataStruct *lcd, unsigned char command)
{
  register unsigned char myCommand = command;
  register unsigned char i;

  digitalWrite(gpio, lcd->rsPin, 0);

  for (i = 0; i < 4; ++i)
  {
    digitalWrite(gpio, lcd->dataPins[i], (myCommand & 1));
    myCommand >>= 1;
  }
  strobe(lcd);
}

/*
 * lcdHome: lcdClear:
 *	Home the cursor or clear the screen.
 *********************************************************************************
 */

void lcdHome(struct lcdDataStruct *lcd)
{
#ifdef DEBUG
  // fprintf(stderr, "lcdHome: lcdPutCommand(%d,%d)\n", lcd, LCD_HOME);
#endif
  lcdPutCommand(lcd, LCD_HOME);
  lcd->cx = lcd->cy = 0;
  delay(5);
}

void lcdClear(struct lcdDataStruct *lcd)
{
#ifdef DEBUG
  // fprintf(stderr, "lcdClear: lcdPutCommand(%d,%d) and lcdPutCommand(%d,%d)\n", lcd, LCD_CLEAR, lcd, LCD_HOME);
#endif
  lcdPutCommand(lcd, LCD_CLEAR);
  lcdPutCommand(lcd, LCD_HOME);
  lcd->cx = lcd->cy = 0;
  delay(5);
}

/*
 * lcdPosition:
 *	Update the position of the cursor on the display.
 *	Ignore invalid locations.
 *********************************************************************************
 */

void lcdPosition(struct lcdDataStruct *lcd, int x, int y)
{
  // struct lcdDataStruct *lcd = lcds [fd] ;

  if ((x > lcd->cols) || (x < 0))
    return;
  if ((y > lcd->rows) || (y < 0))
    return;

  lcdPutCommand(lcd, x + (LCD_DGRAM | (y > 0 ? 0x40 : 0x00) /* rowOff [y] */));

  lcd->cx = x;
  lcd->cy = y;
}

/*
 * lcdDisplay: lcdCursor: lcdCursorBlink:
 *	Turn the display, cursor, cursor blinking on/off
 *********************************************************************************
 */

void lcdDisplay(struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |= LCD_DISPLAY_CTRL;
  else
    lcdControl &= ~LCD_DISPLAY_CTRL;

  lcdPutCommand(lcd, LCD_CTRL | lcdControl);
}

void lcdCursor(struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |= LCD_CURSOR_CTRL;
  else
    lcdControl &= ~LCD_CURSOR_CTRL;

  lcdPutCommand(lcd, LCD_CTRL | lcdControl);
}

void lcdCursorBlink(struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |= LCD_BLINK_CTRL;
  else
    lcdControl &= ~LCD_BLINK_CTRL;

  lcdPutCommand(lcd, LCD_CTRL | lcdControl);
}

/*
 * lcdPutchar:
 *	Send a data byte to be displayed on the display. We implement a very
 *	simple terminal here - with line wrapping, but no scrolling. Yet.
 *********************************************************************************
 */

void lcdPutchar(struct lcdDataStruct *lcd, unsigned char data)
{
  digitalWrite(gpio, lcd->rsPin, 1);
  sendDataCmd(lcd, data);

  if (++lcd->cx == lcd->cols)
  {
    lcd->cx = 0;
    if (++lcd->cy == lcd->rows)
      lcd->cy = 0;

    // TODO: inline computation of address and eliminate rowOff
    lcdPutCommand(lcd, lcd->cx + (LCD_DGRAM | (lcd->cy > 0 ? 0x40 : 0x00) /* rowOff [lcd->cy] */));
  }
}

/*
 * lcdPuts:
 *	Send a string to be displayed on the display
 *********************************************************************************
 */

void lcdPuts(struct lcdDataStruct *lcd, const char *string)
{
  while (*string)
    lcdPutchar(lcd, *string++);
}

/* ======================================================= */
/* SECTION: aux functions for game logic                   */
/* ------------------------------------------------------- */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* --------------------------------------------------------------------------- */
/* interface on top of the low-level pin I/O code */

/* blink the led on pin @led@, @c@ times */
void blinkN(uint32_t *gpio, int led, int c)
{
  // fprintf(stdout, "Blinking led %d, %d times\n", led, c);
  for (int i = 0; i < c * 2; i++)
  {
    int off = i % 2 == 0 ? 7 : 10; // switch from on/off, GPSET0/GPCLR0
    *(gpio + off) = 1 << (led & 31);
    // Delay for one and a half seconds
    delay(1500);
  }
}

/* Print message on lcd (scrolling from right to left if longer than size of the lcd)*/
void printMessageLcd(const char *message, struct lcdDataStruct *lcd, int row)
{
  int len = strlen(message);
  int width = lcd->cols;
  char displayStr[width + 1];

  // Use lcdPuts if message fits without need to scroll
  if (len <= width)
  {
    lcdPosition(lcd, 0, row);
    lcdPuts(lcd, message);
  }
  else
  {
    // Display the string on display and shift it to the left at every iteration
    for (int i = 0; i < len - width + 1; i++)
    {
      strncpy(displayStr, message + i, width);
      displayStr[width] = '\0';

      lcdPosition(lcd, 0, row);
      lcdPuts(lcd, displayStr);
      delay(200); // Increase delay for slower scroll
    }
  }
}

/* ======================================================= */
/* SECTION: main fct                                       */
/* ------------------------------------------------------- */

int main(int argc, char *argv[])
{ 
  struct lcdDataStruct *lcd;
  int bits, rows, cols;
  unsigned char func;

  int found = 0, attempts = 0, i, j, code;
  int c, d, buttonPressed, rel, foo;
  int *attSeq;

  int pinLED = LED, pin2LED2 = LED2, pinButton = BUTTON;
  int fSel, shift, pin, clrOff, setOff, off, res;
  int fd;

  int exact, contained;
  char str1[32];
  char str2[32];

  struct timeval t1, t2;
  int t;

  char buf[32];

  // variables for command-line processing
  char str_in[20], str[20] = "some text";
  int verbose = 0, debug = 0, help = 0, opt_m = 0, opt_n = 0, opt_s = 0, unit_test = 0, res_matches = 0;

  // -------------------------------------------------------
  // process command-line arguments

  // see: man 3 getopt for docu and an example of command line parsing
  { // see the CW spec for the intended meaning of these options
    int opt;
    while ((opt = getopt(argc, argv, "hvdus:")) != -1)
    {
      switch (opt)
      {
      case 'v':
        verbose = 1;
        break;
      case 'h':
        help = 1;
        break;
      case 'd':
        debug = 1;
        break;
      case 'u':
        unit_test = 1;
        break;
      case 's':
        opt_s = atoi(optarg);
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
        exit(EXIT_FAILURE);
      }
    }
  }

  if (help)
  {
    fprintf(stderr, "MasterMind program, running on a Raspberry Pi, with connected LED, button and LCD display\n");
    fprintf(stderr, "Use the button for input of numbers. The LCD display will show the matches with the secret sequence.\n");
    fprintf(stderr, "For full specification of the program see: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf\n");
    fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  if (unit_test && optind >= argc - 1)
  {
    fprintf(stderr, "Expected 2 arguments after option -u\n");
    exit(EXIT_FAILURE);
  }

  if (verbose && unit_test)
  {
    printf("1st argument = %s\n", argv[optind]);
    printf("2nd argument = %s\n", argv[optind + 1]);
  }

  if (verbose)
  {
    fprintf(stdout, "Settings for running the program\n");
    fprintf(stdout, "Verbose is %s\n", (verbose ? "ON" : "OFF"));
    fprintf(stdout, "Debug is %s\n", (debug ? "ON" : "OFF"));
    fprintf(stdout, "Unittest is %s\n", (unit_test ? "ON" : "OFF"));
    if (opt_s)
      fprintf(stdout, "Secret sequence set to %d\n", opt_s);
  }

  seq1 = (int *)malloc(seqlen * sizeof(int));
  seq2 = (int *)malloc(seqlen * sizeof(int));
  cpy1 = (int *)malloc(seqlen * sizeof(int));
  cpy2 = (int *)malloc(seqlen * sizeof(int));

  // check for -u option, and if so run a unit test on the matching function
  if (unit_test && argc > optind + 1)
  { // more arguments to process; only needed with -u
    strcpy(str_in, argv[optind]);
    opt_m = atoi(str_in);
    strcpy(str_in, argv[optind + 1]);
    opt_n = atoi(str_in);
    // CALL a test-matches function; see testm.c for an example implementation
    readSeq(seq1, opt_m); // turn the integer number into a sequence of numbers
    readSeq(seq2, opt_n); // turn the integer number into a sequence of numbers
    if (verbose)
      fprintf(stdout, "Testing matches function with sequences %d and %d\n", opt_m, opt_n);
    res_matches = countMatches(seq1, seq2);
    showMatches(seq1, seq2);
    exit(EXIT_SUCCESS);
  }
  else
  {
    /* nothing to do here; just continue with the rest of the main fct */
  }

  if (opt_s)
  { // if -s option is given, use the sequence as secret sequence
    if (theSeq == NULL)
      theSeq = (int *)malloc(seqlen * sizeof(int));
    readSeq(theSeq, opt_s);
    if (verbose)
    {
      fprintf(stderr, "Running program with secret sequence:\n");
      showSeq(theSeq);
    }
  }

  // -------------------------------------------------------
  // LCD constants, hard-coded: 16x2 display, using a 4-bit connection
  bits = 4;
  cols = 16;
  rows = 2;
  // -------------------------------------------------------

  printf("Raspberry Pi LCD driver, for a %dx%d display (%d-bit wiring) \n", cols, rows, bits);

  if (geteuid() != 0)
    fprintf(stderr, "setup: Must be root. (Did you forget sudo?)\n");

  // init of guess sequence, and copies (for use in countMatches)
  attSeq = (int *)malloc(seqlen * sizeof(int));
  cpy1 = (int *)malloc(seqlen * sizeof(int));
  cpy2 = (int *)malloc(seqlen * sizeof(int));

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000;

  // -----------------------------------------------------------------------------
  // memory mapping
  // Open the master /dev/memory device

  if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
    return failure(FALSE, "setup: Unable to open /dev/mem: %s\n", strerror(errno));

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpiobase);
  if ((int32_t)gpio == -1)
    return failure(FALSE, "setup: mmap (GPIO) failed: %s\n", strerror(errno));

  // -------------------------------------------------------
  // Configuration of LED and BUTTON

  // Button
  pinMode(gpio, pinButton, 0);

  // Green LED
  pinMode(gpio, pinLED, 0);
  writeLED(gpio, pinLED, LOW);

  // Red LED
  pinMode(gpio, pin2LED2, 0);
  writeLED(gpio, pin2LED2, LOW);

  // -------------------------------------------------------
  // INLINED version of lcdInit (can only deal with one LCD attached to the RPi):
  // you can use this code as-is, but you need to implement digitalWrite() and
  // pinMode() which are called from this code
  // Create a new LCD:
  lcd = (struct lcdDataStruct *)malloc(sizeof(struct lcdDataStruct));
  if (lcd == NULL)
    return -1;

  // hard-wired GPIO pins
  lcd->rsPin = RS_PIN;
  lcd->strbPin = STRB_PIN;
  lcd->bits = 4;
  lcd->rows = rows; // # of rows on the display
  lcd->cols = cols; // # of cols on the display
  lcd->cx = 0;      // x-pos of cursor
  lcd->cy = 0;      // y-pos of curosr

  lcd->dataPins[0] = DATA0_PIN;
  lcd->dataPins[1] = DATA1_PIN;
  lcd->dataPins[2] = DATA2_PIN;
  lcd->dataPins[3] = DATA3_PIN;
  // lcd->dataPins [4] = d4 ;
  // lcd->dataPins [5] = d5 ;
  // lcd->dataPins [6] = d6 ;
  // lcd->dataPins [7] = d7 ;

  // lcds [lcdFd] = lcd ;

  digitalWrite(gpio, lcd->rsPin, 0);
  pinMode(gpio, lcd->rsPin, OUTPUT);
  digitalWrite(gpio, lcd->strbPin, 0);
  pinMode(gpio, lcd->strbPin, OUTPUT);

  for (i = 0; i < bits; ++i)
  {
    digitalWrite(gpio, lcd->dataPins[i], 0);
    pinMode(gpio, lcd->dataPins[i], OUTPUT);
  }
  delay(35); // mS

  // Gordon Henderson's explanation of this part of the init code (from wiringPi):
  // 4-bit mode?
  //	OK. This is a PIG and it's not at all obvious from the documentation I had,
  //	so I guess some others have worked through either with better documentation
  //	or more trial and error... Anyway here goes:
  //
  //	It seems that the controller needs to see the FUNC command at least 3 times
  //	consecutively - in 8-bit mode. If you're only using 8-bit mode, then it appears
  //	that you can get away with one func-set, however I'd not rely on it...
  //
  //	So to set 4-bit mode, you need to send the commands one nibble at a time,
  //	the same three times, but send the command to set it into 8-bit mode those
  //	three times, then send a final 4th command to set it into 4-bit mode, and only
  //	then can you flip the switch for the rest of the library to work in 4-bit
  //	mode which sends the commands as 2 x 4-bit values.

  if (bits == 4)
  {
    func = LCD_FUNC | LCD_FUNC_DL; // Set 8-bit mode 3 times
    lcdPut4Command(lcd, func >> 4);
    delay(35);
    lcdPut4Command(lcd, func >> 4);
    delay(35);
    lcdPut4Command(lcd, func >> 4);
    delay(35);
    func = LCD_FUNC; // 4th set: 4-bit mode
    lcdPut4Command(lcd, func >> 4);
    delay(35);
    lcd->bits = 4;
  }
  else
  {
    failure(TRUE, "setup: only 4-bit connection supported\n");
    func = LCD_FUNC | LCD_FUNC_DL;
    lcdPutCommand(lcd, func);
    delay(35);
    lcdPutCommand(lcd, func);
    delay(35);
    lcdPutCommand(lcd, func);
    delay(35);
  }

  if (lcd->rows > 1)
  {
    func |= LCD_FUNC_N;
    lcdPutCommand(lcd, func);
    delay(35);
  }

  // Rest of the initialisation sequence
  lcdDisplay(lcd, TRUE);
  lcdCursor(lcd, FALSE);
  lcdCursorBlink(lcd, FALSE);
  lcdClear(lcd);

  lcdPutCommand(lcd, LCD_ENTRY | LCD_ENTRY_ID);     // set entry mode to increment address counter after write
  lcdPutCommand(lcd, LCD_CDSHIFT | LCD_CDSHIFT_RL); // set display shift to right-to-left

  // END lcdInit ------
  // -----------------------------------------------------------------------------
  // Start of game
  fprintf(stderr, "Printing welcome message on the LCD display ...\n");
  char welcome[1000];
  sprintf(welcome, "   Welcome to Master Mind. The sequence is %d guesses, and there are %d colors. Press enter once you are ready to start. Enjoy!", seqlen, colors);
  printMessageLcd(welcome, lcd, 0);

  /* initialise the secret sequence */
  if (!opt_s)
    initSeq();
  if (debug)
    showSeq(theSeq);

  waitForEnter();
  lcdClear(lcd);

  // Matches array
  int *matches = (int *)malloc(2 * sizeof(int));

  // -----------------------------------------------------------------------------
  // +++++ main loop. Each iteration is a round of the game
  //       Maximum of 10 rounds before game finished
  while (!found)
  {
    attempts++;

    /*
     * USER GUESSES INPUT
     */

    for (i = 0; i < seqlen; i++)
    {
      // Check if user is choosing available colors
      do
      {
        printMessageLcd("Go!", lcd, 0);
        // Initialize 3s timer, which will interupt the wait for presses
        initITimer(TIMEOUT);
        buttonPressed = waitForButton(gpio, pinButton);

        if (buttonPressed > colors || buttonPressed < 1)
        {
          char str[100];
          sprintf(str, "Please input a number from 1 to %d\n", colors);
          lcdClear(lcd);
          fprintf(stdout, str);
          printMessageLcd(str, lcd, 0);
          lcdClear(lcd);
        }
        else
          break;

        // repeat until user inputs appropiate value
      } while (buttonPressed > colors || buttonPressed < 1);

      lcdClear(lcd);

      // Save user input into attempted sequence
      attSeq[i] = buttonPressed;

      // Blink red LED once
      blinkN(gpio, pin2LED2, 1);

      // Display guessed color
      char guessed[20];
      sprintf(guessed, "Guess: %s", full_color_names[buttonPressed - 1]);
      printMessageLcd(guessed, lcd, 0);

      // Repeat the input number by blinking green LED
      blinkN(gpio, pinLED, buttonPressed);

      lcdClear(lcd);
    }

    char guessesm[100];
    sprintf(guessesm, "Guess%d:   %s %s %s", attempts, color_names[attSeq[0] - 1], color_names[attSeq[1] - 1], color_names[attSeq[2] - 1]);

    // Print guesses on LCD
    printMessageLcd(guessesm, lcd, 0);

    // Show guesses if in debug mode
    if (debug)
    {
      fprintf(stdout, guessesm);
    }

    // Indicate end of sequence input
    blinkN(gpio, pin2LED2, 2);


    /*
     * SEQUENCE ANSWER OUTPUT
     */

    // Find the number of exact and approximate macthes
    matches = countMatches(theSeq, attSeq);

    // Print answer on LCD
    lcdClear(lcd);
    char e[20];
    char a[20];
    sprintf(e, "%d exact", matches[0]);
    sprintf(a, "%d approximate", matches[1]);
    printMessageLcd(e, lcd, 0);
    printMessageLcd(a, lcd, 1);

    // Show answers if in debug mode
    if (debug)
    {
      fprintf(stdout, "\nAnswer%d:          %d%d\n", attempts, matches[0], matches[1]);
    }

    // Show exact matches
    blinkN(gpio, pinLED, matches[0]);

    // Separator
    blinkN(gpio, pin2LED2, 1);

    // Show approximate matches
    blinkN(gpio, pinLED, matches[1]);

    lcdClear(lcd);

    // Start of new round
    printMessageLcd("Round finished", lcd, 0);
    blinkN(gpio, pin2LED2, 3);

    lcdClear(lcd);

    /*
     * END OF ROUND HANDLING
     */

    // User found secret sequence
    if (matches[0] == seqlen)
      found = 1;

    // Exit game after 10 attempts
    if (attempts >= 10)
      break;
  }
  if (found)
  {

    /*
     * SUCESS
     */

    // Display successful message and number of attempts on LCD
    char attemptsm[15];
    sprintf(attemptsm, "Attempts: %d", attempts);

    fprintf(stdout, "Sequence found\n");
    printMessageLcd("SUCCESS!!", lcd, 0);
    printMessageLcd(attemptsm, lcd, 1);

    // Turn red LED on
    writeLED(gpio, pin2LED2, HIGH);

    // Blink green LED 3 times
    blinkN(gpio, pinLED, 3);

    // Turn red LED off
    writeLED(gpio, pin2LED2, LOW);
  }
  else
  {
    /*
     * FAILED
     */
    fprintf(stdout, "Sequence not found\n");
  }

  free(matches);
  free(theSeq);
  free(lcd);
  free(seq1);
  free(seq2);
  free(cpy1);
  free(cpy2);
  free(attSeq);

  return 0;
}
