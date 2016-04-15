package org.unhack.project_am_tests_android;

/**
 * Created by unhack on 4/15/16.
 */
public class Cmd {
    byte [] cmd = new byte[]{(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0,(byte)0};
    public Cmd(byte code){
        cmd[0] = 113;
        cmd[1] = code;
    }

    public Cmd getCommand(){
        short crc = (short) (cmd[0] + cmd[1]);
        cmd[13] = (byte)((crc >> 8) & 0xff);
        cmd[14] = (byte)(crc & 0xff);
        return this;
    }

    public byte[] getBytes(){
        return cmd;
    }
}
