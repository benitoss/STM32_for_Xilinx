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

int current_loading_char = 0;

/**
   returns the first page of sorted results
   on current directory of sd
*/
void getFirstPage( FileEntry *orderedFiles, int *shown_files ) {
  FileEntry first;
  first.entry_type = FileEntry::dir;
  first.long_name[0] = '\0';
  first.extension[0] = '\0';
  first.hidden = false;

  setDirectorySize();

  getSortedFirst( orderedFiles, first, shown_files );
}

/**
   returns the last page of sorted results
   on current directory of sd

*/
void getLastPage( FileEntry *orderedFiles, int *shown_files ) {
  FileEntry last;
  last.entry_type = FileEntry::dir;
  last.long_name[0] = '\0';
  last.extension[0] = '\0';
  last.hidden = false;
  // add one to account for .. "directory" (which in fact doesn't appear scanning sd)
  int how_many_files = ( current_dir_size + ( current_dir_depth > 0 ? 1 : 0 )  ) % MAX_SORTED_FILES;
  how_many_files = (how_many_files == 0 ? 8 : how_many_files);
  if (current_dir_size > how_many_files) {
    getSortedLast( orderedFiles, last, shown_files, how_many_files );
  } else {
    getSortedFirst( orderedFiles, last, shown_files );
  }
}

/**
   returns the next page (from the last result of orderedFiles)
   if currently on last page, goes to the first
*/
void getNextPage( FileEntry *orderedFiles, int *shown_files ) {
  getSortedFirst( orderedFiles, orderedFiles[(*shown_files) - 1], shown_files );
  if ( *shown_files == 0 ) {
    getFirstPage( orderedFiles, shown_files );
  }
}

/**
   returns the previous page (from the first result of orderedFiles)
   if currently on first page, goes to the last (tries to be
   consistent with the last page as shown by going from first to last
   by going to the next page sucessively)
*/
void getPreviousPage( FileEntry *orderedFiles, int *shown_files ) {
  getSortedLast( orderedFiles, orderedFiles[0], shown_files, MAX_SORTED_FILES );
  if ( *shown_files == 0 ) {
    getLastPage( orderedFiles, shown_files );
  }
}

/**
   returns 1 if a <= b (directory-wise)
*/
int FileEntrySmaller( FileEntry *a, FileEntry *b ) {
  if ( a->entry_type == b->entry_type ) {
    if ( strcasecmp( a->long_name, b->long_name ) <= 0 ) {
      return 1;
    } else {
      return 0;
    }
  } else if ( a->entry_type == FileEntry::selector && b->entry_type == FileEntry::dir ) {
    return 1;
  } else if ( a->entry_type == FileEntry::dir && b->entry_type == FileEntry::file ) {
    return 1;
  } else return 0;
}

/**
   returns 1 if a >= b (directory-wise)
*/
int FileEntryGreater( FileEntry *a, FileEntry *b ) {
  if ( a->entry_type == b->entry_type ) {
    if ( strcasecmp( a->long_name, b->long_name ) >= 0 ) {
      return 1;
    } else {
      return 0;
    }
  } else if ( a->entry_type == FileEntry::selector && b->entry_type == FileEntry::dir ) {
    return 0;
  } else if ( a->entry_type == FileEntry::dir && b->entry_type == FileEntry::file ) {
    return 0;
  } else return 1;
}

void addDirNav(SdFile *dir, FileEntry *orderedFiles, int *size) {
  FileEntry a;
  if ( isRootDir(dir) && current_dir_size == 0 ) {
    strcpy( a.long_name, "<EMPTY>  " );
    strcpy( a.extension, "" );
    a.entry_type = FileEntry::none;
    addSorted( &a, orderedFiles, size );
    return;
  }
  
  if (select_dir_mode) {
    strcpy( a.long_name, " SET: " );
    strcat( a.long_name, path );
    strcat( a.long_name, " " );

    strcpy( a.extension, "" );
    a.entry_type = FileEntry::selector;
    addSorted( &a, orderedFiles, size );
  }
  if ( !isRootDir(dir) ) {
    strcpy( a.long_name, ".." );
    strcpy( a.extension, "" );
    a.entry_type = FileEntry::dir;
    addSorted( &a, orderedFiles, size );
  }
}

