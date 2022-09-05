//
// Multicore 2
//
// Copyright (c) 2017-2021 - Victor Trucco
//
// Additional code, debug and fixes: Diogo Patrão e Roberto Focosi
//
// All rights reserved
//
// Redistribution and use in source and synthezised forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// Redistributions in synthesized form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// Neither the name of the author nor the names of other contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// You are responsible for any legal issues arising from your use of this code.
//

#define MaxIR_ChainLength 100

int IRlen = 0;
int nDevices = 0;

struct codestr {
  unsigned char onebit: 1;
  unsigned int manuf: 11;
  unsigned int size: 9;
  unsigned char family: 7;
  unsigned char rev: 4;
};

union {
  unsigned long code = 0;
  codestr b;
} idcode;

/*
  void JTAG_clock()
  {
  //    digitalWrite(PIN_TCK, HIGH);
  //    digitalWrite(PIN_TCK, LOW);

      GPIOB->regs->ODR |= 1;
      GPIOB->regs->ODR &= ~(1);
  }
*/

void JTAG_reset() {
  int i;

  digitalWrite(PIN_TMS, HIGH);

  // go to reset state
  for (i = 0; i < 5; i++)
  {
    //JTAG_clock();
    GPIOB->regs->ODR |= 1;
    GPIOB->regs->ODR &= ~(1);
  }
}

void JTAG_EnterSelectDR() {  // From Reset State
  // go to select DR
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
}

// Added by Benitoss
void JTAG_EnterSelectIR() {  // from Reset State
  // go to select DR
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
}

void JTAG_EnterShiftIR() {    // From Select DR
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
}

void JTAG_EnterShiftDR() {
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();

  // digitalWrite(PIN_TMS, LOW); JTAG_clock(); //extra ?
}

void JTAG_ExitShift() {
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
}

void JTAG_ReadDR(int bitlength) {
  JTAG_EnterShiftDR();
  JTAG_ReadData(bitlength);
}

// note: call this function only when in shift-IR or shift-DR state
void JTAG_ReadData(int bitlength) {
  int bitofs = 0;
  unsigned long temp;

  bitlength--;
  while (bitlength--) {
    digitalWrite(PIN_TCK, HIGH);
    temp = digitalRead(PIN_TDO);


    // Serial.println(temp, HEX);

    temp = temp << bitofs ;
    idcode.code |= temp;

    digitalWrite(PIN_TCK, LOW);
    bitofs++;
  }

  digitalWrite(PIN_TMS, HIGH);
  digitalWrite(PIN_TCK, HIGH);

  temp = digitalRead(PIN_TDO);

  // Serial.println(temp, HEX);

  temp = temp << bitofs ;
  idcode.code |= temp;

  digitalWrite(PIN_TCK, LOW);

  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();  // go back to select-DR
}

int JTAG_DetermineChainLength(char* s) {
  int i;

  // empty the chain (fill it with 0's)
  digitalWrite(PIN_TDI, LOW);
  for (i = 0; i < MaxIR_ChainLength; i++) {
    digitalWrite(PIN_TMS, LOW);
    JTAG_clock();
  }

  digitalWrite(PIN_TCK, LOW);

  // feed the chain with 1's
  digitalWrite(PIN_TDI, HIGH);
  for (i = 0; i < MaxIR_ChainLength; i++) {
    digitalWrite(PIN_TCK, HIGH);

    if (digitalRead(PIN_TDO) == HIGH) break;

    digitalWrite(PIN_TCK, LOW);
  }

  digitalWrite(PIN_TCK, LOW);

  Log.verbose("%s = %d"CR, s, i);

  JTAG_ExitShift();
  return i;
}

