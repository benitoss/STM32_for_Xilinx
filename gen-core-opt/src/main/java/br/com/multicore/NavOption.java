package br.com.multicore;

import java.util.LinkedList;

public class NavOption {
    // P,CORE_NAME.dat;S0,NES,Load NES Game...;OAB,System Type,NTSC,PAL,Dendy,Vs

    public int idx;
    public String label;
    public LinkedList<String> values = new LinkedList<>();
    public int defaultOpt;
    public int currentIdx;
}
