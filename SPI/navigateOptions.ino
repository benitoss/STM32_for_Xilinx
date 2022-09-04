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

#define VERSION_DISPLAY_LINES 2
#define VERSION_FIRST_LINE 2

boolean set_root_option = false;
char core_version[VERSION_DISPLAY_LINES][SCREEN_WIDTH_IN_CHARS];

void traceOptions(int page, int total_options, NavigateOption *navigateOption) {
#ifndef DISABLE_LOGGING
  Log.trace("**************************************************************"CR);
  int i_opt;
  i_opt = page * MENU_MAX_LINES;

  for ( int i = i_opt; i < (i_opt+MENU_MAX_LINES); i++ ) {
    unsigned int j = 0;
    if ( i >= total_options) break;

    if ( i == cur_select ) {
      Log.trace(">");
    } else {
      Log.trace(" ");
    }
    Log.trace(" [%d], bin_pos: '%i', label: '%s', load_slot: %d, n_values: %d, option_type: %d, sel_idx: %d, labels: [ ", 
      navigateOption[i].idx,
      navigateOption[i].first_opt / 8,
      navigateOption[i].label,
      navigateOption[i].load_slot,
      navigateOption[i].n_values,
      navigateOption[i].option_type,
      navigateOption[i].sel_idx);
    
    while (navigateOption[i].sel_label[j][0] != '\0' && j < SELECT_LEN && j < navigateOption[i].n_values) {
      if ( j > 0) Log.trace(",");
      Log.trace("%s", navigateOption[i].sel_label[j]);
      j++;      
    }

    Log.trace(" ], values: [ ");

    for ( int o=0; o < j; o++ ){
      if ( o > 0) Log.trace(",");
      if (navigateOption[i].sel_idx == o) {
        Log.trace("[%d]", navigateOption[i].first_opt + o);
      } else {
        Log.trace("%d", navigateOption[i].first_opt + o);
      }
    }

    Log.trace(" ]"CR);
  }
#endif
}

unsigned char codeToNumber (unsigned char code) {
  if ( code >= '0' && code <= '9' ) return code - 48;
  if ( code >= 'A' && code <= 'Z' ) return code - 55;

  return code;
}

// increment ptr until (1) the end of str_conf or (2) the next occurence of delimiter
// return the pointer to the next occurence of delimiter PLUS ONE, or NULL if end of string was reached
char* get_next_line( char delimiter, char* ptr ) {
  while ( *ptr != '\0' && *ptr != delimiter ) {
    ptr++;
  }
  if ( *ptr == '\0' ) {
    return NULL;
  } else {
    return ptr + 1;
  }
}

// get total of options on current place
int get_option_count( char* ptr ) {
  if ( *ptr == '\0' ) {
    return 0;
  }
  int options = 1;
  while ( *ptr != '\0' && *ptr != ';' ) {
    if ( *ptr == ',' ) {
      options++;
    }
    ptr++;
  }
  return options;
}

// copy src to dst until a delimiter or '\0' is found
void copy_till_delimiter( char delimiter, char* src, char* dst, bool end_string = 1 ) {
  while ( *src != '\0' && *src != delimiter ) {
    *dst = *src;
    dst ++;
    src ++;
  }
  if ( end_string ) {
    *dst = '\0';
  }
}
/**
   search and copy to dst the extensions specified by the LOAD option
   with s_index (0-9)
   returns 1 if LOAD option with specified index is found, 0 otherwise
*/
int get_extension_for_load_option( int s_index, char* dst ) {
  char* ptr = str_conf;

  dst[0] = '\0';
  while ( ptr != NULL ) {

    // only load is relevant here
    if ( ptr[ 0 ] != 'S' && ptr[ 0 ] != 'I' && ptr[ 0 ] != 'L') {
      ptr = get_next_line( ';', ptr );
      continue;
    }

    int index;

    if ( ptr[1] == ',' ) {
      index = 1;
    } else {
      index = (int)( ptr[1] - '0' );
    }

    // only load with specified index is relevant
    if ( s_index != index ) {
      Log.verbose( ">> skip - wanted %d - found %d"CR, s_index, index);
      ptr = get_next_line( ';', ptr );
      continue;
    }

    // get extensions
    ptr = get_next_line( ',', ptr );
    copy_till_delimiter( ',', ptr, dst );

    return 1;
  }
  return 0;
}

