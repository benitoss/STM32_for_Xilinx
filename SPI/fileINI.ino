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

bool readIni(unsigned int char_count) {
  bool optionsFound = false;
  core_root[0] = '\0';

  if ( !SD_disabled ) {

    SdFile file;
    char fileIni[LONG_FILENAME_LEN];

    bool launcherConfFound = readLauncherConf(char_count);

    // Change to Core's directory before reading INI
    changeToCorePath();

    strcpy( fileIni, core_name );
    strcat( fileIni, ".ini" );
    file.open( fileIni, O_READ );

    Log.notice( "Search INI: '%s%s' ", core_path, fileIni );

    if ( file.isOpen( ) ) {

      char line[128];
      char *token;
      unsigned int n;

      if ( !launcherConfFound ) char_count--;

      Log.notice(" ... found."CR);

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

      if ( strcmp( strlwr( token ), "root" ) == 0 ) {
          token = strtok(NULL, "=");
          changeToDir(token);
          setCoreRoot();

        } else if ( strcmp( strlwr( token ), "last" ) == 0 ) {
          token = strtok(NULL, "=");
          changeToDir(token);
          setLastPath();

        } else if ( !image_loaded && strcmp( strlwr( token ), "load_image" ) == 0 ) {
          token = strtok(NULL, "=");
          loadImage(token);
          image_loaded = true;

        } else if ( strcmp( strlwr( token ), "options" ) == 0 ) {
          int i = 0;
          token = strtok( NULL, "," );
          while ( token ) {
            option_sel[i] = atoi( token );
            token = strtok( NULL, "," );
            i++;
          }
          optionsFound = true;
        } else if ( strcmp( strlwr( token ), "dis_sd" ) == 0 ) {
          //Disable SD option, for some cores that use direct connection to the external SD
          request_disable_sd = 1;

        } 
        // #DEPRECATED: Configs moved to fileLauncher. Added here to keep compatibility
        else if ( !launcherConfFound && strcmp( strlwr( token ), "mod" ) == 0 ) {
          //MOD option, to assign a hardware number (core variant)
          token = strtok(NULL, "=");
          core_mod = atoi(token);
        }
        // #DEPRECATED: Configs moved to fileLauncher. Added here to keep compatibility
        else if ( !launcherConfFound && strcmp( strlwr( token ), "conf" ) == 0 ) {
          //CONF option, add a config string to the menu
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
        }
      } // end while
      if ( !launcherConfFound ) {
        str_conf[char_count] = ';';
        str_conf[++char_count] = '\0';
      }
    } else {
      Log.notice(" ... not found."CR);
    }
    file.close();

    // Go back to current path after reading
    sd1.chdir(path);

  } // end if ! SD_DISABLED

  return optionsFound;
}

void saveLastImage(FileEntry *file) {
  if ( strcmp(file->long_name, "") == 0 || !confirmAutoLoad() ) return;
  strcpy(image_filename, path);
  strcat(image_filename, file->long_name);

  saveIni();
}

void saveIni(void) {
  traceSaveOptions();
  if ( SD_disabled ) {
    return;
  }
  SdFile dir;
  SdFile file;
  SdFile tfile;
  char fileIni[LONG_FILENAME_LEN];
  char tempIni[LONG_FILENAME_LEN];  

  strcpy(fileIni, core_name);
  strcat(fileIni, ".ini");

  strcpy(tempIni, core_name);
  strcat(tempIni, ".tmp");

  if ( strncmp( fileIni, ".ini", 4 ) == 0 ) {
    Log.trace("Aborting SaveIni"CR);
    return;
  }

  changeToCorePath();

  file.open( fileIni, O_READ );  
  tfile.open( tempIni, O_WRITE | O_CREAT );

  if ( tfile.isOpen() ) {
    char line[128];
    char *token;
    unsigned int n;

    if (file.isOpen()) {
      while ( ( n = file.fgets( line, sizeof( line ) ) ) > 0) {
        if (
          strncmp( line, "CONF", 4 ) == 0 ||
          strncmp( line, "DIS_SD", 6 ) == 0 ||
          strncmp( line, "MOD", 3 ) == 0 ||
          (strcmp(image_filename, "") == 0 && strncmp( line, "LOAD_IMAGE", 10 ) == 0)
          ) {
          tfile.print(line);
        }
      }
    }

    if ( core_root[0] != '\0' ) {
      tfile.print( "ROOT=" );
      tfile.print( core_root );
      Log.trace("Set ROOT='%s'"CR, core_root);
    } else if ( image_filename[0] != '\0' ) {
      tfile.print( "LOAD_IMAGE=" );
      tfile.print( image_filename );
      Log.trace("Set LOAD_IMAGE='%s'"CR, image_filename);
      strcpy(image_filename, "");
    } else {
      tfile.print( "LAST=" );
      tfile.print( last_path );
      Log.trace("Set LAST='%s'"CR, last_path);
    }
    tfile.print( CR );

    //add the options to the file
    tfile.print( "OPTIONS=" );
    for ( int i = 0; i < MAX_OPTIONS; i++ ) {
      tfile.print( option_sel[i], DEC );
      if ( i < (MAX_OPTIONS - 1) ) {
        tfile.print( "," );
      }
    }

    Log.notice( "SaveIni: '%s%s'"CR, core_path, fileIni );
  } else {
    Log.notice( "SaveIni Error: '%s%s'"CR, core_path, fileIni );
  }

  // Go back to current path after saving
  file.close();
  tfile.close();

  sd1.remove(fileIni);
  sd1.rename(tempIni, fileIni);  

  changeToLastPath();
}

void traceSaveOptions() {
#ifndef DISABLE_LOGGING
  Log.verbose("OPTIONS=");
  for ( int i = 0; i < MAX_OPTIONS; i++ ) {
    Log.verbose( "%d", option_sel[i] );
    if ( i < (MAX_OPTIONS - 1) ) {
      Log.verbose( "," );
    }
  }
  Log.verbose(CR);
#endif
}
