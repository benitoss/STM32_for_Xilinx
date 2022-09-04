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

void getExtension(char *extension, char *sfn){
  int idx = 0;
  int j = 0;

  for (int i = strlen(sfn); i > 0; i--) {
    if (sfn[i] == '.') {
      idx = i;
      break;
    }
  }

  if (idx > 0) {
    idx++;
    for (j = 0; j < FILE_EXTENSION_LEN; j++) {
      if (sfn[idx+j] == '\0') break;
      extension[j] = sfn[idx+j];
    }
  }

  extension[j] = '\0';
}

void removeExtension(char *fname, char *lfn){
  int i = 0;

  for ( i = 0; i < LONG_FILENAME_LEN; i++ ) {
    if (lfn[i] == '\0' || lfn[i] == '.') break;
    fname[i] = lfn[i];
  }
  fname[i] = '\0';
}

void renameExtension(char *fname, char *extension, FileEntry *fileEntry) {
  if (strcmp(fileEntry->long_name, "") == 0) {
    strcpy(fname, "");
    return;
  }
  
  removeExtension(fname, fileEntry->long_name);
  strcat(fname, extension);
}

void prepareExtensions( char *extension, char extensions[MAX_PARSED_EXTENSIONS][MAX_LENGTH_EXTENSION], int *totalExtensions)
{
  int i, j = 0, k = 0;
  for (i = 0; i < strlen(extension); i++) {
    if ( extension[i] != '/' ) {
      extensions[j][k++] = extension[i];
    } else {
      // it's a / => separate it
      extensions[j][k] = '\0';
      j++;
      k = 0;
    }
  }

  if ( i > 0 ) {
    extensions[j][k] = '\0';
    j++;
  }
  *totalExtensions = j;

  Log.trace("Total of extensions: [ ");
  for (i = 0; i < j; i++) {
    Log.trace(F("'%s'"), extensions[i]);
    if (i+1 < j) Log.trace(",");
  }
  Log.trace(" ]"CR);
}

/**
   converts data into a SdFile object to FileEntry
*/
void SdFileToFileEntry( SdFile *a, FileEntry *b ) {
  a->getName( b->long_name, LONG_FILENAME_LEN );
  b->entry_type = a->isDir() ? FileEntry::dir : FileEntry::file;
  if ( a->isDir() ) {
    b->extension[0] = '\0';
  } else {
    getExtension(b->extension, b->long_name);
  }
  b->hidden = a->isHidden() || b->long_name[0] == '.';
}
