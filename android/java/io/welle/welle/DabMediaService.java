package io.welle.welle;

import android.content.res.Resources;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.support.v4.media.MediaBrowserCompat;
import android.support.v4.media.MediaBrowserServiceCompat;
import android.support.v4.media.MediaDescriptionCompat;
import android.util.Log;

import io.welle.welle.DabService.DabBinder;

import java.util.ArrayList;
import java.util.List;

public class DabMediaService extends MediaBrowserServiceCompat implements ServiceConnection {

    private static final String TAG = DabMediaService.class.getSimpleName();

    private static final String MEDIA_ID_ROOT = "__ROOT__";
    private static final String MEDIA_ID_DAB_CHANNELS = "__DAB_CHANNELS__";
    private static final String MEDIA_ID_FAVORITE_STATIONS = "__FAVORITE_STATIONS__";

    public static final String[] CHANNELS = {
            // Band III
            "5A",  "5B",  "5C",  "5D",  "6A",  "6B",  "6C",  "6D",  "7A",  "7B",  "7C",  "7D",
            "8A",  "8B",  "8C",  "8D",  "9A",  "9B",  "9C",  "9D", "10A", "10B", "10C", "10D",
            "11A", "11B", "11C", "11D", "12A", "12B", "12C", "12D", "13A", "13B", "13C", "13D",
            "13E", "13F",
            // Band L
            "LA",  "LB",  "LC",  "LD",  "LE",  "LF",  "LG",  "LH",  "LI",  "LJ",  "LK",  "LL",
            "LM",  "LN",  "LO",  "LP"
    };

    private List<MediaBrowserCompat.MediaItem> mChannelList = new ArrayList<>();
    private List<MediaBrowserCompat.MediaItem> mFavoriteList = new ArrayList<>();

    /*
     * (non-Javadoc)
     * @see android.app.Service#onCreate()
     */
    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");
        bindService(new Intent(this, DabService.class), this, Context.BIND_AUTO_CREATE);

        for (String channel : CHANNELS) {
            Bundle extras = new Bundle();
            extras.putInt(DabService.BUNDLE_KEY_DAB_TYPE, DabService.TYPE_DAB_CHANNEL);
            extras.putString(DabService.BUNDLE_KEY_CHANNEL, channel);

            mChannelList.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                    .setMediaId(DabService.toMediaId(null, channel))
                    .setTitle(channel)
                    .setExtras(extras)
                    .build(), MediaBrowserCompat.MediaItem.FLAG_PLAYABLE));
        }
    }

    /**
     * (non-Javadoc)
     *
     * @see android.app.Service#onStartCommand(android.content.Intent, int, int)
     */
    @Override
    public int onStartCommand(Intent startIntent, int flags, int startId) {
        return super.onStartCommand(startIntent, flags, startId);
    }

    /**
     * (non-Javadoc)
     *
     * @see android.app.Service#onDestroy()
     */
    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
        unbindService(this);
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder binder) {
        Log.d(TAG, "onServiceConnected");
        if (binder instanceof DabBinder) {
            Log.d(TAG, "DabService connected");
            DabBinder dabBinder = (DabBinder) binder;

            // Connect RTL-SDR if needed
            if (!dabBinder.isDeviceAvailable()) {
                Log.d(TAG, "DabService connect to RTL-SDR");
                Intent intent = new Intent(this, DabDelegate.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(intent);
            }

            // Set media session
            setSessionToken(dabBinder.getSessionToken());
        }
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        Log.d(TAG, "onServiceDisconnected");
    }

    @Override
    public BrowserRoot onGetRoot(@NonNull String clientPackageName, int clientUid, Bundle rootHints) {
        return new BrowserRoot(MEDIA_ID_ROOT, null);
    }

    @Override
    public void onLoadChildren(@NonNull final String parentMediaId,
                               @NonNull final Result<List<MediaBrowserCompat.MediaItem>> result) {
        Log.d(TAG, "onLoadChildren: parentMediaId=" + parentMediaId);

        switch (parentMediaId) {
            case MEDIA_ID_ROOT:
                Resources resources = getResources();

                List<MediaBrowserCompat.MediaItem> mediaItems = new ArrayList<>();

                mediaItems.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                        .setMediaId(MEDIA_ID_DAB_CHANNELS)
                        .setTitle(resources.getString(R.string.menu_channels))
                        .build(), MediaBrowserCompat.MediaItem.FLAG_BROWSABLE));

                if (!mFavoriteList.isEmpty()) {
                    mediaItems.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                            .setMediaId(MEDIA_ID_FAVORITE_STATIONS)
                            .setTitle(resources.getString(R.string.menu_favorites))
                            .build(), MediaBrowserCompat.MediaItem.FLAG_BROWSABLE));
                }

                result.sendResult(mediaItems);
                break;

            case MEDIA_ID_DAB_CHANNELS:
                result.sendResult(mChannelList);
                break;

            case MEDIA_ID_FAVORITE_STATIONS:
                if (mFavoriteList.isEmpty()) {
                    result.sendResult(mFavoriteList);
                } else {
                    result.sendResult(null);
                }
                break;

            default:
                Log.w(TAG, "Skipping unmatched parentMediaId: " + parentMediaId);
                result.sendResult(null);
                break;
        }
    }

    @Override
    public void onSearch(@NonNull String query, Bundle extras,
                         @NonNull Result<List<MediaBrowserCompat.MediaItem>> result) {
        Log.d(TAG, "onSearch: " + query);
        List<MediaBrowserCompat.MediaItem> mediaItems = new ArrayList<>();
        //TODO add search results
        result.sendResult(mediaItems);
    }
}
