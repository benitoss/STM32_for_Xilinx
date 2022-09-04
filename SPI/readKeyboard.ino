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

#define TIME_MS_FIRST_PRESS 800
#define TIME_MS_OTHER_PRESS 150

/**
   returns an event generated by the keyboard.
   if nothing is pressed, no event (0) is return;
   is something (recognizable by the core) is pressed, EVENT_KEY_PRESS is returned and key/cmd arguments are updated

*/
int readKeyboard(unsigned char *key, unsigned char *cmd) {
  int keyboard_data;
  int event = 0;
  static int last_keyboard_data = 255;
  static int state = 0;
  static uint32 timing;
  /*
     states:
     0 => nothing pressed, waiting
     1 => something pressed for less than 1sec (should generate onKeyPress event only once)
     2 => something pressed for more than 1sec (should generate onKeyPress event once per 0.2sec)
  */

  SPI.setModule( SPI_FPGA );
  SPI_DESELECTED(); // ensure SS stays high for now

  SPI_SELECTED();
  keyboard_data = SPI.transfer(0x10); //command to read from ps2 keyboad

  keyboard_data = SPI.transfer(0);    //dummy data, just to read the response

  *cmd = (keyboard_data & 0xe0) >> 5;  //just the 3 msb are the command
  *key = (keyboard_data & 0x1f);       //just the lower 5 bits are keys

  if ( state == 0 && keyboard_data != 255 ) { // 255 = nothing is pressed
    // generate event onkeypress, update state, update timing
    state = 1;
    timing = millis();
    event = EVENT_KEYPRESS;
  } else if ( state == 1 && last_keyboard_data == keyboard_data ) {
    // if user keeps pressing the same key, wait until 1 sec, then generate event keypress and go to state 2
    if ( (millis() - timing) >= TIME_MS_FIRST_PRESS ) {
      state = 2;
      timing = millis();
      event = EVENT_KEYPRESS;
    }
  } else if ( state == 1 && keyboard_data == 255 ) {
    // TODO: could generate a keyUp event if we needed one
    // if user released the key, returns to state 0
    state = 0;
  } else if ( state == 1 && keyboard_data != last_keyboard_data ) {
    // if user with high dexterity managed to change the key without going through state 0, generate new onkeypress, and update timing
    timing = millis();
    event = EVENT_KEYPRESS;
  } else if ( state == 2 && last_keyboard_data == keyboard_data  ) {
    // user is still pressing it
    if ( (millis() - timing) >= TIME_MS_OTHER_PRESS ) {
      timing = millis();
      event = EVENT_KEYPRESS;
    }
  } else if ( state == 2 && keyboard_data == 255  ) {
    // user finally released it
    state = 0;
  } else if ( state == 2 && keyboard_data != last_keyboard_data ) {
    // if user with high dexterity managed to change the key without going through state 0, generate new onkeypress, and update timing
    state = 1;
    timing = millis();
    event = EVENT_KEYPRESS;
  }

  last_keyboard_data = keyboard_data;

  SPI_DESELECTED(); // SS high

  SPI.setModule( SPI_SD );

  if ( event == EVENT_KEYPRESS ) {
    Log.verbose("OnKeyPress: %X - cmd: %X - key: %X"CR, keyboard_data, *cmd, *key);
  }
  return event;
}

/**
   wait until a key (mapped on the core) is pressed
*/
void waitKeyPress() {
  unsigned char key, cmd;

  while ( readKeyboard(&key, &cmd) != EVENT_KEYPRESS ) {
    delay(100);
  }
}
