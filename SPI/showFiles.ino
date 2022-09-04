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

// show currentPage of results (MENU_MAX_LINES)
void show_files( FileEntry *orderedFiles, int currentLine, int totalFiles, int *rolling_offset, int *idleTime ) {
  int j;

  for ( j = 0; j < MENU_MAX_LINES; j ++ ) {

    // erase lines without files
    if ( ( j ) >= totalFiles ) {
      OsdWriteOffset( j, "", 0, 0, 0, 0 );
      continue;
    }

    char temp_filename[LONG_FILENAME_LEN];
    formatFileEntry(&orderedFiles[j], temp_filename);

    // display unselected file
    if ( j != ( currentLine % MENU_MAX_LINES ) ) {
      OsdWriteOffset( j, temp_filename, 0, 0, 0, 0 );
    } else {
      // display selected file

      // do not roll files that fit the screen
      if ( strlen( temp_filename ) < SCREEN_WIDTH_IN_CHARS ) {
        *rolling_offset = 0;
      } else {


        // if rolled all the way to the end of string, stop, wait 2sec and start again
        if ( *rolling_offset + SCREEN_WIDTH_IN_CHARS > strlen( temp_filename )  ) {
          if ( ( millis( ) - *idleTime ) > 2000 ) {
            *idleTime = millis( );
            *rolling_offset = 0;
          }
        } else
          // if at start of string, wait 2sec to start rolling, then go 1 char each 0.2sec
          if ( ( *rolling_offset == 0 && (millis( ) - *idleTime ) > 2000 ) || ( *rolling_offset > 0 && (millis() - *idleTime ) > 200 ) ) {
            *idleTime = millis( );
            (*rolling_offset)++;
          }
      }
      OsdWriteOffset( j, temp_filename + ( *rolling_offset), 1, 0, 0, 0 );
    }
  }
}

void formatFileEntry(FileEntry *file, char *file_name) {
  int k;

  // skips one byte if its a dir so I can put <> around the dirname
  if ( file->entry_type == FileEntry::dir || file->entry_type == FileEntry::selector ) {
    strcpy( file_name + 1, file->long_name );
    file_name[0] = '<';
    file_name[strlen(file_name)] = '>';
    k = strlen( file->long_name ) + 2;
  } else {
    strcpy( file_name, file->long_name );
    k = strlen( file->long_name );

    // remove filename extension
    if ( !fileBrowserOptions.showExtensions() ) {
      k -= strlen( file->extension ) + 1;
    }
  }
  file_name[k] = '\0';
}

void notice_files( FileEntry *orderedFiles, int totalFiles, int currentLine ) {
#ifndef DISABLE_LOGGING
  Log.trace("----------------------------------------------------------------"CR);
  for ( int j = 0; j < totalFiles; j ++ ) {
    char file_name[LONG_FILENAME_LEN];
    formatFileEntry(&orderedFiles[j], file_name);
    if ( j == currentLine ) {
      Log.trace(">");
    } else {
      Log.trace(" ");
    }
    Log.trace("| %s"CR, file_name);
  }
#endif
}
