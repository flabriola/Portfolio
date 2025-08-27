/* ***************************************************************************** */
/* You can use this file to define the low-level hardware control fcts for       */
/* LED, button and LCD devices.                                                  */
/* Note that these need to be implemented in Assembler.                          */
/* You can use inline Assembler code, or use a stand-alone Assembler file.       */
/* Alternatively, you can implement all fcts directly in master-mind.c,          */
/* using inline Assembler code there.                                            */
/* The Makefile assumes you define the functions here.                           */
/* ***************************************************************************** */

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

// APP constants   ---------------------------------

// Wiring (see call to lcdInit in main, using BCM numbering)
// NB: this needs to match the wiring as defined in master-mind.c

#define STRB_PIN 24
#define RS_PIN 25
#define DATA0_PIN 23
#define DATA1_PIN 10
#define DATA2_PIN 27
#define DATA3_PIN 22

// -----------------------------------------------------------------------------
// includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

// -----------------------------------------------------------------------------
// prototypes

int failure(int fatal, const char *message, ...);

extern volatile sig_atomic_t timeOut;
extern void delay(int ms);

// -----------------------------------------------------------------------------
// Functions to implement here (or directly in master-mind.c)

/* this version needs gpio as argument, because it is in a separate file */
void digitalWrite(uint32_t *gpio, int pin, int value)
{
  asm volatile( "MOV R1, #1\n"              // construct bitmask
                "LSL R1, R1, %[Pin]\n"      // shift to the pin number given in arguments
                "CMP %[Val], #0\n"          // compare for on or off
                "BEQ set_low\n"             // if LOW, store in GPCLR
                "STR R1, [%[Base], #28]\n"  // else, store in GPSET
                "B done\n"                  
                "set_low:\n"               
                "STR R1, [%[Base], #40]\n"
                "done:\n"
                :
                : [Base] "r" (gpio), [Val] "r" (value), [Pin] "r" (pin)
                : "r1", "cc", "memory" 
  );
}

// adapted from setPinMode
void pinMode(uint32_t *gpio, int pin, int mode)
{
  int fSel = pin / 10;
  int shift = (pin % 10) * 3;
  asm volatile( "MOV r1, #4\n"
                //"UDIV r0, %[Pin], r1\n"    // find fSel and store in r0
                "MUL r0, %[Sel], r1\n"     // store fSel * 4 in r0 (gpio register)
                "MOV r2, %[Shift]\n"       // Store shift in r2
                "LDR r3, [%[Base], r0]\n"  // GPIO Register (gpio + fSel)
                "MOV r4, #0b111\n"         // 7
                "LSL r4, r2\n"             // shift the 111 using shift (r2)
                "BIC r3, r3, r4\n"         // (gpio + fSel) & ~(7 << shift)
                "MOV r4, #0b1\n"           // store 1 value
                "LSL r4, r2\n"             // shift the one 
                "ORR r3, r4\n"             // OR operation
                "STR r3, [%[Base], r0]\n"  // write bit sequence to register
                :
                : [Base] "r" (gpio), [Pin] "r" (pin), [Sel] "r" (fSel), [Shift] "r" (shift)
                : "r0", "r1", "r2", "r3", "r4", "cc", "memory"

  );

  // fprintf(stdout, "PM - fSel: %d, shift: %d, pin: %d\n", fSel, shift, pin);
}

void writeLED(uint32_t *gpio, int led, int value)
{
  asm volatile( "MOV R1, #1\n"                // construct bitmask
                "LSL R1, R1, %[Pin]\n"        // shift to the pin number given in arguments
                "CMP %[Val], #0\n"            // compare for on or off
                "BEQ clr\n"                   // if LOW, store in GPCLR
                "STR R1, [%[Base], #28]\n"    // else, store in GPSET
                "B end\n"                   
                "clr:\n"                     
                "STR R1, [%[Base], #40]\n" 
                "end:"                       
                :
                : [Base] "r" (gpio), [Val] "r" (value), [Pin] "r" (led)
                : "r1", "cc", "memory"
  );
}

int readButton(uint32_t *gpio, int button)
{
  int res;
  asm volatile ("MOV r0, #1\n"                // construct bitmask
                "LSL r0, r0, %[But]\n"
                "LDR r1, [%[Base], #52]\n"    // read value from GPLEV0 into register
                "AND r1, r1, r0\n"            // Find if register is high (1) or low (0)
                "CMP r1, #0\n"                
                "MOVEQ %[Result], #0\n"       // If low (not being pressed) return 0
                "MOVNE %[Result], #1\n"       // If high (pressed) return 1
                : [Result] "=r" (res)
                : [But] "r" (button), [Base] "r" (gpio)
                : "r0", "r1", "cc", "memory"

  );
  return res;
}

// Polls for button presses until timer expires, returning the number of button presses
int waitForButton(uint32_t *gpio, int button)
{
  int counter = 0;
  int state = LOW;
  int prevState = LOW;
  // Reset flag
  timeOut = 0;

  // fprintf(stdout, "Waiting for button press\n");

  /* Loop until timeout. At each iteration use the readButton function to check the state of the button,
   * a button press should only be identified if the button has been pressed (HIGH) and released (LOW). */
  while (!timeOut)
  {
    state = readButton(gpio, button);

    if (state != prevState)
    {
      if (state == HIGH)
      {
        counter++; // Increment counter on button press
        // fprintf(stdout, "counter incremented\n");
        delay(300); // Avoid button bounce
      }
      prevState = state;
    }
  }

  return counter;
}