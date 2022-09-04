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

int current_dir_depth = 0;
int currentKey = 0;
int current_dir_size = 0;

char path[PATH_LEN] = {'\0'};
char core_path[PATH_LEN] = {'\0'};
char core_root[PATH_LEN] = {'\0'};
char last_path[PATH_LEN] = {'\0'};
boolean select_dir_mode = false;
boolean disable_soft_root = false;

char last_dirs[MAX_DIRECTORY_DEPTH][LONG_FILENAME_LEN] = {"/"};
unsigned char keys[26] = { KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z };

char mapKeyToChar( int key ) {
  switch (key) {
    case KEY_A:    return 'A';
    case KEY_B:    return 'B';
    case KEY_C:    return 'C';
    case KEY_D:    return 'D';
    case KEY_E:    return 'E';
    case KEY_F:    return 'F';
    case KEY_G:    return 'G';
    case KEY_H:    return 'H';
    case KEY_I:    return 'I';
    case KEY_J:    return 'J';
    case KEY_K:    return 'K';
    case KEY_L:    return 'L';
    case KEY_M:    return 'M';
    case KEY_N:    return 'N';
    case KEY_O:    return 'O';
    case KEY_P:    return 'P';
    case KEY_Q:    return 'Q';
    case KEY_R:    return 'R';
    case KEY_S:    return 'S';
    case KEY_T:    return 'T';
    case KEY_U:    return 'U';
    case KEY_V:    return 'V';
    case KEY_W:    return 'W';
    case KEY_X:    return 'X';
    case KEY_Y:    return 'Y';
    case KEY_Z:    return 'Z';
      // case KEY_0:    return '0';
  }
  return '\0'; // not mapped
}

char mapCharToKey( char key ) {
  switch (key) {
    case 'A':    return KEY_A;
    case 'B':    return KEY_B;
    case 'C':    return KEY_C;
    case 'D':    return KEY_D;
    case 'E':    return KEY_E;
    case 'F':    return KEY_F;
    case 'G':    return KEY_G;
    case 'H':    return KEY_H;
    case 'I':    return KEY_I;
    case 'J':    return KEY_J;
    case 'K':    return KEY_K;
    case 'L':    return KEY_L;
    case 'M':    return KEY_M;
    case 'N':    return KEY_N;
    case 'O':    return KEY_O;
    case 'P':    return KEY_P;
    case 'Q':    return KEY_Q;
    case 'R':    return KEY_R;
    case 'S':    return KEY_S;
    case 'T':    return KEY_T;
    case 'U':    return KEY_U;
    case 'V':    return KEY_V;
    case 'W':    return KEY_W;
    case 'X':    return KEY_X;
    case 'Y':    return KEY_Y;
    case 'Z':    return KEY_Z;
      // case KEY_0:    return '0';
  }
  return '\0'; // not mapped
}
/**
   This is the core/file selection menu

   arguments:
   ----------

   char *extension => string with preferred file extension for cores (separated by "/", no ".")
   bool canAbort => allow aborting by pressing F12?
   bool showExtensions => shows the file extensions?

   return value
   ------------

   bool => true     when a core is selected (also sets the global file_selected, needed elsewhere)
        => false    when user request abort (pressed F12)
*/
bool fileBrowser( char* extension, bool canAbort ) {
  int event;
  bool changedPage = false;

  int shown_files = 0;
  int currentLine = 0; // currently selected line (0-MAX_SORTED_FILES)
  int idleTime = millis(); // how much time (ms) passed since the  last time the selected file has rolled
  int rolling_offset = 0; // how many characters has been currently rolled

  Log.trace( "fileBrowser: { extension='%s', canAbort='%T' }"CR, extension, canAbort );

  FileEntry orderedFiles[ MAX_SORTED_FILES ];

  fileBrowserOptions.setOptions( extension );

  getFirstPage( orderedFiles, &shown_files );

  notice_files( orderedFiles, shown_files, currentLine );

  while (true)
  {
    int j;

    // show files on screen
    show_files( orderedFiles, currentLine, shown_files, &rolling_offset, &idleTime );

    // read keyboard and act
    event = readKeyboard( &key, &cmd );

    if ( event == EVENT_KEYPRESS ) {
      idleTime = millis();
      rolling_offset = 0;

      resetLoadingStatusNextCycle();

      switch ( key ) {
        case KEY_UP: // up
          currentLine --;
          if ( currentLine < 0 ) {
            getPreviousPage( orderedFiles, &shown_files );
            currentLine = shown_files - 1;
            changedPage = true;
          }
          setCurrentKey(orderedFiles[ currentLine ].long_name);
          break;
        case KEY_DOW: // down
          currentLine ++;
          if ( currentLine >= shown_files ) {
            getNextPage( orderedFiles, &shown_files );
            currentLine = 0;
            changedPage = true;
          }
          setCurrentKey(orderedFiles[ currentLine ].long_name);
          break;
        case KEY_LFT: // left
          currentLine = 0;
          getPreviousPage( orderedFiles, &shown_files );
          setCurrentKey(orderedFiles[ currentLine ].long_name);
          break;
        case KEY_RGT: // right
          currentLine = 0;
          getNextPage( orderedFiles, &shown_files );
          setCurrentKey(orderedFiles[ currentLine ].long_name);
          break;
        case KEY_A:
        case KEY_B:
        case KEY_C:
        case KEY_D:
        case KEY_E:
        case KEY_F:
        case KEY_G:
        case KEY_H:
        case KEY_I:
        case KEY_J:
        case KEY_K:
        case KEY_L:
        case KEY_M:
        case KEY_N:
        case KEY_O:
        case KEY_P:
        case KEY_Q:
        case KEY_R:
        case KEY_S:
        case KEY_T:
        case KEY_U:
        case KEY_V:
        case KEY_W:
        case KEY_X:
        case KEY_Y:
        case KEY_Z:
          //case KEY_0: // TODO
          searchLetter( mapKeyToChar( key ), orderedFiles, &currentLine, &shown_files );
          setCurrentKey(orderedFiles[ currentLine ].long_name);
          processingTime("SearchLetter", idleTime);
          break;

        case KEY_RET: // enter
          currentKey = 0;
          if ( orderedFiles[ currentLine ].entry_type == FileEntry::dir ) {
            Log.trace("Changing to directory: '%s'"CR, orderedFiles[ currentLine ].long_name);
            if ( strcmp( orderedFiles[ currentLine ].long_name, ".." ) == 0 ) {
              current_dir_depth--;
              changeDir();
            } else if (current_dir_depth >= MAX_DIRECTORY_DEPTH) {
              char errorMsg[ 33 ];
              sprintf( errorMsg, ">= %d max directory depth.", MAX_DIRECTORY_DEPTH );
              errorScreen( errorMsg );
            } else {
              current_dir_depth++;
              strcpy(last_dirs[current_dir_depth], orderedFiles[ currentLine ].long_name);
              changeDir();
            }

            getFirstPage( orderedFiles, &shown_files );

            currentLine = 0;

          } else if ( orderedFiles[ currentLine ].entry_type == FileEntry::selector ) {
            setCoreRoot();
            Log.trace("Select ROOT dir: %s"CR, core_root);
            return false;

          } else if ( orderedFiles[ currentLine ].entry_type == FileEntry::file ) {
            umountImage();

            if ( stricmp(orderedFiles[ currentLine ].extension, LAUNCHER_EXTENSION) == 0 ) {
              if (!readLauncher(orderedFiles[ currentLine ].long_name)) return false;
            } else {
              file_selected = orderedFiles[ currentLine ];
              strcpy(launcher_selected, "");
              strcpy(load_data, "");
            }
            setLastPath();
            return true;
          } else {
            return false;
          }
          break;
        case KEY_NOTHING: // nothing was pressed
          break;
        default:
          Log.trace( "Unrecognized key pressed: '%d'"CR, key );
          break;
      }

      //F12 - abort
      if ( ( cmd == 0x01 || cmd == 0x02 || cmd == 0x03 ) && canAbort ) {
        return false;
      }

      // recalculate page
      if ( changedPage ) {
        changedPage = false;
        OsdClear( );
      }

      notice_files( orderedFiles, shown_files, currentLine );
      delay(120);
    }// end if events

  }// end while
}

