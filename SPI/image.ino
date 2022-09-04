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

#define IMG_FILE_SIZE 65536
#define IMG_BUFFER_SIZE 512

SdFile image_file;
unsigned long next_lba;

void createSavFile() {
  if (!request_disable_sd && !image_file.isOpen() && strcmp(sav_filename, "") != 0) {
    umountImage();

    SdFile tfile;
    Log.verbose("Creating sav file: '%s'"CR, sav_filename);
    tfile.open( sav_filename, O_WRITE | O_CREAT );
    tfile.sync();
    tfile.close();

    mountImage(sav_filename, 1);
  }
}

void loadImage(char *load_image) {
  if ( strcmp(load_image, "") != 0 ) {
    Log.verbose("Auto load image: %s"CR, load_image);
    mountImage(load_image, (findLoadSlot('I') || findLoadSlot('L')));
  }
}

void mountImage(FileEntry *fileEntry, unsigned int load_slot) {
  mountImage(fileEntry->long_name, load_slot);
}

void mountImage(char *fileName, unsigned int load_slot) {
  if (request_disable_sd) return;

  SdFile image;
  image.open( fileName, O_RDWR ); //O_RDWR  O_READ

  mountImage(&image, load_slot, (image.isOpen() && image.fileSize() > IMG_FILE_SIZE ? image.fileSize() : IMG_FILE_SIZE));
}

void mountImage(SdFile *image, unsigned int load_slot, unsigned long file_size) {
  if (img_mounted) return;
  
  image_file = *image;

  // select the SD Card SPI
  SPI.setModule( SPI_SD );

  next_lba = 9999;

  traceMountImage(image, file_size, load_slot);

  //Select the FPGA SPI
  SPI.setModule( SPI_FPGA );
  SPI_DESELECTED();
  SPI_SELECTED();

  //send the load_slot
  // command 0x55 - UIO_FILE_INDEX;
  ret = SPI.transfer( 0x55 );

  spi8( load_slot );
  SPI_DESELECTED();

  //start the transmission
  SPI_SELECTED();
  ret = SPI.transfer( 0x1d ); //image size info

  // 4 bytes for file size
  SPI.transfer(file_size >> 0);
  SPI.transfer(file_size >> 8);
  SPI.transfer(file_size >> 16);
  SPI.transfer(file_size >> 24);

  // end the tramsission block
  SPI_DESELECTED();

  //start the transmission
  SPI_SELECTED();
  ret = SPI.transfer( 0x1c ); //image mounted signal
  spi8( 0x00 ); //0x01 for a second image    TO DO
  // end the trasmission block
  SPI_DESELECTED();

  img_mounted = true;
}

void umountImage() {
  if (request_disable_sd) return;

  Log.trace(CR"--- UmountImage ---"CR);
  SPI.setModule( SPI_SD );
  image_file.close();

  img_mounted = false;
  image_loaded = false;
}

bool processImage() {
  if (!img_mounted || request_disable_sd) return false;

  unsigned char sd_cmd;
  unsigned char drive_sel;
  unsigned long lba;
  unsigned int startTime;

  SPI.setModule( SPI_FPGA );
  SPI_DESELECTED();
  SPI_SELECTED();

  //command 0x16 - reading sd card status
  ret = SPI.transfer( 0x16 );
  sd_cmd = SPI.transfer( 0x00 );
  drive_sel = SPI.transfer( 0x00 ); //logic drive selected
  lba = SPI.transfer( 0x00 ); //file size in 4 bytes
  lba = (lba << 8) | SPI.transfer( 0x00 );
  lba = (lba << 8) | SPI.transfer( 0x00 );
  lba = (lba << 8) | SPI.transfer( 0x00 );
  SPI_DESELECTED();

  if (lba == -1) {
    umountImage();
    return false;
  }

  //SD RD
  if (sd_cmd & 1) {
    char sd_buffer[IMG_BUFFER_SIZE];
    Log.trace("R");
    Log.verbose("(%x)", lba);

    SPI.setModule( SPI_SD );

    image_file.seekSet( lba * IMG_BUFFER_SIZE ); // por algum motivo o seek acima nao funciona. por enquanto fazendo seek a cada requisição

    image_file.read( sd_buffer, IMG_BUFFER_SIZE );

    SPI.setModule( SPI_FPGA );
    SPI_DESELECTED();
    SPI_SELECTED();
    ret = SPI.transfer( 0x17 );  //command 0x17 -  SD card sector read
    SPI.write( sd_buffer, IMG_BUFFER_SIZE );
    SPI_DESELECTED();

    blinkLed(lba);

    return true;
  }

  //SD WR
  if (sd_cmd & 2) {
    char sd_buffer[IMG_BUFFER_SIZE];
    createSavFile();

    Log.trace("W");
    Log.verbose("(%x)", lba);

    unsigned long iseek = lba * IMG_BUFFER_SIZE;

    SPI.setModule( SPI_FPGA );
    SPI_DESELECTED();
    SPI_SELECTED();
    ret = SPI.transfer( 0x18 );  //command 0x18 -  SD card sector write

    for (int i = 0; i < IMG_BUFFER_SIZE; i++) {
      ret = SPI.transfer(0x00); //Dummy, just to get the response
      sd_buffer[i] = ret;
    }
    SPI_DESELECTED();

    SPI.setModule( SPI_SD );

    image_file.seekSet(iseek);

    ret = image_file.write(sd_buffer, IMG_BUFFER_SIZE);

    image_file.sync();

    SPI_DESELECTED();

    blinkLed(lba);

    return true;
  }

  return false;
}

void blinkLed(int state) {
  digitalWrite( PIN_LED, state >> 3 & 1 );
}

void traceMountImage(SdFile *image_file, unsigned long file_size, unsigned int load_slot) {
#ifndef DISABLE_LOGGING
  char file_name[LONG_FILENAME_LEN];
  Log.trace(CR"=== MountImage ==="CR);
  image_file->getName( file_name, LONG_FILENAME_LEN );
  Log.trace( "Image Name: %s"CR, file_name );
  Log.trace( "Image Size: %l"CR, file_size );
  Log.trace( "Image Load Slot: %i"CR, load_slot );
  Log.trace( "=================="CR);
#endif
}
