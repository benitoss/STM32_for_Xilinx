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

#define EVENT_NOTHING 0
#define EVENT_KEYPRESS 1

// the keycodes produced by the core (lower 5bits)
#define KEY_UP  30
#define KEY_DOW 29
#define KEY_LFT 27
#define KEY_RGT 23
#define KEY_RET 15
#define KEY_NOTHING 31
#define KEY_A   0
#define KEY_B   1
#define KEY_C   2
#define KEY_D   3
#define KEY_E   4
#define KEY_F   5
#define KEY_G   6
#define KEY_H   7
#define KEY_I   8
#define KEY_J   9
#define KEY_K   10
#define KEY_L   11
#define KEY_M   12
#define KEY_N   13
#define KEY_O   14
#define KEY_P   16
#define KEY_Q   17
#define KEY_R   18
#define KEY_S   19
#define KEY_T   20
#define KEY_U   21
#define KEY_V   22
#define KEY_W   24
#define KEY_X   25
#define KEY_Y   26
#define KEY_Z   28

// the maximum number of files per directory that can be sorted (in this version, the number of lines displayed)
#define MAX_SORTED_FILES 8

#define PATH_LEN 512
#define MAX_DIRECTORY_DEPTH 15

// the maximum of extensions that can be parsed (separated by "/") (context: filtering files in navigateMenu)
#define MAX_PARSED_EXTENSIONS 10

// the maximum length of extensions (including ".")
#define MAX_LENGTH_EXTENSION 5

// prototypes
void prepareExtensions( char *extension, char extensions[MAX_PARSED_EXTENSIONS][MAX_LENGTH_EXTENSION], int *totalExtensions);

// options to the file browser
struct FileBrowserOptions {
  char extensions[ MAX_PARSED_EXTENSIONS ][ MAX_LENGTH_EXTENSION ];
  int totalExtensions;

  void setOptions( char *extensions ) {
    prepareExtensions( extensions, this->extensions, &this->totalExtensions );
  }

  int getTotalExtensions() {
    return this->totalExtensions;
  }

  char* getExtension( int i ) {
    return (char *) this->extensions[ i ];
  }

  bool filterExtension( FileEntry *a ) {
    if ( this->totalExtensions == 0 || this->extensions[0][0] == '*' ) {
      return true;
    }
    for ( int i = 0; i < this->totalExtensions; i++ ) {
      if ( stricmp(a->extension, extensions[i]) == 0 || extensions[i][1] == '*' ) {
        return true;
      }
    }
    return false;
  }

  bool showExtensions() {
    return stricmp(EXTENSION, extensions[0]) != 0;
  }

  bool test( FileEntry *a ) {
    if ( a->hidden ) return false;

    if ( a->entry_type == FileEntry::dir ) {
      return true;
    }

    return filterExtension( a );
  }

} fileBrowserOptions;