void setCurrentKey(char* selectedFile) {
  if (selectedFile != '\0') {
    for (int k = 0; k < sizeof(keys); k++) {
      if (mapCharToKey(selectedFile[0]) == keys[k]) {
        currentKey = k;
        break;
      }
    }
  }
}

boolean isRootDir() {
  boolean isRoot;
  SdFile dir;
  dir.open(path, O_READ);
  isRoot = isRootDir(&dir);
  dir.close();
}

boolean isRootDir(SdFile *dir) {
  return !dir->isSubDir() || ( !select_dir_mode && !disable_soft_root && stricmp(path, core_root) == 0 );
}

void setLastDirs() {
  char *token;
  char wPath[PATH_LEN];
  strcpy(wPath, path);
  current_dir_depth=0;

  token = strtok( wPath, "/" );
  while ( token ) {
    current_dir_depth++;
    strcpy(last_dirs[current_dir_depth], token);
    token = strtok( NULL, "/" );
  }
}

void setDirectorySize() {
  SdFile file;
  FileEntry entry;
  int ct = 0;

  SdFile dir;
  if ( !dir.open(path) ) Log.error("Error opening path: %s"CR, path);

  dir.rewind();

  while ( file.openNext( &dir, O_READ ) ) {
    SdFileToFileEntry( &file, &entry );
    file.close( );

    loadingStatus();

    if ( fileBrowserOptions.test( &entry ) ) {
      ct ++;
    }
  }
  dir.close();
  current_dir_size = ct;
}

void changeToDir(char *directory) {
  if ( directory[0] != '\0' && sd1.exists(directory) ) {
    strcpy(path, directory);
    setLastDirs();
    changeDir();
  }
}

void changeToCoreRoot() {
  changeToDir(core_root);
}

void changeToCorePath() {
  changeToDir(core_path);
}

void changeToLastPath() {
  changeToDir(last_path);
}

void setCorePath() {
  strcpy(core_path, path);
  Log.trace("Core path: %s"CR, core_path);
}

void setCoreRoot() {
  strcpy(core_root, path);
  Log.trace("Core ROOT: %s"CR, core_root);
}

void setLastPath() {
  strcpy(last_path, path);
  Log.trace("Last path: %s"CR, last_path);
}

void changeDir() {
  strcpy(path, "/");
  for (int d=1; d <= current_dir_depth; d++) {
    strcat(path, last_dirs[d]);
    strcat(path, "/");
  }
  strcat(path, '\0');
  sd1.chdir(path);
}