void clear_option_data( NavigateOption *navigateOption ) {
  for ( int i = 0; i < MAX_OPTIONS; i++ ) {
    unsigned int j = 0;
    while (j < SELECT_LEN) {
      strcpy(navigateOption[i].sel_label[j], '\0');
      j++;      
    }
    navigateOption[i].idx = 0;
    strcpy(navigateOption[i].label, '\0');
    navigateOption[i].load_slot = 0;
    navigateOption[i].first_opt = 0;
    navigateOption[i].n_values = 0;
    navigateOption[i].sel_idx = 0;
    navigateOption[i].sel_opt = 0;
    navigateOption[i].option_type = NavigateOption::option;
  }
  for ( int cv = 0; cv < VERSION_DISPLAY_LINES; cv ++) strcpy(core_version[cv], "");
}

void changeSelection(int incr, int option_type, int max_options) {
  if ( option_type == NavigateOption::option ) {
    int num_op_token = option_sel[ cur_select ] & 0xf8; // 11111000b
    Log.trace("( %d ): %d => ", num_op_token, option_sel[ cur_select ]);
  
    option_sel[ cur_select ] = option_sel[ cur_select ] + incr;

    if ( ( option_sel[ cur_select ] & 0x07 ) == max_options || ( option_sel[ cur_select ] & 0x07 ) == 0 ) {
      option_sel[ cur_select ] = num_op_token;
    } else if ( option_sel[ cur_select ] < num_op_token ) {
      option_sel[ cur_select ] = num_op_token + max_options - 1;
    }
    Log.trace("%d"CR, option_sel[ cur_select ]);
  }
}

int findLoadSlot( char opt ) {
  char* ptr = str_conf;

  while ( ptr != NULL ) {
    char* subptr;
    // get first token, that specifies the option type
    subptr = ptr;

    if ( subptr[0] == opt ) {
      return ( subptr[1] != ',' ? codeToNumber( subptr[1] ) : 1 );
    }

    ptr = get_next_line( ';', ptr );
  }

  return 0;
}