int JTAG_scan() {
  int i = 0;

  JTAG_reset();
  JTAG_EnterSelectDR();
  JTAG_EnterShiftIR() ;

  IRlen = JTAG_DetermineChainLength("tamanho do IR");

  JTAG_EnterShiftDR();
  nDevices = JTAG_DetermineChainLength("Qtd devices");

  if (IRlen == MaxIR_ChainLength || nDevices == MaxIR_ChainLength ) {
    Log.error("JTAG ERROR!!!!"CR);
    return 1;
  }

  // read the IDCODEs (assume all devices support IDCODE, so read 32 bits per device)
  JTAG_reset();
  JTAG_EnterSelectDR();
  JTAG_ReadDR(32 * nDevices);

  Log.trace("Device IDCODE: %x"CR, idcode.code + HEX);
  Log.trace(" rev: %x"CR, idcode.b.rev);
  Log.trace(" family: %x"CR, idcode.b.family);
  Log.trace(" size: %x"CR, idcode.b.size);
  Log.trace(" manuf: %x"CR, idcode.b.manuf);
  Log.trace(" onebit: %x"CR, idcode.b.onebit);

  return 0;
}

/*
void JTAG_PREprogram() {  // For Altera
  int n;

  JTAG_reset();
  JTAG_EnterSelectDR();
  JTAG_EnterShiftIR() ;

  //  digitalWrite(PIN_TMS, LOW); JTAG_clock(); //extra ?

  // aqui o TMS ja esta baixo, nao precisa de outro comando pra abaixar.

  // IR = PROGRAM =   00 0000 0010    // IR = CONFIG_IO = 00 0000 1101
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW);
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();

  // exit IR mode

  digitalWrite(PIN_TMS, HIGH); JTAG_clock();

  // update IR mode

  // Drive TDI HIGH while moving to SHIFTDR 
  digitalWrite(PIN_TDI, HIGH);

  digitalWrite(PIN_TMS, HIGH); JTAG_clock();

  // select dr scan mode

  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();

  // shift dr mode

  //digitalWrite(PIN_TMS, LOW); JTAG_clock(); //extra ?


  // Issue MAX_JTAG_INIT_CLOCK clocks in SHIFTDR state 
  digitalWrite(PIN_TDI, HIGH);
  for (n = 0; n < 300; n++) {
    JTAG_clock();
  }

  digitalWrite(PIN_TDI, LOW);
}
*/


void JTAG_PREprogram() {  // Spartan6
   
   int n;

  //            comment                                TDI   TMS TCK
  // 1: On power-up, place a logic 1 on the TMS,
  //    and clock the TCK five times. This ensures      X     1   5
  //    starting in the TLR (Test-Logic-Reset) state.
  //
   digitalWrite(PIN_TMS, HIGH);
   for (n = 0; n < 6; n++){
     //GPIOB->regs->ODR |= (1<<10);
     JTAG_clock();
   }
    
  
  //  2: Move into the RTI state.                       X     0   1
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
   
  //  3: Move into the SELECT-IR state.                 X     1   2
  
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();JTAG_clock();
   
  //  4: Enter the SHIFT-IR state.                      X     0   2
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();JTAG_clock();
   
  //  5: Start loading the JPROGRAM instruction,     00101    0   5
  //    (0000101) LSB first:
  
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
   
  //  6: Load the MSB of the JPROGRAM instruction
  //    when exiting SHIFT-IR, as defined in the        0     1   1
  //    IEEE standard.
  //
  digitalWrite(PIN_TMS, HIGH);
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
   
  //  7: Enter the SELECT-DR state                      X     1   2
  
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();JTAG_clock();
   
  //  8: Enter the SHIFT-DR state.                      X     0   2
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();JTAG_clock();

}


