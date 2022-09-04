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

bool searchLetterAhead( char key, FileEntry *orderedFiles, int *currentLine, int *shown_files, int entryFilter, bool lastPass ) {
  FileEntry current;
  int searchCount = 0;

  int originalLine = (*currentLine);
  if ( originalLine < 0 ) originalLine = 0;
  strcpy( current.long_name, orderedFiles[originalLine].long_name );
  current.entry_type = orderedFiles[originalLine].entry_type;

//  Log.trace("searchLetterAhead= key: %c, filter: %d, lastPass: %T, currentLine: %d, currentType: %d, currentFileName: %c"CR, key, entryFilter, lastPass, *currentLine, current.entry_type, current.long_name[0]);

  do {
    // scan current page
    while ( (*currentLine) < ( (*shown_files) - 1 ) ) {
      (*currentLine) ++;
      searchCount++;

      // if we scanned everything and ended up in the same place
      if ( searchCount > current_dir_size ) {
        return false;
      }

      if (entryFilter == FileEntry::file && orderedFiles[ *currentLine ].entry_type == FileEntry::file && (toupper( orderedFiles[ *currentLine ].long_name[ 0 ] ) > key)){
        if (lastPass) {
          (*currentLine) = originalLine;
          return searchLetterBehind( key, orderedFiles, currentLine, shown_files, FileEntry::dir, true );          
        } else {
          (*currentLine) = -1;
          getFirstPage( orderedFiles, shown_files );
          return searchLetterAhead( key, orderedFiles, currentLine, shown_files, FileEntry::dir, true );
        }
      }

      if (!lastPass && orderedFiles[ *currentLine ].entry_type != entryFilter && orderedFiles[ *currentLine ].long_name[0] != '.') {
        (*currentLine) = -1;
        return searchLetterAhead( key, orderedFiles, currentLine, shown_files, (current.entry_type == FileEntry::file ? FileEntry::dir : FileEntry::file), true );
      }

      // if we found
      if ( toupper( orderedFiles[ *currentLine ].long_name[ 0 ] ) == key ) {
        Log.trace("Found: LastPass: %T Type: %d"CR, lastPass, entryFilter);
        return true;
      }
    }
    getNextPage( orderedFiles, shown_files );
    (*currentLine) = -1;
  } while ( true );
}

bool searchLetterBehind( char key, FileEntry *orderedFiles, int *currentLine, int *shown_files, int entryFilter, bool lastPass ) {
  char searchKey = ( key - 1 < 'A' ? (isRootDir() ? 'A' : '.') : key - 1 );
  FileEntry current;
  int searchCount = 0;
  int originalLine = (*currentLine);
  
  if ( originalLine < 0 ) originalLine = 0;
  strcpy( current.long_name, orderedFiles[originalLine].long_name );
  current.entry_type = orderedFiles[originalLine].entry_type;
  

//  Log.trace("searchLetterBehind= key: %c, filter: %d, lastPass: %T, currentLine: %d, currentType: %d, currentFileName: %c"CR, searchKey, entryFilter, lastPass, *currentLine, current.entry_type, current.long_name[0]);

  do {
    // scan current page
    while ( (*currentLine) > 0 ) {
      (*currentLine) --;
      searchCount++;

      // if we scanned everything and ended up in the same place
      if ( searchCount > current_dir_size ) {
        return false;
      }

      if (!lastPass && orderedFiles[ *currentLine ].entry_type != entryFilter && orderedFiles[ *currentLine ].long_name[0] != '.') {
        (*currentLine) = *shown_files;
        return searchLetterBehind( key, orderedFiles, currentLine, shown_files, (current.entry_type == FileEntry::file ? FileEntry::dir : FileEntry::file), true );;
      }

      // if we found
      if ( toupper( orderedFiles[ *currentLine ].long_name[0] ) <= searchKey ) {
        Log.trace("Found: LastPass: %T Type: %d"CR, lastPass, entryFilter);
        (*currentLine) = -1;
        return searchLetterAhead( key, orderedFiles, currentLine, shown_files, entryFilter, true );
      }
    }
    getPreviousPage( orderedFiles, shown_files );
    (*currentLine) = *shown_files;
  } while ( true );
}

void searchLetter( char key, FileEntry *orderedFiles, int *currentLine, int *shown_files ) {
  bool found = false;
  char currentKey = toupper( orderedFiles[ *currentLine ].long_name[0]);
  int currentEntryType = orderedFiles[ *currentLine ].entry_type;

  if ((key - currentKey) >= 0) {
    found = searchLetterAhead( key, orderedFiles, currentLine, shown_files, (currentEntryType == FileEntry::file ? FileEntry::file : FileEntry::dir), false );
  } else {
    found = searchLetterBehind( key, orderedFiles, currentLine, shown_files, (currentEntryType == FileEntry::file ? FileEntry::file : FileEntry::dir), false );
  }

  currentKey = toupper( orderedFiles[ *currentLine ].long_name[0]);
  if (!found && currentKey != key) {
    (*currentLine) = 0;
    getFirstPage(orderedFiles, shown_files);
  }
}
