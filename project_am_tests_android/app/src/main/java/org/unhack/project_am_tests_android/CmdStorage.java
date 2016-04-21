package org.unhack.project_am_tests_android;

import android.content.Intent;

import com.maxmpz.poweramp.player.PowerampAPI;

import java.util.HashMap;

/**
 * Created by unhack on 4/15/16.
 */
public class CmdStorage {
    public HashMap<String, Cmd> pre_build_commands = new HashMap<>();
    public HashMap<Byte, Intent> android_commands = new HashMap<>();

    public CmdStorage(){
        pre_build_commands.put("HeadUnitOn",new Cmd((byte)1).getCommand());
        pre_build_commands.put("Clear",new Cmd((byte)0).getCommand());
        pre_build_commands.put("Menu",new Cmd((byte)10).getCommand());
        pre_build_commands.put("Esc",new Cmd((byte)11).getCommand());
        pre_build_commands.put("Ok",new Cmd((byte)16).getCommand());
        pre_build_commands.put("RightButton",new Cmd((byte)12).getCommand());
        pre_build_commands.put("LeftButton",new Cmd((byte)13).getCommand());
        pre_build_commands.put("UpButton",new Cmd((byte)14).getCommand());
        pre_build_commands.put("DownButton",new Cmd((byte)15).getCommand());
        android_commands.put((byte)100,new Intent(PowerampAPI.ACTION_API_COMMAND).putExtra(PowerampAPI.COMMAND, PowerampAPI.Commands.NEXT).setPackage(PowerampAPI.PACKAGE_NAME));
        android_commands.put((byte)101,new Intent(PowerampAPI.ACTION_API_COMMAND).putExtra(PowerampAPI.COMMAND, PowerampAPI.Commands.PREVIOUS).setPackage(PowerampAPI.PACKAGE_NAME));
        android_commands.put((byte)104,new Intent(PowerampAPI.ACTION_API_COMMAND).putExtra(PowerampAPI.COMMAND, PowerampAPI.Commands.TOGGLE_PLAY_PAUSE).setPackage(PowerampAPI.PACKAGE_NAME));

    }

    public byte[] getCommand(String cmd_key){
        return pre_build_commands.get(cmd_key).getBytes();
    }

    public Intent getAndroidIntent(byte code){
        if (android_commands.containsKey(code)){
            return android_commands.get(code);
        }
        return null;
    }
}