/*
void JTAG_PREprogram() {  // Artix7
   
   int n;

  //            comment                                TDI   TMS TCK
  // 1: On power-up, place a logic 1 on the TMS,
  //    and clock the TCK five times. This ensures      X     1   5
  //    starting in the TLR (Test-Logic-Reset) state.
  //
   digitalWrite(PIN_TMS, HIGH);
   //GPIOB->regs->ODR |= (1<<10);
   for (n = 0; n < 6; n++){
     JTAG_clock();
   }
    
  
  //  2: Move into the RTI state.                       X     0   1
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();

   
  //  3: Move into the SELECT-IR state.                 X     1   2
  
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();JTAG_clock();
   
  //  4: Enter the SHIFT-IR state.                      X     0   2
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();JTAG_clock();
   
  //  5: Start loading the JPROGRAM instruction,     01011    0   5
  //    (001011) LSB first:
  
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  //digitalWrite(PIN_TDI, HIGH); // we dont noeed to repeat 
  JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
   
  //  6: Load the MSB of the JPROGRAM instruction
  //    when exiting SHIFT-IR, as defined in the        0     1   1
  //    IEEE standard.
  //
  digitalWrite(PIN_TMS, HIGH);
  //digitalWrite(PIN_TDI, LOW); // we dont need to repeat
  JTAG_clock();
   
  //  7: Place a logic 1 on the TMS and clock the 
  //     TCK five times. This ensures starting in the
  //     TLR (Test- Logic-Reset) state                  X     1   5
  
  //digitalWrite(PIN_TMS, HIGH); // We  dont need to repeat
  for (n = 0; n < 6; n++){
     JTAG_clock();
   }
  
  //  8: Move into the RTI state.                       X     0   10.000
  
  for (n = 0; n < 10000 ; n++){
     JTAG_clock();
   }

   //  9: Move into the SELECT-IR state.                X     1   2 
   digitalWrite(PIN_TMS, HIGH);
   JTAG_clock();JTAG_clock();

  //  10: Enter the SHIFT-IR state.                      X     0   2 
   digitalWrite(PIN_TMS, LOW);
   JTAG_clock();JTAG_clock();

  // 11: Start loading the CFG_IN instruction,       00101    0   5
  //     LSB first:
   digitalWrite(PIN_TDI, HIGH); JTAG_clock();
   digitalWrite(PIN_TDI, LOW); JTAG_clock();
   digitalWrite(PIN_TDI, HIGH); JTAG_clock();
   digitalWrite(PIN_TDI, LOW); JTAG_clock(); JTAG_clock();

   // 12: Load the MSB of CFG_IN instruction            0     1   1
   // when exiting SHIFT-IR, as defined in the
   // IEEE standard.

   // digitalWrite(PIN_TDI, LOW); 
   digitalWrite(PIN_TMS, HIGH); JTAG_clock();

   // 13: Enter the SELECT-DR state.                   X     1   2

   //digitalWrite(PIN_TMS, HIGH); // already done
   JTAG_clock();JTAG_clock();

}
*/
  
/*
void JTAG_POSprogram() { //For Altera
  int n;

  //aqui esta no exit DR

  digitalWrite(PIN_TMS, HIGH); JTAG_clock();

  // aqui esta no update DR

  digitalWrite(PIN_TMS, LOW); JTAG_clock();

  //Aqui esta no RUN/IDLE

  JTAG_EnterSelectDR();
  JTAG_EnterShiftIR();

  // aqui em shift ir


  // IR = CHECK STATUS = 00 0000 0100
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW);
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();


  //aqui esta no exit IR
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();

  //aqui esta no select dr scan

  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();

  // aqui esta no shift IR


  // IR = START = 00 0000 0011
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW); JTAG_clock();

  digitalWrite(PIN_TDI, LOW);
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();

  //aqui esta no exit IR

  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  digitalWrite(PIN_TMS, LOW); JTAG_clock();

  //aqui esta no IDLE

  //espera
  for (n = 0; n < 200; n++) {
    //JTAG_clock();
    GPIOB->regs->ODR |= 1;
    GPIOB->regs->ODR &= ~(1);
  }

  JTAG_reset();

}
*/


void JTAG_POSprogram() {   //Spartan6
  int n;
  //            comment                                TDI   TMS TCK
  //             
  // 11: Enter the UPDATE-DR state.                     X     1   1             
  
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
   
  //  12: Move into the RTI state.                      X     0   1
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  
  // 13: Enter the SELECT-IR state.                     X     1   2
  
  digitalWrite(PIN_TMS, HIGH); JTAG_clock(); JTAG_clock();
  
  // 14: Move to the SHIFT-IR state.                    X     0   2
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();JTAG_clock();
  
  // 15: Start loading the JSTART instruction 
  //     (001100) (optional). The JSTART instruction  01100   0   5
  //     initializes the startup sequence.
  
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  // digitalWrite(PIN_TDI, LOW);  //Already set
  JTAG_clock();
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  // digitalWrite(PIN_TDI, HIGH); //Already set
  JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  
  // 16: Load the last bit of the JSTART instruction.   0     1   1

  // digitalWrite(PIN_TDI, LOW);  //Already set
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  
  // 17: Move to the UPDATE-IR state.                   X     1   1
  
  // digitalWrite(PIN_TMS, HIGH);  //Already set
  JTAG_clock();
  
  // 18: Move to the RTI state and clock the
  //     startup sequence by applying a minimum         X     0   16
  //     of 16 clock cycles to the TCK.
  
  digitalWrite(PIN_TMS, LOW);
  for (n = 0; n < 16; n++) {
    //JTAG_clock();
    GPIOB->regs->ODR |= 1;
    GPIOB->regs->ODR &= ~(1);
  }

  // 19: Move to the TLR state. The device is
  // now functional.                                    X     1   3
  
   digitalWrite(PIN_TMS, HIGH);
  JTAG_clock();JTAG_clock();JTAG_clock();

  
  JTAG_reset();
  
  //digitalWrite(PIN_TMS, HIGH); //Already set
  //
  // GPIOB->regs->ODR |= (1<<10);
  // for (n = 0; n < 5; n++){
  //    JTAG_clock();
  //}

}


