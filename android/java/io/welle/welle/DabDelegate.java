package io.welle.welle;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

public class DabDelegate extends Activity {
    private static final String TAG = DabDelegate.class.getSimpleName();

    public static void startDab(Context context) {
        context.startActivity(new Intent(context, DabDelegate.class));
    }

    private ServiceConnection mConnection = null;

    private boolean startSdrActivity(UsbDevice usbDevice) {
        try {
            Intent intent = new Intent(Intent.ACTION_VIEW).setData(
                    Uri.parse("iqsrc://-a " + DabService.SDR_ADDRESS
                            + " -p " + DabService.SDR_PORT
                            + " -s " + DabService.SDR_SAMPLERATE));
            if (usbDevice != null) {
                intent.putExtra(UsbManager.EXTRA_DEVICE, usbDevice);
            }
            startActivityForResult(intent, 1234);
            return true;
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "RTL-SDR error: " + "Android RTL-SDR driver is not installed");
            return false;
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");

        Intent intent = getIntent();
        if (intent != null && intent.hasExtra(UsbManager.EXTRA_DEVICE)) {
            UsbDevice usbDevice = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            Log.i(TAG, "USB attached: " + usbDevice.getDeviceName());
            startSdrActivity(usbDevice);
        } else {
            Log.i(TAG, "DAB started");
            mConnection = new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder binder) {
                    DabService.DabBinder dabBinder = (DabService.DabBinder) binder;
                    if (dabBinder.isDeviceAvailable()) {
                        finish();
                    } else if (!startSdrActivity(null)) {
                        //TODO show Android Install Dialog & Error
                    }
                }

                @Override
                public void onServiceDisconnected(ComponentName name) {
                    finish();
                }
            };
            startService(new Intent(this, DabService.class));
            bindService(new Intent(this, DabService.class), mConnection, Context.BIND_AUTO_CREATE);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != 1234) return; // This is the requestCode that was used with startActivityForResult
        if (resultCode == RESULT_OK) {
            // Connection with device has been opened and the rtl-tcp server is running. You are now responsible for connecting.
            Intent serviceIntent = new Intent(this, DabService.class);
            serviceIntent.setAction(DabService.ACTION_SDR_DEVICE_ATTACHED);
            serviceIntent.putExtras(data.getExtras());
            startService(serviceIntent);
        } else if (data != null) {
            // something went wrong, and the driver failed to start
            String errmsg = data.getStringExtra("detailed_exception_message");
            Log.e(TAG, "RTL-SDR error: " + errmsg);
        }
        finish();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mConnection != null) {
            unbindService(mConnection);
        }
    }
}