/**
   retrieve directory entries from sd, removing entries
   smaller than filterSmallerThan, and order up to MAX_SORTED_FILES
   into orderedFiles. the actual number of remaining files is
   stored into size.
*/
void getSortedFirst( FileEntry *orderedFiles, FileEntry filterSmallerThan, int *size ) {
  SdFile entry;
  FileEntry a;

  *size = 0;

  SdFile dir;
  if ( !dir.open(path) ) Log.error("Error opening path: %s"CR, path);

  // if is not root and we have to show directories, the first entry must be ".."

  if (filterSmallerThan.long_name[0] == '\0') addDirNav(&dir, orderedFiles, size);

  dir.rewind();

  while ( entry.openNext( &dir, O_READ ) ) {
    loadingStatus();
    SdFileToFileEntry( &entry, &a );
    entry.close();

    if ( ! fileBrowserOptions.test( &a ) ) {
      continue;
    }
    if ( filterSmallerThan.long_name[0] != '\0' && FileEntrySmaller( &a, &filterSmallerThan ) ) {
      continue;
    }
    if ( (*size) < MAX_SORTED_FILES || FileEntrySmaller( &a, &orderedFiles[ ( *size ) - 1 ] ) ) {
      addSorted( &a, orderedFiles, size );
    }
  }
  dir.close();
}


/**
   retrieve directory entries from sd, removing entries
   greater than filterSmallerThan, and order up to how_many_entries
   into orderedFiles. the actual number of displayed files
   is stored into size
*/
void getSortedLast( FileEntry *orderedFiles,  FileEntry filterGreaterThan, int *size, int how_many_entries ) {
  SdFile entry;
  FileEntry a;

  *size = 0;

  SdFile dir;

  if ( !dir.open(path) ) Log.error("Error opening path: %s"CR, path);

  dir.rewind();

  while ( entry.openNext( &dir, O_READ ) ) {
    loadingStatus();
    SdFileToFileEntry( &entry, &a );
    entry.close();

    if ( ! fileBrowserOptions.test( &a ) ) {
      continue;
    }
    if ( filterGreaterThan.long_name[0] != '\0' && FileEntryGreater( &a, &filterGreaterThan ) ) {
      continue;
    }

    if ( (*size) < how_many_entries ) {
      addSorted( &a, orderedFiles, size );
    }
    else if ( FileEntryGreater( &a, &orderedFiles[0] ) ) {
      addSortedRev( &a, orderedFiles, size );
    }
  }

  // if is not root, we are probably on the first page, and we have to show directories, the first entry must be ".."
  if ( filterGreaterThan.long_name[0] != '\0' && (*size) < MAX_SORTED_FILES && (*size) != 0 ) addDirNav(&dir, orderedFiles, size);

  dir.close();
}

/**
   add element a to orderedFile in the proper place (to be ordered)

   assumes that a > orderedFile[0] and that orderedFile[i] < orderedFile[i+1]

*/
void addSorted( FileEntry *a, FileEntry *orderedFiles, int *size ) {
  int i;
  // find where to insert the element
  for ( i = 0; i < *size; i++ ) {
    if ( FileEntrySmaller( a, &orderedFiles[i] ) ) {
      break;
    }
  }
  // grow orderedFile size if possible
  if ( *size < MAX_SORTED_FILES ) {
    (*size)++;
  }
  // open up space for the new element
  for ( int j = (*size) - 1; j > i; j-- ) {
    orderedFiles[j] = orderedFiles[j-1];
  }
  // insert the new element
  strcpy( orderedFiles[i].long_name, a->long_name );
  strcpy( orderedFiles[i].extension, a->extension );
  orderedFiles[i].entry_type = a->entry_type;
}

/**
   add element a to orderedFile in the proper place (to be ordered)

   assumes that a > orderedFile[0] and that orderedFile[i] < orderedFile[i+1]

*/
void addSortedRev( FileEntry *a, FileEntry *orderedFiles, int *size ) {
  int i;
  for ( i = 0; i < *size; i++ ) {
    if ( FileEntrySmaller( a, &orderedFiles[i] ) ) {
      break;
    }
  }
  i--;
  for ( int j = 0; j < i; j ++ ) {
    orderedFiles[j] = orderedFiles[j+1];
  }
  strcpy( orderedFiles[i].long_name, a->long_name );
  strcpy( orderedFiles[i].extension, a->extension );
  orderedFiles[i].entry_type = a->entry_type;
}

void loadingStatus() {
  if (next_cycle_millis > millis()) return;
  next_cycle_millis = millis() + 500;

  OsdClear();

  switch(current_loading_char) {
    case 0:
      OsdWriteOffset( 0, "Loading... \\", 0, 0, 0, 0 );
      break;
    case 1:
      OsdWriteOffset( 0, "Loading... |", 0, 0, 0, 0 );
      break;
    case 2:
      OsdWriteOffset( 0, "Loading... /", 0, 0, 0, 0 );
      break;
    default:
      current_loading_char = -1;
      OsdWriteOffset( 0, "Loading... -", 0, 0, 0, 0 );
      break;
  }
  current_loading_char++;
}