/*
void JTAG_POSprogram() {   //Artix7
  int n;
  //            comment                                TDI   TMS TCK
  //             
  // 17: Enter the UPDATE-DR state.                     X     1   1             
  
  // digitalWrite(PIN_TMS, HIGH); //Already set
  JTAG_clock();
   
  //  18: Move into the RTI state.                      X     0   1
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();
  
  // 19: Enter the SELECT-IR state.                     X     1   2
  
  digitalWrite(PIN_TMS, HIGH); JTAG_clock(); JTAG_clock();
  
  // 20: Move to the SHIFT-IR state.                    X     0   2
  
  digitalWrite(PIN_TMS, LOW); JTAG_clock();JTAG_clock();
  
  // 21: Start loading the JSTART instruction 
  //     (001100) (optional). The JSTART instruction  01100   0   5
  //     initializes the startup sequence.
  
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  // digitalWrite(PIN_TDI, LOW); // Already set
  JTAG_clock();
  digitalWrite(PIN_TDI, HIGH); JTAG_clock();
  //digitalWrite(PIN_TDI, HIGH); // Already set
  JTAG_clock();
  digitalWrite(PIN_TDI, LOW); JTAG_clock();
  
  // 22: Load the last bit of the JSTART instruction.   0     1   1

  // digitalWrite(PIN_TDI, LOW); // Already set
  digitalWrite(PIN_TMS, HIGH); JTAG_clock();
  
  // 23: Move to the UPDATE-IR state.                   X     1   1
  
  //digitalWrite(PIN_TMS, HIGH); Already  set
  JTAG_clock();
  
  // 24: Move to the RTI state and clock the startup
  // sequence by applying a minimum of 2000 clock       X     0   2000
  // cycles to the TCK.
  
  digitalWrite(PIN_TMS, LOW);
  for (n = 0; n < 2000; n++) {
    //JTAG_clock();
    GPIOB->regs->ODR |= 1;
    GPIOB->regs->ODR &= ~(1);
  }

  // 25: Move to the TLR state. The device is
  // now functional.                                    X     1   3

  digitalWrite(PIN_TMS, HIGH);
  JTAG_clock();JTAG_clock();JTAG_clock();
  
  JTAG_reset();
  
  //digitalWrite(PIN_TMS, HIGH); //Already set
  //
  // GPIOB->regs->ODR |= (1<<10);
  // for (n = 0; n < 5; n++){
  //    JTAG_clock();
  //}

}
*/
//   JTAG
void setupJTAG( ) {
  pinMode( PIN_TCK, OUTPUT );
  pinMode( PIN_TDO, INPUT_PULLUP );
  pinMode( PIN_TMS, OUTPUT );
  pinMode( PIN_TDI, OUTPUT );

  digitalWrite( PIN_TCK, LOW );
  digitalWrite( PIN_TMS, LOW );
  digitalWrite( PIN_TDI, LOW );
}

void releaseJTAG( ) {
  pinMode( PIN_TCK, INPUT_PULLUP );
  pinMode( PIN_TDO, INPUT_PULLUP );
  pinMode( PIN_TMS, INPUT_PULLUP );
  pinMode( PIN_TDI, INPUT_PULLUP );
}
