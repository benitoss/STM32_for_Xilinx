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

#include "pins.h"
#include <ArduinoLog.h>
#include "SdFatConfig.h"
#include <SdFat.h>
#include <SPI.h>
#include <Wire.h>
#include "OSD.h"
#include "navigateOptions.h"
#include "fileEntry.h"
#include "fileBrowser.h"
#include "sdCard.h"

#ifndef SD_FAT_VERSION_STR
#define SD_FAT_VERSION_STR "1"
#endif

#define START_PIN  PA8
#define nWAITpin  PA15

#define NSS_PIN  PB12
#define NSS_SET ( GPIOB->regs->BSRR = (1U << 12) )
#define NSS_CLEAR ( GPIOB->regs->BRR = (1U << 12) )

#define BUFFER_SIZE 512
#define CORE_NAME_SIZE 64

unsigned char request_disable_sd = 0x00;

unsigned char core_mod = 0;

char core_name[CORE_NAME_SIZE];

char sav_filename[LONG_FILENAME_LEN] = { "" };
char image_filename[LONG_FILENAME_LEN] = { "" };
char load_data[LONG_FILENAME_LEN] = { "" };
char launcher_selected[LONG_FILENAME_LEN] = { "" };

bool CORE_ok = true;

char str_conf[1024];

unsigned char ret;
unsigned long menu_action;

int defaults_bin[33];

bool img_mounted;

