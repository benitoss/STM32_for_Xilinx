package br.com.multicore;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.stream.Stream;

import static java.lang.System.exit;

class Params {
    public String options;

    Params(String[] args) {
        if (args.length < 1) {
            displayUsage();
            exit(1);
        } else {
            if (args[0].startsWith("OPTIONS=") || args[0].contains(";")) {
                options = args[0];
            } else {
                try (Stream<String> stream = Files.lines(Paths.get(args[0]))) {
                    options = stream.findFirst().get();
                } catch (IOException e) {
                    System.out.println("Error opening file: "+ args[0]);
                    displayUsage();
                    exit(1);
                }
            }
        }
    }

    private void displayUsage() {
        System.out.println("Usage: ");
        System.out.println("> gen-core-opt <options-string>");
        System.out.println("> gen-core-opt <input-file>");
        System.out.println();
        System.out.println("Example:");
        System.out.println("> gen-core-opt \"D,disable STM SD;OAB,Scanlines,Off,25%,50%,75%;OC,Blend,Off,On;O2,CPU Clock,Normal,Turbo;O3,Slot1,MegaSCC+ 1MB,Empty;O45,Slot2,MegaSCC+ 2MB,Empty,MegaRAM 2MB,MegaRAM 1MB;O6,RAM,2048kB,4096kB;OD,Slot 0,Expanded,Primary;O7,OPL3 sound,Yes,No;O9,Tape sound,Off,On;O1,Scandoubler,On,Off;OEF,Keymap,BR,ES,FR,US;T0,Reset;V,v1.0.210109;R,Load Other Core...;\"");
        System.out.println("OR");
        System.out.println("> gen-core-opt smx.opt");
        System.out.println("# Where 'smx.opt' have the same first example's string inside (without quotes)");
        System.out.println("OR");
        System.out.println("> gen-core-opt OPTIONS=0,9,24,40,56,0,64,81,121,96,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
        System.out.println("# Converts to an option to use on ARC files: DEFAULT=0x8402");
    }
}