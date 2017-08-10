package io.welle.welle;

import android.app.Activity;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

public class DabDelegate extends Activity {
    private static final String TAG = DabDelegate.class.getSimpleName();

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "DabDelegate onCreate");

        Intent intent = getIntent();
        Log.i(TAG, "DabDelegate onCreate"
                + " action: " + intent.getAction()
                + " component: " + intent.getComponent().toString()
                + " dataStr: " + intent.getDataString());

        if (intent.getAction().equals(DabService.ACTION_SDR_DEVICE_ATTACHED)) {
            UsbDevice usbDevice = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            Log.i(TAG, "USB attached: " + usbDevice.getDeviceName());

            Intent newIntent = new Intent(Intent.ACTION_VIEW).setData(
                    Uri.parse("iqsrc://-a " + DabService.SDR_ADDRESS
                            + " -p " + DabService.SDR_PORT
                            + " -s " + DabService.SDR_SAMPLERATE));
            newIntent.putExtra(UsbManager.EXTRA_DEVICE, usbDevice);
            startActivityForResult(newIntent, 1234);
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
        } else {
            // something went wrong, and the driver failed to start
            String errmsg = data.getStringExtra("detailed_exception_message");
            Log.i(TAG, "RTL-SDR error: " + errmsg);
        }
        finish();
    }
}