int parseOptions( NavigateOption *navigateOption ) {
  // parsing pointer
  char* ptr = str_conf;

  boolean unknown;
  int opt_sel = 0;
  int cur_opt = 0;
  set_root_option = false;

  while ( ptr != NULL ) {
    // parse specific options from the line
    char* subptr;
    // get first token, that specifies the option type
    subptr = ptr;
    
    unknown = false;

    if ( !request_disable_sd && (subptr[ 0 ] == 'S' || subptr[ 0 ] == 'I' || subptr[ 0 ] == 'L') ) {
//      Log.verbose("It's a LOAD option: %c"CR, subptr[1]);
      set_root_option = true;

      // set behaviour
      navigateOption[ cur_opt ].idx = opt_sel;
      navigateOption[ cur_opt ].option_type = (subptr[ 0 ] == 'S' ) ? NavigateOption::load_file : NavigateOption::mount_image;
      navigateOption[ cur_opt ].load_slot = ( subptr[ 1 ] != ',' ? codeToNumber( subptr[ 1 ] ) : 1 );
      navigateOption[ cur_opt ].first_opt = 0;

      // next token is the extensions - ignore for now
      subptr = get_next_line( ',', subptr );

      // copy the title to line_option, padded with spaces
      subptr = get_next_line( ',', subptr );
    
      memset( navigateOption[ cur_opt ].label, ' ', SCREEN_WIDTH_IN_CHARS );
      copy_till_delimiter( ';', subptr, navigateOption[ cur_opt ].label, 0 );
      navigateOption[ cur_opt ].label[ SCREEN_WIDTH_IN_CHARS-1 ] = '\0';

      // clear options
      navigateOption[ cur_opt ].n_values = 0;
      
      option_sel[ opt_sel ] = 0;
    }
    else if ( set_root_option && subptr[ 0 ] == 'H' ) {
      navigateOption[ cur_opt ].idx = opt_sel;
      navigateOption[ cur_opt ].option_type = NavigateOption::set_root;
      navigateOption[ cur_opt ].load_slot = 0;
      navigateOption[ cur_opt ].first_opt = 0;
      
      option_sel[ opt_sel ] = 0;

      // copy the title to line_option, padded with spaces
      strcpy(navigateOption[ cur_opt ].label, "Set root folder...              ");
      navigateOption[ cur_opt ].label[ SCREEN_WIDTH_IN_CHARS-1 ] = '\0';

      // clear options
      navigateOption[ cur_opt ].n_values = 0;

    } else if ( subptr[ 0 ] == 'O' ) {
      // get identifier
      int num_op_token = codeToNumber( subptr[ 1 ] );
      int firstOpt = num_op_token << 3;

      navigateOption[ cur_opt ].idx = opt_sel;
      navigateOption[ cur_opt ].option_type = NavigateOption::option;
      navigateOption[ cur_opt ].load_slot = 0;
      navigateOption[ cur_opt ].first_opt = firstOpt;

      subptr = get_next_line( ',', subptr );
      memset( navigateOption[ cur_opt ].label, ' ', SCREEN_WIDTH_IN_CHARS );
      copy_till_delimiter( ',', subptr, navigateOption[ cur_opt ].label, 0 );
      navigateOption[ cur_opt ].label[ SCREEN_WIDTH_IN_CHARS-1 ] = '\0';

      // get each option value to the array
      int optIdx = 0;
      char options[256];
      char* optPtr;
      subptr = get_next_line( ',', subptr );
      copy_till_delimiter( ';', subptr, options, 1 );
      optPtr = options;
      while ( optPtr != NULL ) {        
        copy_till_delimiter( ',', optPtr, navigateOption[ cur_opt ].sel_label[optIdx], 1 );
        navigateOption[ cur_opt ].sel_label[optIdx][ OPTION_LEN-1 ] = '\0';

        int curOpt = firstOpt + optIdx;
        if ( curOpt == option_sel[ opt_sel ] ) {
          navigateOption[ cur_opt ].sel_idx = optIdx;
          navigateOption[ cur_opt ].sel_opt = curOpt;
        }
        optPtr = get_next_line( ',', optPtr );
        optIdx++;
      }
      navigateOption[ cur_opt ].n_values = optIdx;

      option_sel[ opt_sel ] = firstOpt | (option_sel[ opt_sel ] & 0x07);
    }
    //it's an toggle or "load another core" option
    else if ( subptr[ 0 ] == 'T' || subptr[ 0 ] == 'R' ) {

      // get identifier
      int num_op_token = codeToNumber( subptr[ 1 ] );
      int firstOpt = num_op_token << 3;
      
      navigateOption[ cur_opt ].first_opt = firstOpt;

      // set behaviour
      navigateOption[ cur_opt ].idx = opt_sel;
      navigateOption[ cur_opt ].option_type = (subptr[ 0 ] == 'R' ) ? NavigateOption::load_new_core : NavigateOption::toggle;
      navigateOption[ cur_opt ].load_slot = 0;

      option_sel[ opt_sel ] = firstOpt;

      subptr = get_next_line( ',', subptr );
      
      memset( navigateOption[ cur_opt ].label, ' ', SCREEN_WIDTH_IN_CHARS );
      copy_till_delimiter( ';', subptr, navigateOption[ cur_opt ].label, 0 );
      navigateOption[ cur_opt ].label[ SCREEN_WIDTH_IN_CHARS-1 ] = '\0';

      // clear options
      navigateOption[ cur_opt ].n_values = 0;
      
    } 
    else if ( subptr[ 0 ] == 'V' ) {
      char tmp_cv[SCREEN_WIDTH_IN_CHARS * VERSION_DISPLAY_LINES];
      subptr = get_next_line( ',', subptr );

      copy_till_delimiter( ';', subptr, tmp_cv, 1 );

      int c = 0;
      int ct = 0;
      int cv_line = 0;
      int max_width = (SCREEN_WIDTH_IN_CHARS - 3);

      while ( cv_line < VERSION_DISPLAY_LINES && tmp_cv[ct] != '\0' ) {
        if ( c == max_width-1 || tmp_cv[ct] == '|' ) {
          core_version[cv_line][c] = '\0';
          cv_line++;
          c = 0;
        }
        core_version[cv_line][c] = tmp_cv[ct];
        c++;
        ct++;
      }
      core_version[cv_line][c] = '\0';

      unknown = true;
    }
    else {
      unknown = true;
    }

    if ( !unknown ) {
      cur_opt++;
      opt_sel++;
    }

    // go to next line
    ptr = get_next_line( ';', ptr );
  }

  return cur_opt;
}

