package br.com.multicore;

import org.codehaus.plexus.util.StringUtils;
import org.jnativehook.GlobalScreen;
import org.jnativehook.NativeHookException;
import org.jnativehook.keyboard.NativeKeyEvent;
import org.jnativehook.keyboard.NativeKeyListener;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.logging.Level;
import java.util.logging.Logger;

import static java.lang.System.exit;
import static java.lang.Thread.sleep;
import static java.util.Arrays.*;

public class Main implements NativeKeyListener {

    private final static int MAX_OPTIONS = 32;
    private final static LinkedList<NavOption> navOptions = new LinkedList<>();
    private static int cur_sel;
    private static boolean keyPressed;
    private static boolean finish;

    public static void main(String[] args) throws Exception {
        disableLogging();

        Params params = new Params(args);
        String input = params.options;

        if (StringUtils.isEmpty(input)) {
            exit(1);
        }

        if (input.startsWith("OPTIONS=")) {
            System.out.println("DEFAULT=0x" + optionsToDefault(input));
            exit(0);
        }

        parseInput(input);

        try {
            if (!GlobalScreen.isNativeHookRegistered()) {
                GlobalScreen.registerNativeHook();
                GlobalScreen.addNativeKeyListener(new Main());
            }
        } catch (NativeHookException nhe) {
            System.out.println(nhe.getMessage());
            System.out.println("Controls keys disabled. Change OS permissions to enable.");
        }

        while ( !finish ) {
            cls();

            displayControls();
            displayNavOptions();
            displayOptionsLine();

            while (!keyPressed) sleep(100);
            keyPressed = false;
        }

        try {
            GlobalScreen.unregisterNativeHook();
        } catch (NativeHookException e) {
            System.out.println(e.getMessage());
        }
    }

    private static void displayControls() {
        System.out.println("Use <UP>, <DOWN>, <LEFT>, <RIGHT> to navigate.");
        System.out.println("<ESC> to finish.\n\n");
    }

    private static void disableLogging() {
        Arrays.stream(Logger.getLogger("").getHandlers()).forEach(h -> h.setLevel(Level.OFF));
    }

    private static void parseInput(String input) {
        String[] options = input.split(";");
        int idx = 0;
        for (String option : options) {
            if (isVisibleOption(option)) {
                NavOption navOption = new NavOption();
                LinkedList<String> tokens = new LinkedList<>(asList(option.split(",")));
                tokens.removeFirst();
                navOption.idx = idx;
                navOption.label = String.valueOf(tokens.removeFirst());
                navOption.currentIdx = 0;
                if (option.startsWith("R")) {
                    navOption.defaultOpt = 96;
                } else if (option.startsWith("O")) {
                    navOption.defaultOpt = codeToNumber(option.charAt(1)) << 3;
                    navOption.values = new LinkedList<>(tokens);
                } else if (option.startsWith("S") || option.startsWith("I") || option.startsWith("L")) {
                    navOption.label = String.valueOf(tokens.removeFirst());
                }
                navOptions.add(navOption);
                idx++;
            }
        }
    }

    private static void cls() {
        System.out.println("\033[H\033[2J");
        System.out.flush();
    }

    private static void displayOptionsLine() {
        int i;
        long statusWord = 0;

        System.out.println();
        String options = "OPTIONS=";

        for ( i = 0; i < navOptions.size(); i++ ) {
            if (i > 0) options += ",";
            int optionSel = navOptions.get(i).defaultOpt + navOptions.get(i).currentIdx;
            options += optionSel;
            statusWord |= (long) (optionSel & 0x07) << ( optionSel >> 3 );
        }

        for ( int h = i; h < MAX_OPTIONS-1; h++ ) {
            options += ",0";
        }
        options += ",255";
        System.out.println(options);
        System.out.println("StatusWord (L): " + statusWord);
        System.out.println("StatusWord (H): 0x" + Long.toHexString(statusWord).toUpperCase());
        System.out.println("StatusWord (B): " + Long.toBinaryString(statusWord));
    }

    private static String optionsToDefault(String options) {
        long statusWord = 0;

        options = options.replaceAll("OPTIONS=", "");
        String[] optionSelArray = options.split(",");

        for (String optionSel : optionSelArray) {
            int os = Integer.parseInt(optionSel);
            if (os == 255) os = 0;
            statusWord |= (long) (os & 0x07) << ( os >> 3 );
        }
        return Long.toHexString(statusWord).toUpperCase();
    }

    private static void displayNavOptions() {
        for ( NavOption option : navOptions ) {
            if (option.idx == cur_sel) {
                System.out.print(">");
            } else {
                System.out.print(" ");
            }
            System.out.printf("(%d) ", option.defaultOpt/8);
            System.out.printf("%1$-32s ( ", option.label);
            for (int v = 0; v < option.values.size(); v++) {
                if ( v > 0 ) System.out.print(",");
                String value = option.values.get(v);
                if ( v == option.currentIdx) {
                    System.out.print("["+value+"]");
                } else {
                    System.out.print(value);
                }
            }
            System.out.println(" )");
        }
    }

    static boolean isVisibleOption(String option) {
        return ( option.startsWith("T")
                || option.startsWith("R")
                || option.startsWith("O")
                || option.startsWith("S")
                || option.startsWith("I")
                || option.startsWith("H")
                || option.startsWith("L")
        );
    }

    static int codeToNumber(char code) {
        if ( code >= '0' && code <= '9' ) return code - 48;
        if ( code >= 'A' && code <= 'Z' ) return code - 55;

        return 0;
    }

    @Override
    public void nativeKeyTyped(NativeKeyEvent nativeKeyEvent) {

    }

    @Override
    public void nativeKeyPressed(NativeKeyEvent nativeKeyEvent) {

    }

    @Override
    public void nativeKeyReleased(NativeKeyEvent nativeKeyEvent) {
        switch (nativeKeyEvent.getKeyCode()) {
            case NativeKeyEvent.VC_DOWN:
                if (cur_sel < navOptions.size()-1) cur_sel++;
                break;
            case NativeKeyEvent.VC_UP:
                if (cur_sel > 0) cur_sel--;
                break;
            case NativeKeyEvent.VC_RIGHT:
                if (navOptions.get(cur_sel).currentIdx < navOptions.get(cur_sel).values.size()-1 )
                    navOptions.get(cur_sel).currentIdx++;
                break;
            case NativeKeyEvent.VC_LEFT:
                if (navOptions.get(cur_sel).currentIdx > 0 )
                    navOptions.get(cur_sel).currentIdx--;
                break;
            case NativeKeyEvent.VC_ESCAPE:
                finish = true;
                break;
        }
        keyPressed = true;
    }
}
