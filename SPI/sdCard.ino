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

void initializeSdCard(void) {
  // select the SD Card SPI
  SPI.setModule( SPI_SD );

  if ( !sd1.begin( SD_CONFIG ) ) {

    Log.notice("SD Card initialization failed!"CR);
    initOSD();
    OsdWriteOffset( 3, "          No SD Card!!! ", 1, 0, 0, 0 );

    //hold until a SD is inserted
    while ( !sd1.begin( SD_CONFIG ) ) {
      delay(500);
    }
    removeOSD( );
  }
  changeDir();
  Log.notice("SD Card initialization done."CR);
}

void disableSD (unsigned char disable_sd) {
    Log.trace("disableSD : %d"CR, disable_sd);

    if (disable_sd)
    {
      if (!SD_disabled)
      {
        //SPI.setModule( SPI_SD );
       // SPI.end();
        pinMode( PA4, INPUT_PULLUP );
        pinMode( PA5, INPUT_PULLUP );
        pinMode( PA6, INPUT_PULLUP );
        pinMode( PA7, INPUT_PULLUP );

        SD_disabled = true;
//        SendStatusWord ();
        Log.notice("SD disabled"CR);
      }
    }
    else
    {
     if (SD_disabled)
     {
        pinMode( PA4, OUTPUT ); // SS
        pinMode( PA5, OUTPUT ); // SCK
        pinMode( PA6, INPUT_PULLUP ); // MISO
        pinMode( PA7, OUTPUT ); // MOSI

        SD_disabled = false;
   //     SendStatusWord ();
        Log.notice("SD enabled"CR);
      }
    }

    //tell the core we already handled the request
    option_sel[ 31 ] = 0xff;

}