void buildOptionsDefaults() {
  Log.trace(CR"*** buildOptionsDefaults ***"CR);
  // parsing pointer
  char* ptr = str_conf;

  boolean unknown;
  int opt_sel = 0;

  while ( ptr != NULL ) {
    // parse specific options from the line
    char* subptr;
    // get first token, that specifies the option type
    subptr = ptr;

    unknown = false;

    if ( subptr[ 0 ] == 'O' ) {
      // get identifier
      int num_op_token = codeToNumber( subptr[ 1 ] );
      int firstOpt = num_op_token << 3;

      // Get the size of binary positions
      int last_op_token = 0;
      int lotidx = 0;
      do {
        last_op_token = codeToNumber( subptr[lotidx] );
        lotidx++;
      } while (subptr[lotidx] != ',' && subptr[lotidx] != '\0');
      int osize = last_op_token - num_op_token + 1;

      subptr = get_next_line( ',', subptr );

      // get each option value to the array
      int optIdx = 0;
      char options[256];
      char* optPtr;
      subptr = get_next_line( ',', subptr );
      copy_till_delimiter( ';', subptr, options, 1 );
      optPtr = options;
      while ( optPtr != NULL ) {
        optPtr = get_next_line( ',', optPtr );
        optIdx++;
      }

      int dsel = 0;
      for (int d = 0; d < osize; d++) {
        int dindex = firstOpt < 8 ? 0 : ((firstOpt/8) + d);
        dsel += (defaults_bin[dindex] * binaryPosition(d));
      }
      option_sel[ opt_sel ] = firstOpt + dsel;
    }
    else if ( subptr[ 0 ] == 'T'
              || subptr[ 0 ] == 'R'
              || (set_root_option && subptr[ 0 ] == 'H')
              || (!request_disable_sd && (subptr[ 0 ] == 'S' || subptr[ 0 ] == 'I' || subptr[ 0 ] == 'L'))
              ) {
      unknown = false;
    }
    else {
      unknown = true;
    }

    if ( !unknown ) {
      opt_sel++;
    }

    // go to next line
    ptr = get_next_line( ';', ptr );
  }
  SendStatusWord();
}

// C++ "pow(n,n)" consumes 7% of Sketch size :/
int binaryPosition(int p) {
  switch(p){
    case 0: return 1;
    case 1: return 2;
    case 2: return 4;
    case 3: return 8;
    case 4: return 16;
  }
}

