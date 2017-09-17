package io.welle.welle;

import android.content.res.Resources;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.media.browse.MediaBrowser;
import android.media.MediaDescription;
import android.service.media.MediaBrowserService;
import android.net.Uri;
import android.util.Log;

import io.welle.welle.DabService.DabBinder;

import java.util.Comparator;
import java.util.Iterator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class DabMediaService extends MediaBrowserService implements ServiceConnection {

    private static final String TAG = DabMediaService.class.getSimpleName();

    private static final String MEDIA_ID_ROOT = "__ROOT__";
    private static final String MEDIA_ID_DAB_CHANNELS = "__DAB_CHANNELS__";
    private static final String MEDIA_ID_FAVORITE_STATIONS = "__FAVORITE_STATIONS__";

    private static final String[] CHANNELS = {
            // Band III
            "5A",  "5B",  "5C",  "5D",  "6A",  "6B",  "6C",  "6D",  "7A",  "7B",  "7C",  "7D",
            "8A",  "8B",  "8C",  "8D",  "9A",  "9B",  "9C",  "9D", "10A", "10B", "10C", "10D",
            "11A", "11B", "11C", "11D", "12A", "12B", "12C", "12D", "13A", "13B", "13C", "13D",
            "13E", "13F",
            // Band L
            "LA",  "LB",  "LC",  "LD",  "LE",  "LF",  "LG",  "LH",  "LI",  "LJ",  "LK",  "LL",
            "LM",  "LN",  "LO",  "LP"
    };

    private static DabMediaService instance = null;
    private static List<MediaBrowser.MediaItem> mChannelList = new ArrayList<>();
    private static List<MediaBrowser.MediaItem> mFavoriteList = new ArrayList<>();

    public static void addFavoriteStation(String station, String channel) {
        Log.i(TAG, "Add favorite station: " + station + " channel: " + channel);
        boolean updateRoot = mFavoriteList.isEmpty();

        mFavoriteList.add(new MediaBrowser.MediaItem(DabService.createStation(station, channel),
                MediaBrowser.MediaItem.FLAG_PLAYABLE));
        Collections.sort(mFavoriteList, new Comparator<MediaBrowser.MediaItem>() {
            @Override
            public int compare(MediaBrowser.MediaItem lhs, MediaBrowser.MediaItem rhs) {
                return DabService.compareStation(lhs.getDescription(), rhs.getDescription());
            }
        });

        if (instance != null) {
            instance.notifyChildrenChanged(updateRoot ? MEDIA_ID_ROOT : MEDIA_ID_FAVORITE_STATIONS);
        }
    }

    public static void removeFavoriteStation(String station, String channel) {
        Log.i(TAG, "Remove favorite station: " + station + " channel: " + channel);

        String mediaId = DabService.toMediaId(station, channel);
        Iterator<MediaBrowser.MediaItem> it = mFavoriteList.iterator();
        while (it.hasNext()) {
            MediaBrowser.MediaItem mediaItem = it.next();
            if (mediaId.equals(mediaItem.getMediaId())) {
                it.remove();
            }
        }

        if (instance != null) {
            instance.notifyChildrenChanged(mFavoriteList.isEmpty() ? MEDIA_ID_ROOT : MEDIA_ID_FAVORITE_STATIONS);
        }
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onCreate()
     */
    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");

        instance = this;

        bindService(new Intent(this, DabService.class), this, Context.BIND_AUTO_CREATE);

        for (String channel : CHANNELS) {
            Bundle extras = new Bundle();
            extras.putInt(DabService.BUNDLE_KEY_DAB_TYPE, DabService.TYPE_DAB_CHANNEL);
            extras.putString(DabService.BUNDLE_KEY_CHANNEL, channel);

            mChannelList.add(new MediaBrowser.MediaItem(new MediaDescription.Builder()
                    .setMediaId(DabService.toMediaId(null, channel))
                    .setTitle(channel)
                    .setExtras(extras)
                    .build(), MediaBrowser.MediaItem.FLAG_PLAYABLE));
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
                               @NonNull final Result<List<MediaBrowser.MediaItem>> result) {
        Log.d(TAG, "onLoadChildren: parentMediaId=" + parentMediaId);

        switch (parentMediaId) {
            case MEDIA_ID_ROOT:
                Resources resources = getResources();

                List<MediaBrowser.MediaItem> mediaItems = new ArrayList<>();

                if (!mFavoriteList.isEmpty()) {
                    mediaItems.add(new MediaBrowser.MediaItem(new MediaDescription.Builder()
                            .setMediaId(MEDIA_ID_FAVORITE_STATIONS)
                            .setTitle(resources.getString(R.string.menu_favorites))
                            .setIconUri(Uri.parse("android.resource://" + "io.welle.welle/drawable/ic_favorites"))
                            .build(), MediaBrowser.MediaItem.FLAG_BROWSABLE));
                }

                mediaItems.add(new MediaBrowser.MediaItem(new MediaDescription.Builder()
                        .setMediaId(MEDIA_ID_DAB_CHANNELS)
                        .setTitle(resources.getString(R.string.menu_channels))
                        .setIconUri(Uri.parse("android.resource://" + "io.welle.welle/drawable/ic_antenna"))
                        .build(), MediaBrowser.MediaItem.FLAG_BROWSABLE));

                result.sendResult(mediaItems);
                break;

            case MEDIA_ID_DAB_CHANNELS:
                result.sendResult(mChannelList);
                break;

            case MEDIA_ID_FAVORITE_STATIONS:
                result.sendResult(mFavoriteList);
                break;

            default:
                Log.w(TAG, "Skipping unmatched parentMediaId: " + parentMediaId);
                result.sendResult(null);
                break;
        }
    }
}
