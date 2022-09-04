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

bool readLauncher(char *filename) {
  SdFile file;
  FileEntry launcherSelected;

  Log.notice( "Launcher file: '%s' "CR, filename );

  file.open( filename, O_READ );

  SdFileToFileEntry(&file, &launcherSelected);

  strcpy(launcher_selected, launcherSelected.long_name);

  if ( file.isOpen( ) ) {
    char line[128];
    char *token;
    unsigned int n;
    int char_count = 0;

    while ( ( n = file.fgets( line, sizeof( line ) ) ) > 0 ) {
      int i = 0;
      for ( i = 0; i < 128; i++ ) {
        //clip the line at the CR or LF
        if ( line[i] == 0x0d || line[i] == 0x0a) {
          line[i] = '\0';
          break;
        }
      }

      token = strtok( line, "=" );

      if ( strcmp( strlwr( token ), "rbf" ) == 0 ) {
        //RBF option, points to Core's binary
        token = strtok(NULL, "=");
        char core_rbf[LONG_FILENAME_LEN];
        strcpy(core_rbf, token);
        strcat(core_rbf, ".");
        strcat(core_rbf, LAUNCHER_CORE_EXT);

        SdFile coreRbf;
        coreRbf.open( core_rbf, O_READ );
        if (coreRbf.isOpen()) {
          SdFileToFileEntry(&coreRbf, &file_selected);
          coreRbf.close();
        } else {
          strcat(core_rbf, " not found!!");
          errorScreen(core_rbf);
          return false;
        }
      } else if ( strcmp( strlwr( token ), "mod" ) == 0 ) {
        //MOD option, to assign a hardware number (core variant)
        token = strtok(NULL, "=");
        core_mod = atoi(token);

      } else if ( strcmp( strlwr( token ), "load_data" ) == 0 ) {
        token = strtok(NULL, "=");
        strcpy(load_data, token);

      }

    } // end while
  } else {
    Log.notice(" ... not found."CR);
  }
  file.close();

  return true;
}

bool readLauncherConf(unsigned int char_count) {
  if ( strcmp(launcher_selected, "") == 0 ) return false;

  SdFile file;

  Log.notice( "Launcher Conf file: '%s' "CR, launcher_selected );

  file.open( launcher_selected, O_READ );

  if ( file.isOpen( ) ) {
    char line[128];
    char *token;
    unsigned int n;

    char_count--;

    while ( ( n = file.fgets( line, sizeof( line ) ) ) > 0 ) {
      int i = 0;
      for ( i = 0; i < 128; i++ ) {
        //clip the line at the CR or LF
        if ( line[i] == 0x0d || line[i] == 0x0a) {
          line[i] = '\0';
          break;
        }
      }

      token = strtok( line, "=" );

      //CONF option, add a config string to the menu
      if ( strcmp( strlwr( token ), "conf" ) == 0 ) {
        token = strtok( NULL, "=" );
        int i = 0;

        str_conf[char_count] = ';';
        char_count++;

        while ( token[i] ) {
          //remove quotes and CR LF
          if ( token[i] != '"' ) {
            str_conf[char_count] = token[i];
            char_count++;
          }
          i++;
        }

      } else if ( strcmp( strlwr( token ), "default" ) == 0 ) {
        token = strtok(NULL, "=");
        char bin[35];
        hex_to_bin(bin, token);
        Log.trace("Hex to Bin (%i): %s -> %s"CR, strlen(bin), token, bin);

        int pos = 0;
        for ( int s = strlen(bin)-1; s >= 0; s-- ) {
          defaults_bin[pos] = ( bin[s] == '1' ? 1 : 0 );
//          Log.trace("%i:%i,",pos, defaults_bin[pos]);
          pos++;
        }

      } else if ( strcmp( strlwr( token ), "default_opt" ) == 0 ) {
        int i = 0;
        token = strtok( NULL, "," );
        while ( token ) {
          option_sel[i] = atoi( token );
          token = strtok( NULL, "," );
          i++;
        }
        SendStatusWord();

      } else if ( strcmp( strlwr( token ), "name" ) == 0 ) {
        //NAME option, to force a CORE_NAME (different than the loaded)
        token = strtok(NULL, "=");
        strcpy(core_name, token);

      }

    } // end while
    str_conf[char_count] = ';';
    str_conf[++char_count] = '\0';
  } else {
    Log.notice(" ... not found."CR);
  }
  file.close();

  return true;
}

const char* hex_char_to_bin(char c)
{
  switch(toupper(c)) {
    case '0': return "0000";
    case '1': return "0001";
    case '2': return "0010";
    case '3': return "0011";
    case '4': return "0100";
    case '5': return "0101";
    case '6': return "0110";
    case '7': return "0111";
    case '8': return "1000";
    case '9': return "1001";
    case 'A': return "1010";
    case 'B': return "1011";
    case 'C': return "1100";
    case 'D': return "1101";
    case 'E': return "1110";
    case 'F': return "1111";
    default: return "";
  }
}

void hex_to_bin(char *bin, char *hex) {
  strcpy(bin, "");
  for(int i = 0; i < 10; i++){
    if (hex[i] == '\0') break;
    if ( strcmp(hex_char_to_bin(hex[i]), "") == 0 ) {
      strcpy(bin, "");
    }
    strcat(bin, hex_char_to_bin(hex[i]));
  }
  char cbin[35];
  while (strlen(bin) < 32) {
    strcpy(cbin, "0000");
    strcat(cbin, bin);
    strcpy(bin, cbin);
  }
}