void displayVersion() {
  if (strcmp(core_version[0], "") == 0) return;

  char line[SCREEN_WIDTH_IN_CHARS];

  Log.trace("Version: ");

  for ( int j = 0; j < MENU_MAX_LINES; j++ ) {
    strcpy( line, "                               " );
    if ( j == 1) {
      // copy title to line
      strcpy( line + ((SCREEN_WIDTH_IN_CHARS - 8) / 2), "Version:" );
    } else if ( j >= VERSION_FIRST_LINE && (j-VERSION_FIRST_LINE) < VERSION_DISPLAY_LINES ) {
      strcpy( line + ((SCREEN_WIDTH_IN_CHARS - strlen(core_version[j-VERSION_FIRST_LINE])) / 2), core_version[j-VERSION_FIRST_LINE] );
      Log.trace("%s", core_version[j-VERSION_FIRST_LINE]);
    }
    OsdWriteOffset( j % MENU_MAX_LINES, line, 0, 0, 0, 0);
  }
  Log.trace(CR);

  delay(3000);
}

void displayOptions(int page, int total_options, NavigateOption *navigateOption) {
  int i_opt;
  i_opt = page * MENU_MAX_LINES;

  for ( int j = i_opt; j < (i_opt+MENU_MAX_LINES); j++ ) {
    char line[SCREEN_WIDTH_IN_CHARS];
    if ( j >= total_options ) {
      strcpy( line, "                               " );
    } else {
      // copy title to line
      strcpy( line, navigateOption[j].label );

      int option_selected = option_sel[ navigateOption[j].idx ] & 0x07;
      strcpy( line + SCREEN_WIDTH_IN_CHARS - 1 - strlen( navigateOption[j].sel_label[navigateOption[j].sel_idx] ), navigateOption[j].sel_label[navigateOption[j].sel_idx] );
    }
    OsdWriteOffset( j % MENU_MAX_LINES, line, ( cur_select == j ), 0, 0, 0);
  }
}

