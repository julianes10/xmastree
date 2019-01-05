package com.omegastar.xmastree;


import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.bluetooth.*;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.skydoves.colorpickerview.ColorEnvelope;
import com.skydoves.colorpickerview.ColorPickerView;
import com.skydoves.colorpickerview.listeners.ColorEnvelopeListener;
import com.skydoves.colorpickerview.listeners.ColorListener;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.LinkedList;
import java.util.Queue;
import java.util.Set;
import java.util.UUID;

import static android.bluetooth.BluetoothAdapter.STATE_ON;




public class MainActivity extends AppCompatActivity {
    public boolean dd; //debug detail
    public boolean d;  //debug

    BluetoothAdapter bluetooth;
    private String btmac;
    SharedPreferences preferences;
    BluetoothDevice peerDevice;


    BluetoothSocket mmSocket;
    OutputStream mmOutputStream;
    InputStream mmInputStream;

    //For Rx stuff
    Thread workerThread;
    byte[] readBuffer;
    int    readBufferPosition;
    volatile boolean stopWorker;

    //For controlled Tx stuff
    Thread workerThreadTx;
    volatile boolean stopWorkerTx;
    Queue<String> q=new LinkedList<String>();


    volatile int comStatus; //XOFF, XON
    public static final int ST_UNKNOWN = 0;
    public static final int ST_XON_INPROGRESS = 1;
    public static final int ST_XON = 2;

    int[] color= new int[]{0, 0, 0, 0};  //Argb




    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        d=true;
        comStatus=ST_UNKNOWN;
        if (d) Log.d("DXMASTREE", "*************************CREATE MAIN*********************************");



        bluetooth = BluetoothAdapter.getDefaultAdapter();

        if(bluetooth != null)
        {
            String status;
            if (bluetooth.isEnabled()) {
                String mydeviceaddress = bluetooth.getAddress();
                String mydevicename = bluetooth.getName();
                String state = "ON";
                if ( bluetooth.getState() != STATE_ON)
                    state="NOT ON";
                status = mydevicename + " : " + mydeviceaddress + " : " +  state;
            }
            else
            {
                status = "Bluetooth is not Enabled.";
            }

            Toast.makeText(this, status, Toast.LENGTH_LONG).show();
            TextView t = (TextView) findViewById(R.id.localBtStatus);
            t.setText(status);
        }