// globais relacionadas ao menu de opções in-core (F12)
// opcao selecionada (para cada item do menu in-core, qual opção foi selecionada (no caso de um menu de múltiplas opções ))
unsigned char option_sel[MAX_OPTIONS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned char key;
unsigned char cmd;

// only the 5 lower bit are keys
unsigned char last_key = 31;
unsigned char last_cmd = 1;

unsigned char last_ret;
unsigned char cur_select = 0;

int next_cycle_millis = 0;

FileEntry file_selected;

void noticeVersion() {
#ifndef DISABLE_LOGGING
  Log.notice( "SPI version: %c.%c%c"CR, version[0], version[1], version[2] );
  Log.notice( "SdFat version: %s"CR, SD_FAT_VERSION_STR );
  Log.notice( "SdFat support: %s"CR, SDFAT_FILE_TYPE_STR );
#endif
}

void processingTime(char* processName, unsigned int startTime) {
  unsigned int duration = 0;
  duration = millis() - startTime;
  Log.notice("'%s'> Processing time: %l ms"CR, processName, duration );
}

void resetLoadingStatusNextCycle() {
  next_cycle_millis = millis() + 500;
}
void resetMenuActionNextCycle(){
  menu_action = millis() + 500;
}

void SendStatusWord(void) {
  //generate the Status Word
  long statusWord = 0;
  Log.verbose( CR"> SendStatusWord"CR );
  for ( int j = 0; j < MAX_OPTIONS; j++ ) {

    statusWord |= ( option_sel[ j ] & 0x07 ) << ( option_sel[ j ] >> 3 );
    Log.verbose(" sel[%d]=%d - LSB %d - MSB %d - byte %d"CR, j, option_sel[j], option_sel[j] & 0x07, option_sel[j] >> 3, (option_sel[j] & 0x07 ) << ( option_sel[j] >> 3 ));
  }

  SendStatusWord(statusWord);
}

void SendStatusWord(long statusWord) {
  Log.trace("StatusWord: %l - %X - %B"CR, statusWord, statusWord, statusWord);
  
  SPI_SELECTED();

  // cmd to send the status data
  spi_osd_cmd_cont( 0x15 );
  spi32( statusWord );
  spi8( core_mod );
  spi8( (SD_disabled) ? 0x01 : 0x00 );
  SPI_DESELECTED();
}


void waitACK (void)
{
  // set second SPI on PA12-PA15
  SPI.setModule(2);

  SPI_DESELECTED();

  // syncronize and clear the SPI buffers
  // wait for command acknoledge
  while ( ret != 'K' ) {
    SPI_SELECTED();
    // clear the SPI buffers
    ret = SPI.transfer( 0 );

    // wait a little to receive the last bit
    delay(100);

    // ensure SS stays high for now
    SPI_DESELECTED();

    if ( last_ret != ret ) {
      last_ret = ret;
    }
  }

  SPI.setModule(1);
}

void getStrConf() {
  unsigned int char_count = 0;

  // let's ask if we need a specific file or need to show the menu
  // command 0x14 - get STR_CONFIG
  ret = SPI.transfer(0x14);

  while (ret != 0) {
    ret = SPI.transfer(0x00); //get one char at time
    str_conf[char_count] = ret;

    char_count++;
  }

  if (!readIni(char_count)) buildOptionsDefaults();

  if (!request_disable_sd) {
    strcat(str_conf, ";H,Set root folder...;R,Load Other Core...;");
  }

  Log.trace("CONF str_conf (%d): %s|"CR, strlen(str_conf), str_conf);
}

bool waitSlaveCommand( void ) {
  unsigned char cmd;

  // set second SPI on PA12-PA15
  SPI.setModule( SPI_FPGA );
  SPI_DESELECTED();

  //SS clear to send a comand
  SPI_SELECTED();

  //read data from slave
  ret = SPI.transfer( 0x10 );

  //wait for commands
  while ( true ) {
    cmd = SPI.transfer( 0x00 ); //Dummy data, just to get the response
    cmd = ( cmd & 0xe0 ) >> 5; //just the 3 msb are the command

    // IF PRESSED F12
    if ( cmd == 0x01 || cmd == 0x02  || cmd == 0x03 ) {
      if (menu_action > millis()) return true;
      resetMenuActionNextCycle();

      OsdClear();
      EnableOsdSPI();

      getStrConf();

      unsigned int char_count = strlen(str_conf);

      //check if we have a CONF_STR
      if ( char_count > 1 && ( cmd == 0x01 || cmd == 0x03 ) ) {
        // command 0x55 - UIO_FILE_INDEX;
        SPI.transfer(0x55);
        // index 0 - ROM
        SPI.transfer(0x00);

        // slave deselected
        SPI_DESELECTED();
        // slave selected
        SPI_SELECTED();

        if ( ((str_conf[0] == 'D' && str_conf[1] == ',') || request_disable_sd) && cmd == 0x01 )
        {
          disableSD(1);
          SendStatusWord();
          OSDVisible(false);
        }

        //initial data pump only when cmd=0x01
        else if ( str_conf[0] == 'P' && (str_conf[1] == ',' || str_conf[2] == ',') && cmd == 0x01 )
        {
          //datapump only
          int strpos;

          if (str_conf[2] == ',')
          {
               setSPIspeed(codeToNumber(str_conf[1]));
               SPI.setModule( SPI_SD );
          }
          else
          {
               setSPIspeed(codeToNumber(2));
               SPI.setModule( SPI_SD );
          }

          char *token = strtok(str_conf, ",");
          token = strtok(NULL, ",");
          token = strtok(token, ";");

          char core_name_mask[LONG_FILENAME_LEN];
          char core_name_mask_extension[5];

          removeExtension(core_name_mask, token);
          getExtension(core_name_mask_extension, token);


          if ( strcmp( load_data, "" )  != 0 )  {
            strcpy( token, load_data );

          } else if ( strcmp( core_name_mask, "CORE_NAME" )  == 0 )  {
            strcpy( token, core_name );
            strcat( token, "." );
            strcat( token, core_name_mask_extension );

          } else if ( strcmp( token, "INIT" )  == 0 )  {
            strcpy( token, "" );
          }

          changeToCorePath();
          FileEntry tokenEntry;
          SdFile tokenFile;
          tokenFile.open( token, O_READ );
          SdFileToFileEntry(&tokenFile, &tokenEntry);
          tokenFile.close();

          //index for the ROM
          dataPump(&tokenEntry, 0);

          disableSD(request_disable_sd); //disable the SD pins, if required

          //update the core with the read options
          SendStatusWord();

        } else {

          //we have an "options" menu
          //update the core with the read options
          SendStatusWord();

          // is it a new core load?
          if ( ! navigateOptions() ) {
            return false;
          }
          saveIni();
          OSDVisible(false);
        }

      } else {
        //we dont have a STR_CONF, so show the menu or CMD = 0x02
        SPI_DESELECTED();

        //show the menu to navigate
        OSDVisible( true );

        if ( fileBrowser( "", true ) ) {
          //slave deselected
          SPI_DESELECTED();
          // command 0x55 - UIO_FILE_INDEX;
          spi_osd_cmd_cont( 0x55 );
          // index as 0x01 for Sinclair QL
          SPI.transfer( 0x01 );

          //slave deselected
          SPI_DESELECTED();
          //slave selected
          SPI_SELECTED();

          dataPump(&file_selected, 0);
        }

        OSDVisible( false );
      }

      break; //ok!
    } else if ( cmd == 0x07 ) {
      break;
    }
  }

  DisableOsdSPI( );
  return true;
}

uint16_t crc16_update( uint16_t crc, uint8_t a ) {
  int i;

  crc ^= a;
  for ( i = 0; i < 8; ++i ) {
    if ( crc & 1 )
      crc = ( crc >> 1 ) ^ 0xA001;
    else
      crc = ( crc >> 1 );
  }
  return crc;
}

uint16_t crc16_CCITT( uint16_t  crc, uint8_t data ) {
  uint8_t i;

  crc = crc ^ ( ( uint16_t ) data << 8 );
  for ( i = 0; i < 8; i++ ) {
    if ( crc & 0x8000 )
      crc = ( crc << 1 ) ^ 0x1021;
    else
      crc <<= 1;
  }
  return crc;
}


uint16_t checksum_16( uint16_t  total, uint8_t data ) {
  total = total + data;
  return total;
}

void loadingBar(unsigned long current, unsigned long total) {
  if (next_cycle_millis > millis()) return;
  next_cycle_millis = millis() + 800;

  unsigned long percent = (current / 2) * 100 / (total / 2); // Divide by 2 to calculate big numbers

  char buffer_temp[6];

  sprintf(buffer_temp, " %d%% \0", percent);

  OsdWriteOffset( 3, "            Loading", 0 , 0, 0, 0 );
  OSD_progressBar( 4, buffer_temp, percent );
  Log.notice("*");
}

void savData(FileEntry *fileEntry) {
  if (request_disable_sd || img_mounted || strcmp(fileEntry->long_name, "") == 0) return;

  Log.verbose("Looking savData for: %s"CR, fileEntry->long_name);

  char extension[5] = { ".sav" };
  int load_slot = 1;

  renameExtension(sav_filename, extension, fileEntry);

  Log.trace("savData filename: %s ..."CR, sav_filename);

  mountImage(sav_filename, load_slot);
  SendStatusWord();
}

void dataPump( FileEntry *fileEntry, int transfer_index ) {
  unsigned long file_size      = 0;
  unsigned long to_read        = 0;
  unsigned long read_count     = 0;
  unsigned long loaded         = 0;
  uint16_t      crc_read       = 0;
  uint16_t      crc_write      = 0;
  int           state          = LOW;
  unsigned char c              = 0;

  char sd_buffer[BUFFER_SIZE];

  char fileSelected[LONG_FILENAME_LEN];
  char fileSelectedExt[6];
  strcpy( fileSelected, fileEntry->long_name );
  strcpy( fileSelectedExt, fileEntry->extension);

  strlwr( fileSelected );
  strlwr( fileSelectedExt );

  SdFile file;
  file.open( fileSelected, O_READ );

  if (file.isOpen()) {
    file_size = file.fileSize();
    read_count = file_size / sizeof( sd_buffer );
    if ( file_size % sizeof( sd_buffer ) != 0 ) {
      read_count++;
    }

    Log.notice(CR"data pump: '%s'"CR, fileSelected);
    Log.trace("file_ext: '%s'"CR, fileSelectedExt);
    Log.trace("file size: '%d'"CR, file_size);
    Log.trace("read_count: '%d'"CR, read_count);
    Log.trace("index: '%d'"CR, transfer_index);
  }

  SPI.setModule( SPI_FPGA );

  // ensure SS stays high for now
  SPI_DESELECTED();

  SPI_SELECTED();

  //send the transfer_index
  // command 0x55 - UIO_FILE_INDEX;
  spi_osd_cmd_cont( 0x55 );

  // index 0 - ROM, 1 = drive 1, 2 = drive 2
  spi8( transfer_index );
  SPI_DESELECTED();

  // config buffer - 16 bytes
  spi_osd_cmd_cont( 0x60 ); //cmd to fill the config buffer

  // spi functions transfer the MSB byte first
  // 4 bytes for file size
  spi32( file_size );

  //3 bytes for extension
  spi8( fileSelectedExt[ 0 ] );
  spi8( fileSelectedExt[ 1 ] );
  spi8( fileSelectedExt[ 2 ] );

  for ( int bf = 1; bf <= 6; bf++) spi8( 0xFF );

  //last bytes as STM version
  spi8( version[2] - '0' );
  spi8( version[1] - '0' );
  spi8( version[0] - '0' );

  DisableOsdSPI( );

  // select the SD Card SPI
  SPI.setModule( SPI_SD );

  if (!file.isOpen()) {
    Log.notice("Not found for pump: '%s'"CR, fileSelected);

    // select the second SPI (connected on FPGA)
    SPI.setModule( SPI_FPGA );

    //SS clear to send a comand
    SPI_SELECTED();
    ret = SPI.transfer( 0x61 ); //start the data pump to slave
    // end the pumped block
    SPI_DESELECTED();

    // select the SD Card SPI
    SPI.setModule( SPI_SD );
  } else {
    OSDVisible(true);
    OsdClear();
    unsigned int startTime = millis();
    resetLoadingStatusNextCycle();

    Log.notice("Data pump > ");

    for (int k = 1; k < read_count + 1; k++ )
    {
      to_read = ( file_size >= ( sizeof( sd_buffer ) * k ) ) ? sizeof( sd_buffer ) : file_size - loaded + 1;
      file.read( sd_buffer, to_read );

      // select the second SPI (connected on FPGA)
      SPI.setModule( SPI_FPGA );

      //SS clear to send a comand
      SPI_SELECTED();
      ret = SPI.transfer( 0x61 ); //start the data pump to slave

      SPI.write( sd_buffer, to_read );

      // end the pumped block
      SPI_DESELECTED();

      loaded += to_read;

      loadingBar(loaded, file_size);

      // select the SD Card SPI
      SPI.setModule( SPI_SD );

      state++;

      //led off
      digitalWrite( PIN_LED, state >> 6 & 1 );
    }
    Log.notice(" OK"CR);

    // Force display 100%
    next_cycle_millis = 0;
    loadingBar(file_size, file_size);
    delay(100);
    OSDVisible(false);

    processingTime("Data pump", startTime);
    Log.notice(CR);
  }

  //led off
  digitalWrite( PIN_LED, HIGH );
  file.close();

  // CLEAR THE LOADING MESSAGE
  OsdWriteOffset( 6, " ", 0 , 0, 0, 0 );
  OsdWriteOffset( 7, " ", 0 , 0, 0, 0 );

  crc_read = 0;
  crc_write = 0;

  // end the data pump
  spi_osd_cmd_cont( 0x62 );

  // SS end sequence
  SPI_DESELECTED();

  savData(fileEntry);
}

void program_FPGA() {
  unsigned long bitcount = 0;
  bool last = false;
  int n = 0;
  unsigned long file_size;
  unsigned int read_count = 0;
  unsigned char val;
  unsigned long to_read;
  unsigned long loaded = 0;
  char sd_buffer[BUFFER_SIZE];
  int value;
  char fileSelected[LONG_FILENAME_LEN];
  strcpy( fileSelected, file_selected.long_name);

  setCorePath();

  Log.notice("Programming > ");
  JTAG_PREprogram();

  SdFile file;
  file.open( fileSelected, O_READ );

  int mark_pos = 0;
  int total = file.fileSize();
  int divisor = total / 32;
  int state = LOW;

  file_size = file.fileSize();
  read_count = file_size / sizeof( sd_buffer );
  if ( file_size % sizeof( sd_buffer ) != 0 ) {
    read_count ++;
  }

  // no Interrupts()
  for ( int k = 1; k < read_count + 1; k++ ) {
    to_read = ( file_size >= ( sizeof( sd_buffer ) * k ) ) ? sizeof( sd_buffer ) : file_size - loaded + 1;
    val = file.read( sd_buffer, to_read );
    for ( int f = 0; f < to_read; f ++ ) {

      if ( bitcount % divisor == 0 ) {
        Log.notice("*");
        state = ! state;
        digitalWrite( PIN_LED, state );
      }
      bitcount++ ;

      for ( n = 0; n <= 7; n++ )
      {
        if ( ( sd_buffer[ f ] >> n ) & 0x01 ) {
          // tdipin =1
          GPIOB->regs->ODR |= 2;
        }
        else {
          // tdipin = 0
          GPIOB->regs->ODR &= ~(2);
        }

        JTAG_clock();
      }
    }
    loaded += to_read;
  }

  /* AKL (Version1.7): Dump additional 16 bytes of 0xFF at the end of the RBF file */
  GPIOB->regs->ODR |= 1;
  for ( n = 0; n < 127; n++ ) {
    GPIOB->regs->ODR |= 1;
    GPIOB->regs->ODR &= ~(1);
  }

  digitalWrite( PIN_TDI, HIGH );
  digitalWrite( PIN_TMS, HIGH );
  JTAG_clock( );

  Log.notice(" OK"CR"Programmed: %l bytes"CR, bitcount);

  file.close();

  JTAG_POSprogram();
}

void initialData( void ) {
  // config buffer - 16 bytes
  spi_osd_cmd_cont(0x60); //cmd to fill the config buffer

  for (int i = 1; i <= 13; i++) spi8(0xFF);

  //last bytes as STM version
  spi8(version[2] - '0');
  spi8(version[1] - '0');
  spi8(version[0] - '0');

  DisableOsdSPI();

  strcpy(core_name, "");
  strcpy(load_data, "");
  strcpy(image_filename, "");
}

void processTasks()
{
  unsigned long timeout = 0;

  do {
    waitACK();
    digitalWrite( PIN_LED, HIGH );
    do {
      if (processImage()) timeout = millis() + 120;

    } while (timeout > millis());

  } while( waitSlaveCommand( ) );
}

void menuLoadNewCore()
{
  initOSD();

  if ( ! fileBrowser( CORE_EXTENSIONS, true ) )  {
#ifdef TEST_NAVIGATION
    //if we receive a command, go to the command state (to make the development easier)
    OSDVisible( false );
    processTasks();
#else
    CORE_ok = false;
    return;
#endif
  }
  umountImage();

  CORE_ok = true;
}

void setSPIspeed (unsigned char speed)
{
  SPI.setModule( SPI_FPGA );
  SPI_DESELECTED();

  // SPI_CLOCK_DIV2 => 36mhz SPI
  // SPI_CLOCK_DIV4 => 18mhz SPI
  // SPI_CLOCK_DIV8 => 9mhz SPI

  if (speed == 0)
    SPI.setClockDivider( SPI_CLOCK_DIV8 ); //9mhz SPI
  else if (speed == 1)
    SPI.setClockDivider( SPI_CLOCK_DIV4 ); //18mhz SPI
  else
    SPI.setClockDivider( SPI_CLOCK_DIV2 ); //36mhz SPI

  Log.trace("SPI Speed : %d"CR, speed);
}
/**
   prepare hardware

 **/
void setup( void )
{
  //Initialize serial and wait for port to open:
  Serial.begin( 115200 );

  while ( ! Serial ) {
    // wait for serial port to connect. Needed for native USB port only
    delay( 100 );
  }
  Log.begin(LOG_LEVEL, &Serial, false);

  pinMode( PIN_nWAIT, INPUT_PULLUP );
  pinMode( PIN_LED, OUTPUT );

  // configure NSS pin
  pinMode( PIN_NSS, OUTPUT );

  // set second SPI on PA12-PA15
  SPI.setModule( SPI_FPGA );

  // ensure SS stays high for now
  SPI_DESELECTED();
  SPI.begin( );

  // SPI_CLOCK_DIV2 => 36mhz SPI
  // SPI_CLOCK_DIV4 => 18mhz SPI
  // SPI_CLOCK_DIV8 => 9mhz SPI
  SPI.setClockDivider( SPI_CLOCK_DIV2 ); //36mhz SPI

  //slave deselected
  SPI_DESELECTED();
  waitACK( );

  Log.notice("*"CR);

  noticeVersion();

  initialData( );
  initializeSdCard();

  initOSD();
  splashScreen();
  removeOSD();

  char defaultCore[LONG_FILENAME_LEN];
  strcpy( defaultCore, "core." EXTENSION );

  if ( ! sd1.exists( defaultCore ) ) {
    Log.notice( "core.%s not found"CR, EXTENSION );
    CORE_ok = false;
  } else {
    SdFile dfltCore;
    dfltCore.open( defaultCore, O_READ );
    SdFileToFileEntry(&dfltCore, &file_selected);
    dfltCore.close();
  }
}// end of setup

/**
   Main loop - manages the core menu and the application menu
**/
void loop ( void )
{
  Log.trace( "init main loop."CR );

  //if we have a core.EXTENSION, lets transfer, without a menu
  if  ( CORE_ok ) {
    // Save the core name to use later inside the core
    for ( int i = 0 ; i < sizeof( core_name ); i++ ) {
      core_name[i] = '\0';
    }

    removeExtension(core_name, file_selected.long_name);

    Log.notice("core name: '%s'"CR, core_name );

    //led off
    digitalWrite( PIN_LED, HIGH );

    // wait for the FPGA power on
    delay( 300 );
    setupJTAG( );
    if ( JTAG_scan( ) == 0 ) {
      unsigned int startTime = millis();
      program_FPGA( );
      processingTime("Program FPGA", startTime);
      Log.notice(CR);
    }
    releaseJTAG();

    Log.trace( "OK, finished"CR );

    //loop forever waiting commands
    processTasks();
  }
  else
  {
    menuLoadNewCore();
  }

}  // end of loop