boolean navigateOptions() {
//  strcpy(str_conf, "P,Spectrum.dat;S,TAP/CSW/TZX,Load *.TAP.CSW.TZX;O6,Fast tape load,On,Off;O7,Joystick,J1 Kempst J2 Sincl II,J1 Sincl II J2 Kempst;O89,Video timings,ULA-128,ULA-48,Pentagon;OFG,Scandoubler Fx,None,HQ2x,CRT 25%,CRT 50%;OAC,Memory,Standard 128K,Pentagon 1024K,Profi 1024K,Standard 48K,+2A +3;ODE,Features,ULA+ & Timex,ULA+,Timex,None;T0,Reset;V,v3.40.(BDI);;R,Load Other Core...;");

  unsigned int total_options = 0;
  unsigned int opt_sel = 0;
  unsigned int page = 0;
  boolean selectionChanged = false;
  // keyboard events
  int event;
  // extensions for loading files
  char exts[ 32 ];

  cur_select = 0;

  NavigateOption navigateOption[MAX_OPTIONS];

  clear_option_data( navigateOption );
  total_options = parseOptions( navigateOption );
  Log.trace("Total options: %d"CR, total_options);

  initOSD();

  traceOptions(page, total_options, navigateOption);

  // the options navigation loop
  while ( true ) {
    displayOptions(page, total_options, navigateOption);

    // set second SPI on PA12-PA15
    SPI.setModule( SPI_FPGA );

    // ensure SS stays high for now
    SPI_DESELECTED();
    //NSS_SET;
    SPI_SELECTED();
    //NSS_CLEAR;

    // read keyboard and act
    event = readKeyboard( &key, &cmd );

    if ( event == EVENT_KEYPRESS ) {
      if ( key == KEY_UP ) {
        if (selectionChanged) SendStatusWord();
        selectionChanged = false;
        if ( cur_select > 0 ) {
          cur_select--;
        } else {
          cur_select = total_options - 1;
        }
      }
      else if ( key == KEY_DOW ) {
        if (selectionChanged) SendStatusWord();
        selectionChanged = false;
        if ( cur_select < total_options - 1 ) {
          cur_select++;
        } else {
          cur_select = 0;
        }
      }
#ifndef DISABLE_NEW_OPTION_NAV
      else if ( key == KEY_LFT ) {
        changeSelection(-1, navigateOption[cur_select].option_type, navigateOption[cur_select].n_values );
        navigateOption[cur_select].sel_idx = ( option_sel[ cur_select ] & 0x07 );
        selectionChanged = true;
      }
      else if ( key == KEY_RGT ) {
        changeSelection(+1, navigateOption[cur_select].option_type, navigateOption[cur_select].n_values );
        navigateOption[cur_select].sel_idx = ( option_sel[ cur_select ] & 0x07 );
        selectionChanged = true;
      }
#endif
      else if ( key == KEY_V ) {
        displayVersion();
      }
      else if ( key == KEY_RET ) {
        // Log.notice("Option selected: idx: %d, type: %d"CR, navigateOption[ cur_select ].idx, navigateOption[ cur_select ].option_type);

        disable_soft_root = false;

        if ( navigateOption[ cur_select ].option_type == NavigateOption::load_file
              || navigateOption[ cur_select ].option_type == NavigateOption::mount_image
              ) {

          int s_index = navigateOption[cur_select].load_slot;
          unsigned char load_image = (navigateOption[cur_select].option_type == NavigateOption::mount_image);

          Log.notice("navigateOption[%d].load_slot = %d"CR, cur_select, navigateOption[cur_select].load_slot);

          get_extension_for_load_option( s_index, exts );
          Log.notice("Extensions for filtering: %s"CR, exts);

          OsdClear();

          changeToCoreRoot();

          if ( fileBrowser( exts, true ) ) {

            SPI_DESELECTED();
            SPI_SELECTED();

            if (load_image) {
              saveLastImage(&file_selected);
              mountImage(&file_selected, s_index);
            } else {
              dataPump(&file_selected, s_index);
            }

            OSDVisible(false);
            last_cmd = 1;
            last_key = 31;
            return true;
          }
        }
        else if ( navigateOption[ cur_select  ].option_type == NavigateOption::set_root ) {
          Log.trace( "Set ROOT directory"CR );
          OsdClear();
          select_dir_mode = true;
          char disableFilesExts[ 32 ] = { "none" };

          if ( fileBrowser( disableFilesExts, true ) ) {
            SPI_DESELECTED();
            SPI_SELECTED();

            OSDVisible(false);
            last_cmd = 1;
            last_key = 31;
          }
          select_dir_mode = false;
        }
        else if ( navigateOption[ cur_select  ].option_type == NavigateOption::load_new_core ) {
          Log.trace( "Load a new core"CR );
          disable_soft_root = true;
          menuLoadNewCore();
          if (CORE_ok) return false;
        }
        // toggle
        else if ( navigateOption[ cur_select  ].option_type == NavigateOption::toggle ) {
          int num_op_token = option_sel[ cur_select ] & 0xf8; // 11111000b

          option_sel[cur_select] = num_op_token | 0x01;
          Log.notice("Toggle active: %d"CR, option_sel[cur_select]);
          SendStatusWord();
          delay( 300 );

          option_sel[cur_select] = num_op_token;
          Log.notice("Toggle released: %d"CR, option_sel[cur_select]);
          SendStatusWord();

          //close the OSD
          OSDVisible(false);
          last_cmd = 1;
          last_key = 31;
          return true;
        }
        // option
        else {
          if (!selectionChanged) changeSelection(+1, navigateOption[cur_select].option_type, navigateOption[cur_select].n_values );
          navigateOption[cur_select].sel_idx = ( option_sel[ cur_select ] & 0x07 );
          selectionChanged = false;
          SendStatusWord();
        }
      }
      //F12 - abort
      else if ( ( cmd == 0x01 || cmd == 0x02 || cmd == 0x03 ) ) {
          if (menu_action > millis()) continue;
          resetMenuActionNextCycle();
          return true;
      }

      // recalculate page
      if ( page != cur_select / MENU_MAX_LINES ) {
        page = cur_select / MENU_MAX_LINES;
        OsdClear();
      }

      traceOptions(page, total_options, navigateOption);
    } // end if keypress
  } // end while
}