        String aux="98:D3:32:20:FB:90";  //Default
        try {

            preferences = PreferenceManager.getDefaultSharedPreferences(this.getApplicationContext());
            aux=preferences.getString("btmac",null);
        }
        catch (Exception e) {
            e.printStackTrace();
            if (d) Log.d("DXMASTREE", "Error getting settings bt mac");
        }
        setBtmac(aux);
        ColorPickerView colorPickerView = (ColorPickerView) findViewById(R.id.colorPickerView);
        colorPickerView.setColorListener(new ColorEnvelopeListener() {
            @Override
            public void onColorSelected(ColorEnvelope envelope, boolean fromUser) {
                color=envelope.getArgb();
                /*showRx("ColorEnvelopeListener:#"
                            + Integer.toHexString(envelope.getArgb()[0])
                            + Integer.toHexString(envelope.getArgb()[1])
                            + Integer.toHexString(envelope.getArgb()[2])
                            + Integer.toHexString(envelope.getArgb()[3]) );*/
            }
        });
    }

    private void setBtmac (String d) {
        EditText editText = (EditText) findViewById(R.id.target);
        editText.setText(d);
        editText.postInvalidate(); // to update
        this.btmac = d;
    }

    public void apply(View view) {

        EditText editText = (EditText) findViewById(R.id.target);
        setBtmac(editText.getText().toString());
        //refreshData();
        SharedPreferences.Editor editor = preferences.edit();
        editor.putString("btmac", editText.getText().toString()); // value to store
        editor.commit();

        try
        {
            findBT();
            openBT();
        }
        catch (IOException ex) { }
    }

    void findBT()
    {
        if(bluetooth == null)     return;

        if(!bluetooth.isEnabled())
        {
            if (d) Log.d("DXMASTREE", "ACTION_REQUEST_ENABLE...");
            Intent enableBluetooth = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBluetooth, 0);
        }

        Set<BluetoothDevice> pairedDevices = bluetooth.getBondedDevices();
        if (d) Log.d("DXMASTREE", "getBondedDevices...");
        if(pairedDevices.size() > 0)
        {
            for(BluetoothDevice device : pairedDevices)
            {
                if (d) Log.d("DXMASTREE", "Checking BondedDevices" + device.getName() + "(" + device.getAddress() +  ")...");
                if(device.getAddress().equalsIgnoreCase(this.btmac))
                {
                    peerDevice = device;
                    break;
                }
            }
        }
        else { if (d) Log.d("DXMASTREE", "None BondedDevices...");}
        String status="Not paired device: " + this.btmac;
        if (peerDevice != null) {
            status = peerDevice.getName() + " " + peerDevice.getAddress() + "CONNECTED";
        }
        Toast.makeText(this, status, Toast.LENGTH_LONG).show();
        TextView t = (TextView) findViewById(R.id.peerBtStatus);
        t.setText(status);

    }

    void openBT() throws IOException
    {
        UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); //Standard SerialPortService ID
        mmSocket = peerDevice.createRfcommSocketToServiceRecord(uuid);
        mmSocket.connect();
        mmOutputStream = mmSocket.getOutputStream();
        mmInputStream = mmSocket.getInputStream();

        beginListenForData();
        beginSenderData();
    }

    void beginListenForData()
    {
        final Handler handler = new Handler();
        final byte delimiter = 10; //This is the ASCII code for a newline character

        stopWorker = false;
        readBufferPosition = 0;
        readBuffer = new byte[1024];
        workerThread = new Thread(new Runnable()
        {
            public void run()
            {
                while(!Thread.currentThread().isInterrupted() && !stopWorker)
                {
                    try
                    {
                        int bytesAvailable = mmInputStream.available();
                        if(bytesAvailable > 0)
                        {
                            byte[] packetBytes = new byte[bytesAvailable];
                            mmInputStream.read(packetBytes);
                            for(int i=0;i<bytesAvailable;i++)
                            {
                                byte b = packetBytes[i];
                                if(b == delimiter)
                                {
                                    byte[] encodedBytes = new byte[readBufferPosition];
                                    System.arraycopy(readBuffer, 0, encodedBytes, 0, encodedBytes.length);
                                    final String data = new String(encodedBytes, "US-ASCII");
                                    readBufferPosition = 0;

                                    handler.post(new Runnable()
                                    {
                                        public void run() {
                                            showRx(data);
                                            if (d) Log.d("DXMASTREE", "msg received:"+data);
                                            if (data.substring(0,1).equals("F")) {
                                                comStatus = ST_UNKNOWN;
                                                if (d) Log.d("DXMASTREE", "XOFF received");
                                            }
                                            else if (data.substring(0,1).equals("X")){ //(data.equals("X")) {
                                                comStatus = ST_XON;
                                                if (d) Log.d("DXMASTREE", "XON received");
                                            }
                                        }
                                    });
                                }
                                else
                                {
                                    readBuffer[readBufferPosition++] = b;
                                }
                            }
                        }
                    }
                    catch (IOException ex)
                    {
                        stopWorker = true;
                        if (d) Log.d("DXMASTREE", "IOException reading data");
                    }
                }
            }
        });

        workerThread.start();
    }

    void beginSenderData()
    {
        final Handler handler = new Handler();
        final byte delimiter = 10; //This is the ASCII code for a newline character

        stopWorkerTx = false;
        workerThreadTx = new Thread(new Runnable()
        {
            public void run()
            {
                while(!Thread.currentThread().isInterrupted() && !stopWorkerTx)
                {
                    try
                    {
                        String msg;
                        if (q.isEmpty()) continue;
                        switch(comStatus) {
                            case ST_XON:
                                msg = q.poll();
                                if (msg!=null) {
                                    msg += "\n";
                                    mmOutputStream.write(msg.getBytes());
                                    if (d) Log.d("DXMASTREE", "Sent a msg:" + msg);
                                }
                                break;
                            case ST_XON_INPROGRESS:
                                //if (dd) Log.d("DXMASTREE", "Waiting rx XON...");
                                ;//TODO
                                // WAIT UNTIL RX, ELSE TIMOUT AND MOVE TO UNKONWEN comStatus=ST_XON;

                                break;
                            default:
                                msg = "XON\n";
                                mmOutputStream.write(msg.getBytes());
                                comStatus=ST_XON_INPROGRESS;
                                if (d) Log.d("DXMASTREE", "XON sent");
                        }
                    }
                    catch (IOException ex)
                    {
                        stopWorkerTx = true;
                        if (d) Log.d("DXMASTREE", "IOException reading data");
                    }
                }
            }
        });

        workerThreadTx.start();
    }


    public void showRx(String info) {
        ((EditText)(findViewById(R.id.etRxBox))).append(info);
    }


    public void changeMode(View view) {
        try {
                for (int i = 0; i < 1; i++) {
                    String msg = ":LX:LT0000:LMA:LC" +
                            String.format("%02X", color[1]) + "," +
                            String.format("%02X", color[2]) + "," +
                            String.format("%02X", color[3]);
                    q.add(msg);
                }
        }
        catch (Exception ex) {
            if (d) Log.d("DXMASTREE", "Change mode  ");
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        /*LocalBroadcastManager.getInstance(this).registerReceiver((receiver),
                new IntentFilter("REFRESH_DHT"));
        LocalBroadcastManager.getInstance(this).registerReceiver((receiver),
                        new IntentFilter("REFRESH_KODI"));
        timerActive=true;*/
    }






}
