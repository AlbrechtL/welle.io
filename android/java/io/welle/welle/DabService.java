package io.welle.welle;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.hardware.usb.UsbManager;
import android.media.AudioManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.content.Intent;

import org.qtproject.qt5.android.bindings.QtService;

import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.v4.app.NotificationManagerCompat;
import android.support.v4.media.MediaBrowserCompat;
import android.support.v4.media.MediaDescriptionCompat;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaButtonReceiver;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.MediaSessionCompat.Token;
import android.support.v4.media.session.PlaybackStateCompat;
import android.support.v7.app.NotificationCompat;
import android.util.Log;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class DabService extends QtService implements AudioManager.OnAudioFocusChangeListener {

    private static final String TAG = DabService.class.getSimpleName();

    // SDR device attached
    public static final String ACTION_SDR_DEVICE_ATTACHED = "com.sdrtouch.rtlsdr.SDR_DEVICE_ATTACHED";
    public static final String ACTION_SDR_DEVICE_DETACHED = "com.sdrtouch.rtlsdr.SDR_DEVICE_DETACHED";
    public static final String EXTRA_DEVICE_NAME = "deviceName";
    public static final String SDR_ADDRESS = "127.0.0.1";
    public static final int SDR_PORT = 1234;
    public static final int SDR_SAMPLERATE = 2048000;

    // ID for our MediaNotification.
    public static final int NOTIFICATION_ID = 416;

    // DAB status
    private static final int DAB_STATUS_ERROR       = -1;
    private static final int DAB_STATUS_UNKNOWN     = 0;
    private static final int DAB_STATUS_INITIALISED = 1;
    private static final int DAB_STATUS_PLAYING     = 2;
    private static final int DAB_STATUS_PAUSED      = 3;
    private static final int DAB_STATUS_STOPPED     = 4;
    private static final int DAB_STATUS_SCANNING    = 5;

    // Custom actions
    private static final String CUSTOM_ACTION_PLAY = "io.welle.welle.PLAY";
    private static final String CUSTOM_ACTION_PAUSE = "io.welle.welle.PAUSE";
    private static final String CUSTOM_ACTION_SKIP_NEXT = "io.welle.welle.SKIP_NEXT";
    private static final String CUSTOM_ACTION_SKIP_PREV = "io.welle.welle.SKIP_PREV";
    private static final String CUSTOM_ACTION_SCAN_START = "io.welle.welle.SCAN_START";
    private static final String CUSTOM_ACTION_SCAN_STOP = "io.welle.welle.SCAN_STOP";
    private static final String CUSTOM_ACTION_FAVORITE = "io.welle.welle.FAVORITE";
    private static final String CUSTOM_ACTION_RECORD = "io.welle.welle.RECORD";
    private static final String CUSTOM_ACTION_NEXT_CHANNEL = "io.welle.welle.NEXT_CHANNEL";

    private static final int ACTION_OPEN = 1;
    private static final int ACTION_PLAY = 2;
    private static final int ACTION_PAUSE = 3;
    private static final int ACTION_SKIP_NEXT = 4;
    private static final int ACTION_SKIP_PREV = 5;
    private static final int ACTION_SCAN_START = 6;
    private static final int ACTION_SCAN_STOP = 7;
    private static final int ACTION_NEXT_CHANNEL = 8;

    // Bundle extras
    private static final String BUNDLE_KEY_DAB_TYPE = "DAB_TYPE";
    private static final String BUNDLE_KEY_STATION = "STATION_NAME";
    private static final String BUNDLE_KEY_CHANNEL = "CHANNEL";

    private static final String MEDIA_ID_CHANNEL_SCAN = "####";
    private static final String MEDIA_ID_ERROR = "****";
    private static final String MEDIA_ID_UNKNOWN = "__";

    private static final int TYPE_DAB_STATION = 1;
    private static final int TYPE_DAB_CHANNEL = 2;

    // Use these extras to reserve space for the corresponding actions, even when they are disabled
    // in the playbackstate, so the custom actions don't reflow.
    private static final String SLOT_RESERVATION_SKIP_TO_NEXT =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_SKIP_TO_NEXT";
    private static final String SLOT_RESERVATION_SKIP_TO_PREV =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_SKIP_TO_PREVIOUS";
    private static final String SLOT_RESERVATION_QUEUE =
            "com.google.android.gms.car.media.ALWAYS_RESERVE_SPACE_FOR.ACTION_QUEUE";
    private static final String ANDROID_AUTO_STATUS = "com.google.android.gms.car.media.STATUS";
    private static final String ANDROID_AUTO_MEDIA_CONNECTION_STATUS = "media_connection_status";
    private static final String ANDROID_AUTO_MEDIA_CONNECTED = "media_connected";

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

    private static List<MediaBrowserCompat.MediaItem> channelList = null;

    public static List<MediaBrowserCompat.MediaItem> getChannelList() {
        if (channelList == null) {
            channelList = new ArrayList<>();
            for (String channel : DabService.CHANNELS) {
                Bundle extras = new Bundle();
                extras.putInt(BUNDLE_KEY_DAB_TYPE, TYPE_DAB_CHANNEL);
                extras.putString(BUNDLE_KEY_CHANNEL, channel);

                channelList.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                        .setMediaId(toMediaId(null, channel))
                        .setTitle(channel)
                        .setExtras(extras)
                        .build(), MediaBrowserCompat.MediaItem.FLAG_PLAYABLE));
            }
        }
        return channelList;
    }

    public static void startDabService(Context context) {
        context.startService(new Intent(context, DabService.class));
    }

    public static String toMediaId(String station, String channel) {
        if (station == null)
            station = MEDIA_ID_UNKNOWN;
        if (channel == null)
            channel = MEDIA_ID_UNKNOWN;
        return station + channel;
    }
    
    // Native

    private static DabService instance = null;

    public static native void openTcpConnection(String host, int port);

    public static native boolean isFavoriteStation(String station, String channel);
    public static native void addFavoriteStation(String station, String channel);
    public static native void removeFavoriteStation(String station, String channel);

    public static native void play(String station, String channel);
    public static native String lastStation();

    public static native void duckPlayback(boolean duck);
    public static native void pausePlayback();
    public static native void stopPlayback();

    public static native void nextChannel();
    public static native void setManualChannel(String channelName);

    public static native void startChannelScan();
    public static native void stopChannelScan();

    public static native void saveStations();

    public static void serviceReady() {
        Log.i(TAG, "Service ready");
        if (instance == null)
            return;

        instance.handleServiceReady();
    }

    public static void deviceReady() {
        Log.i(TAG, "Device ready");
        if (instance == null)
            return;

        instance.handleDeviceReady();
    }

    public static void addStation(String station, String channel) {
        Log.i(TAG, "Add station: " + station + " channel: " + channel);
        if (instance == null)
            return;

        Bundle extras = new Bundle();
        extras.putInt(BUNDLE_KEY_DAB_TYPE, TYPE_DAB_STATION);
        extras.putString(BUNDLE_KEY_STATION, station);
        extras.putString(BUNDLE_KEY_CHANNEL, channel);

        instance.mStationList.add(new MediaBrowserCompat.MediaItem(new MediaDescriptionCompat.Builder()
                .setMediaId(toMediaId(station, channel))
                .setTitle(station.trim() + " (" + channel + ")")
                .setExtras(extras)
                .build(), MediaBrowserCompat.MediaItem.FLAG_PLAYABLE));

        Collections.sort(instance.mStationList, new Comparator<MediaBrowserCompat.MediaItem>() {
            @Override
            public int compare(MediaBrowserCompat.MediaItem lhs, MediaBrowserCompat.MediaItem rhs) {
                String lStation = lhs.getDescription().getTitle().toString();
                String rStation = rhs.getDescription().getTitle().toString();

                int comp = lStation.compareToIgnoreCase(rStation);
                //if (comp == 0) {
                //    String lChannel = lhs.getDescription().getExtras().getString(BUNDLE_KEY_CHANNEL);
                //    String rChannel = rhs.getDescription().getExtras().getString(BUNDLE_KEY_CHANNEL);
                //    comp = lChannel.compareToIgnoreCase(rChannel);
                //}
                return comp;
            }
        });

        if (instance.mDabCallback != null) {
            instance.mDabCallback.updateStationList(instance.mStationList);
        }
    }

    public static void clearStations() {
        Log.i(TAG, "Clear stations");
        if (instance == null)
            return;

        instance.mStationList.clear();
    }

    public static void channelScanStopped() {
        Log.i(TAG, "Channel scan stopped");
        if (instance == null)
            return;

        instance.mChannelScanProgress = -1;
    }

    public static void channelScanProgress(int progress) {
        Log.i(TAG, "Channel scan progress: " + progress);
        if (instance == null)
            return;

        instance.mChannelScanProgress = progress;
    }

    public static void updateGuiData(int status, String channel, String station, String title, String label, String type) {
        Log.i(TAG, "Update display data status" + status + " channel: " + channel
                + ", station=" + station + ", title=" + title + ", label=" + label
                + ", type=" + type);
        if (instance == null)
            return;

        instance.mDabStatus = status;
        instance.mCurrentStation = station;
        instance.mCurrentChannel = channel;

        instance.mTrack = new MediaMetadataCompat.Builder()
                .putString(MediaMetadataCompat.METADATA_KEY_TITLE, station)
                .putString(MediaMetadataCompat.METADATA_KEY_ALBUM, channel)
                .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, title)
                .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE, label)
                .putString(MediaMetadataCompat.METADATA_KEY_GENRE, type)
                .build();

        instance.updatePlaybackState();
    }

    public static void showErrorMessage(String text) {
        Log.i(TAG, "Show error message: " + text);
        if (instance == null)
            return;

        instance.mError = text;
        instance.mDabStatus = DAB_STATUS_ERROR;
        instance.updatePlaybackState();
    }

    public static void showInfoMessage(String text) {
        Log.i(TAG, "Show info message: " + text);
    }

    public interface DabCallback {
        public void updateStationList(List<MediaBrowserCompat.MediaItem> list);
        public void updateFavoriteList(List<MediaBrowserCompat.MediaItem> list);
        public void updateDabStatus(int playbackState);
        public void updateScanProgress(int scanProgress);
        public void showError(String error);
    }

    public class DabBinder extends Binder {

        Token getSessionToken() {
            return mSession.getSessionToken();
        }

        List<MediaBrowserCompat.MediaItem> getStationList() {
            return mStationList;
        }

        List<MediaBrowserCompat.MediaItem> getFavoriteList() {
            return mFavoriteList;
        }

        void setDabCallback(DabCallback callback) {
            Log.i(TAG, "DabCallback set");
            mDabCallback = callback;
            initDabCallback();
        }
    }

    private class DabDevice {
        public DabDevice(String name, String host, int port) {
            this.name = name;
            this.host = host;
            this.port = port;
            this.connected = false;
        }
        public String name;
        public String host;
        public int port;
        public boolean connected;
    }

    private DabDevice mDabDevice = null;
    private DabCallback mDabCallback = null;
    private MediaSessionCompat mSession = null;
    private MediaMetadataCompat mTrack = null;
    private NotificationManagerCompat mNotificationManager = null;
    private BroadcastReceiver mDabReceiver = null;

    private int mChannelScanProgress = -1;
    private int mDabStatus = DAB_STATUS_UNKNOWN;
    private String mCurrentStation = null;
    private String mCurrentChannel = null;
    private String mError = null;

    private boolean mServiceReady = false;
    private boolean mAudioFocus = false;

    List<MediaBrowserCompat.MediaItem> mStationList = new ArrayList<>();
    List<MediaBrowserCompat.MediaItem> mFavoriteList = new ArrayList<>();

    private IBinder mBinder = new DabBinder();

    private void handleServiceReady() {
        Log.d(TAG, "Handle service ready");
        mServiceReady = true;

        if (mDabDevice != null) {
            if (!mDabDevice.connected)
                openTcpConnection(mDabDevice.host, mDabDevice.port);
        }
    }

    private void handleDeviceReady() {
        Log.d(TAG, "Handle device ready");
        if (mDabDevice != null) mDabDevice.connected = true;

        // Inform others
        initDabCallback();

        // Play last station
        playLastStation();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    private final class MediaSessionCallback extends MediaSessionCompat.Callback {

        @Override
        public void onPlayFromMediaId(String mediaId, Bundle extras) {
            Log.d(TAG, "playFromMediaId mediaId:" + mediaId + "  extras=" + extras);
            handlePlayRequest(extras);
        }

        @Override
        public void onPlay() {
            Log.d(TAG, "play");
            handlePlayRequest(null);
        }

        @Override
        public void onPause() {
            Log.d(TAG, "pause");
            handlePauseRequest();
        }

        @Override
        public void onStop() {
            Log.d(TAG, "stop");
            handleStopRequest();
        }

        @Override
        public void onSkipToNext() {
            Log.d(TAG, "skipToNext");
            handleSkipRequest(1);
        }

        @Override
        public void onSkipToPrevious() {
            Log.d(TAG, "skipToPrevious");
            handleSkipRequest(-1);
        }

        @Override
        public void onSkipToQueueItem(long queueId) {
            Log.d(TAG, "OnSkipToQueueItem:" + queueId);
        }

        @Override
        public void onSeekTo(long position) {
            Log.d(TAG, "onSeekTo:" + position);
        }

        @Override
        public void onCustomAction(@NonNull String action, Bundle extras) {
            if (!handleCustomAction(action, extras))
                Log.d(TAG, "onCustomAction unsupported action: " + action);
        }

        @Override
        public void onPlayFromSearch(final String query, final Bundle extras) {
            Log.d(TAG, "playFromSearch query=" + query + " extras=" + extras);
        }
    }

    private void handlePlayRequest(Bundle extras) {
        Log.d(TAG, "handlePlayRequest");

        if (!requestAudioFocus()) {
            Log.d(TAG, "request audio focus failed!");
            return;
        }

        if (!mSession.isActive()) {
            mSession.setActive(true);
        }

        if (extras == null) {
            Log.d(TAG, "handlePlayRequest: last station");
            if (mCurrentStation != null && mCurrentChannel != null) {
                play(mCurrentStation, mCurrentChannel);
            } else {
                playLastStation();
            }
        } else {
            int dabType = extras.getInt(BUNDLE_KEY_DAB_TYPE);
            String channel = extras.getString(BUNDLE_KEY_CHANNEL);

            if (dabType == TYPE_DAB_STATION) {
                String station = extras.getString(BUNDLE_KEY_STATION);
                Log.d(TAG, "handlePlayRequest: station: " + station + " channel: " + channel);
                play(station, channel);
            } else if (dabType == TYPE_DAB_CHANNEL) {
                Log.d(TAG, "handlePlayRequest: channel: " + channel);
                setManualChannel(channel);
            }
        }
    }

    private void handlePauseRequest() {
        Log.d(TAG, "handlePauseRequest");
        if (0 <= mChannelScanProgress)
            stopChannelScan();
        else
            pausePlayback();

        releaseAudioFocus();
    }

    private void handleStopRequest() {
        Log.d(TAG, "handleStopRequest");
        if (0 <= mChannelScanProgress)
            stopChannelScan();
        else
            stopPlayback();

        releaseAudioFocus();
    }

    private void handleSkipRequest(int n) {
        Log.d(TAG, "handleSkipRequest: " + n);
        if (mCurrentStation == null || mCurrentChannel == null || mStationList.isEmpty())
            return;

        int index = -1;
        int count = -1;
        String id = toMediaId(mCurrentStation, mCurrentChannel);
        for (MediaBrowserCompat.MediaItem mediaItem: mStationList) {
            ++count;
            if (mediaItem.getMediaId().equals(id)) {
                index = count;
                Log.d(TAG, "handleSkipRequest: found: " + id + " index: " + count);
                break;
            }
        }

        MediaBrowserCompat.MediaItem mediaItem = null;
        if (index < 0) {
            mediaItem = mStationList.get(0);
        } else {
            index += n;
            if (index < 0) {
                index = 0;
            } else if (index >= mStationList.size()) {
                index = index % mStationList.size();
            }
            mediaItem = mStationList.get(index);
        }

        if (mediaItem != null) {
            handlePlayRequest(mediaItem.getDescription().getExtras());
        }
    }

    private void handleScanStartRequest() {
        Log.d(TAG, "handleScanStartRequest");
        startChannelScan();
    }

    private void handleScanStopRequest() {
        Log.d(TAG, "handleScanStopRequest");
        stopChannelScan();
    }

    private boolean handleCustomAction(String action, Bundle extras) {
        if (CUSTOM_ACTION_PLAY.equals(action)) {
            Log.i(TAG, "handleCustomAction: play");
            handlePlayRequest(extras);
        } else if (CUSTOM_ACTION_PAUSE.equals(action)) {
            Log.i(TAG, "handleCustomAction: pause");
            handlePauseRequest();
        } else if (CUSTOM_ACTION_SKIP_NEXT.equals(action)) {
            Log.i(TAG, "handleCustomAction: skip next");
            handleSkipRequest(1);
        } else if (CUSTOM_ACTION_SKIP_PREV.equals(action)) {
            Log.i(TAG, "handleCustomAction: skip prev");
            handleSkipRequest(-1);
        } else if (CUSTOM_ACTION_SCAN_START.equals(action)) {
            Log.i(TAG, "handleCustomAction: scan start");
            handleScanStartRequest();
        } else if (CUSTOM_ACTION_SCAN_STOP.equals(action)) {
            Log.i(TAG, "handleCustomAction: scan stop");
            handleScanStopRequest();
        } else if (CUSTOM_ACTION_FAVORITE.equals(action)) {
            Log.i(TAG, "handleCustomAction: favorite");

            if (mCurrentStation != null && mCurrentChannel != null) {
                if (isFavoriteStation(mCurrentStation, mCurrentChannel))
                    removeFavoriteStation(mCurrentStation, mCurrentChannel);
                else
                    addFavoriteStation(mCurrentStation, mCurrentChannel);
                updatePlaybackState();
            }
        } else if (CUSTOM_ACTION_NEXT_CHANNEL.equals(action)) {
            Log.i(TAG, "handleCustomAction: nex channel");
            nextChannel();
        } else if (CUSTOM_ACTION_RECORD.equals(action)) {
            Log.i(TAG, "handleCustomAction: record");
        } else {
            return false;
        }
        return true;
    }

    private void playLastStation() {
        Log.d(TAG, "playLastStation");
        String id = lastStation();

        if (id.isEmpty()) {
            if (mStationList.isEmpty())
                return;
            Bundle extras = mStationList.get(0).getDescription().getExtras();
            handlePlayRequest(extras);
            return;
        }

        for (MediaBrowserCompat.MediaItem mediaItem: mStationList) {
            if (mediaItem.getMediaId().equals(id)) {
                Log.d(TAG, "playLastStation: station " + id + " found");
                handlePlayRequest(mediaItem.getDescription().getExtras());
                return;
            }
        }

        int len = id.length();
        if (len < 3)
            return;
        String station = id.substring(0, (len - 2));
        String channel = id.substring((len - 2));

        Bundle extras = new Bundle();
        extras.putInt(BUNDLE_KEY_DAB_TYPE, TYPE_DAB_STATION);
        extras.putString(BUNDLE_KEY_STATION, station);
        extras.putString(BUNDLE_KEY_CHANNEL, channel);

        handlePlayRequest(extras);
    }

    private void updatePlaybackState() {
        Log.d(TAG, "updatePlaybackState");

        Resources resources = getResources();
        PlaybackStateCompat.Builder stateBuilder = new PlaybackStateCompat.Builder();

        long playbackActions = 0;

        // Update playback state
        if (DAB_STATUS_ERROR == mDabStatus) {
            String error =  mError != null ? mError : resources.getString(R.string.error_unknown);
            stateBuilder.setErrorMessage(-1, error);
        } else if (DAB_STATUS_UNKNOWN == mDabStatus) {
            String error =  getResources().getString(R.string.error_not_initialised);
            stateBuilder.setErrorMessage(-1, error);
        } else {
            playbackActions |= PlaybackStateCompat.ACTION_PLAY_FROM_MEDIA_ID;
            //TODO playbackActions |= PlaybackStateCompat.ACTION_PLAY_FROM_SEARCH;

            if (0 > mChannelScanProgress) {
                // Playback
                playbackActions |= PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS |
                        PlaybackStateCompat.ACTION_SKIP_TO_NEXT;

                if (mDabStatus == DAB_STATUS_PLAYING) {
                    playbackActions |= PlaybackStateCompat.ACTION_PAUSE;
                } else {
                    playbackActions |= PlaybackStateCompat.ACTION_PLAY;
                }

                stateBuilder.addCustomAction(CUSTOM_ACTION_SCAN_START, resources.getString(R.string.action_scan),
                        android.R.drawable.ic_menu_search);

                stateBuilder.addCustomAction(CUSTOM_ACTION_NEXT_CHANNEL, resources.getString(R.string.action_next_channel),
                        android.R.drawable.ic_menu_upload); //ic_menu_rotate

//TODO record                stateBuilder.addCustomAction(CUSTOM_ACTION_RECORD,
//                        resources.getString(R.string.record), android.R.drawable.ic_menu_add);

                // Set the activeQueueItemId if the current index is valid.
                if (mCurrentStation != null && mCurrentChannel != null) {
                    //stateBuilder.setActiveQueueItemId(mCurrentMedia.getQueueId());
//TODO fav                    stateBuilder.addCustomAction(CUSTOM_ACTION_FAVORITE,
//                            resources.getString(R.string.action_favorite),
//                            (isFavoriteStation(mCurrentStation, mCurrentChannel)
//                                    ? android.R.drawable.star_on
//                                    : android.R.drawable.star_off));
                }
            } else {
                // Scanning
                playbackActions |= PlaybackStateCompat.ACTION_PAUSE;
                stateBuilder.addCustomAction(CUSTOM_ACTION_SCAN_STOP, resources.getString(R.string.action_scan),
                        android.R.drawable.ic_menu_close_clear_cancel);
            }

            int playbackState;
            switch (mDabStatus) {
                case DAB_STATUS_ERROR: playbackState = PlaybackStateCompat.STATE_ERROR; break;
                case DAB_STATUS_INITIALISED: playbackState = PlaybackStateCompat.STATE_NONE; break;
                case DAB_STATUS_PLAYING: playbackState = PlaybackStateCompat.STATE_PLAYING; break;
                case DAB_STATUS_PAUSED: playbackState = PlaybackStateCompat.STATE_PAUSED; break;
                case DAB_STATUS_STOPPED: playbackState = PlaybackStateCompat.STATE_STOPPED; break;
                case DAB_STATUS_SCANNING: playbackState = PlaybackStateCompat.STATE_PLAYING; break;
                default: playbackState = PlaybackStateCompat.STATE_NONE;
            }

            stateBuilder.setActions(playbackActions);
            stateBuilder.setState(playbackState, PlaybackStateCompat.PLAYBACK_POSITION_UNKNOWN, 1.0f,
                    SystemClock.elapsedRealtime());
        }

        mSession.setPlaybackState(stateBuilder.build());

        // Update meta data
        updateMetadata();
    }

    private void updateMetadata() {
        if (DAB_STATUS_ERROR == mDabStatus) {
            String error =  mError != null ? mError  : getResources().getString(R.string.error_unknown);
            MediaMetadataCompat track = new MediaMetadataCompat.Builder()
                    .putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, MEDIA_ID_ERROR)
                    .putString(MediaMetadataCompat.METADATA_KEY_TITLE, error)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, error)
                    .build();

            mSession.setMetadata(track);
        } else if (DAB_STATUS_UNKNOWN == mDabStatus) {
            String error =  getResources().getString(R.string.error_not_initialised);
            MediaMetadataCompat track = new MediaMetadataCompat.Builder()
                    .putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, MEDIA_ID_ERROR)
                    .putString(MediaMetadataCompat.METADATA_KEY_TITLE, error)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, error)
                    .build();

            mSession.setMetadata(track);
        } else if (mTrack != null) {
            mSession.setMetadata(mTrack);
        } else if (0 <= mChannelScanProgress) {
            Resources resources = getResources();
            String title = resources.getString(R.string.label_scanning) + " " + mChannelScanProgress;
            String subTitle = (mCurrentChannel == null) ? "" : mCurrentChannel;

            MediaMetadataCompat track = new MediaMetadataCompat.Builder()
                    .putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, MEDIA_ID_CHANNEL_SCAN)
                    .putString(MediaMetadataCompat.METADATA_KEY_TITLE, title)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, title)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE, subTitle)
                    .build();

            mSession.setMetadata(track);
        } else if (mCurrentStation != null) {
            String subTitle = (mCurrentChannel == null) ? "" : mCurrentChannel;

            MediaMetadataCompat track = new MediaMetadataCompat.Builder()
                    .putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, toMediaId(mCurrentStation, mCurrentChannel))
                    .putString(MediaMetadataCompat.METADATA_KEY_TITLE, mCurrentStation)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, mCurrentStation)
                    .putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE, subTitle)
                    .build();

            mSession.setMetadata(track);
        }

        // Update notification
        mNotificationManager.notify(NOTIFICATION_ID, createNotification());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    private Notification createNotification() {
        Resources resources = getResources();

        // Update Notification
        NotificationCompat.Builder notificationBuilder = new NotificationCompat.Builder(this);

        if (DAB_STATUS_ERROR == mDabStatus) {
            // Error
            String error = mError != null ? mError : getResources().getString(R.string.error_unknown);
            notificationBuilder.setStyle(new NotificationCompat.MediaStyle());
            notificationBuilder.setContentTitle(error);
        } else if (DAB_STATUS_UNKNOWN == mDabStatus) {
            notificationBuilder.setStyle(new NotificationCompat.MediaStyle());
            notificationBuilder.setContentTitle(getResources().getString(R.string.error_not_initialised));
        } else if (0 <= mChannelScanProgress) {
            // Scanning
            notificationBuilder.setStyle(new NotificationCompat.MediaStyle().setShowActionsInCompactView(new int[]{0}));

            // Stop scan button
            Intent intent = new Intent(CUSTOM_ACTION_SCAN_STOP);
            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_menu_close_clear_cancel,
                    resources.getString(R.string.action_scan),
                    PendingIntent.getBroadcast(this, ACTION_SCAN_STOP, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Title & Text
            if (mTrack != null) {
                notificationBuilder.setContentTitle(mTrack.getText(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE));
                notificationBuilder.setContentText(mTrack.getText(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE));
            } else {
                notificationBuilder.setContentTitle(resources.getString(R.string.label_scanning) + " " + mChannelScanProgress);
                notificationBuilder.setContentText((mCurrentChannel == null) ? "" : mCurrentChannel);
            }
        } else {
            // Playback
            notificationBuilder.setStyle(new NotificationCompat.MediaStyle().setShowActionsInCompactView(new int[]{0,1,2}));
            Intent intent;

            // Skip prev button
            intent = new Intent(CUSTOM_ACTION_SKIP_PREV);
            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_media_previous,
                    resources.getString(R.string.action_skip_prev),
                    PendingIntent.getBroadcast(this, ACTION_SKIP_PREV, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Play/Pause toggle button
            if (DAB_STATUS_PLAYING == mDabStatus) {
                intent = new Intent(CUSTOM_ACTION_PAUSE);
                notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_media_pause,
                        resources.getString(R.string.action_pause),
                        PendingIntent.getBroadcast(this, ACTION_PLAY, intent, PendingIntent.FLAG_UPDATE_CURRENT)
                ));
            } else {
                intent = new Intent(CUSTOM_ACTION_PLAY);
                notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_media_play,
                        resources.getString(R.string.action_play),
                        PendingIntent.getBroadcast(this, ACTION_PAUSE, intent, PendingIntent.FLAG_UPDATE_CURRENT)
                ));
            }

            // Skip next button
            intent = new Intent(CUSTOM_ACTION_SKIP_NEXT);
            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_media_next,
                    resources.getString(R.string.action_skip_next),
                    PendingIntent.getBroadcast(this, ACTION_SKIP_NEXT, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Start scan button
            intent = new Intent(CUSTOM_ACTION_SCAN_START);
            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_menu_search,
                    resources.getString(R.string.action_scan),
                    PendingIntent.getBroadcast(this, ACTION_SCAN_START, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Next channel button
//TODO next chan            intent = new Intent(CUSTOM_ACTION_NEXT_CHANNEL);
//            notificationBuilder.addAction(new NotificationCompat.Action(android.R.drawable.ic_menu_rotate,
//                    resources.getString(R.string.action_next_channel),
//                    PendingIntent.getBroadcast(this, ACTION_NEXT_CHANNEL, intent, PendingIntent.FLAG_UPDATE_CURRENT)
//            ));

            // Title & Text
            if (mTrack != null) {
                notificationBuilder.setContentTitle(mTrack.getText(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE));
                notificationBuilder.setContentText(mTrack.getText(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE));
            } else {
                notificationBuilder.setContentTitle((mCurrentStation == null) ? "" : mCurrentStation);
                notificationBuilder.setContentText((mCurrentChannel == null) ? "" : mCurrentChannel);
            }
        }

        notificationBuilder.setSmallIcon(R.drawable.icon);
        //notificationBuilder.setLargeIcon(aBitmap);
        notificationBuilder.setShowWhen(false);
        notificationBuilder.setVisibility(NotificationCompat.VISIBILITY_PUBLIC);

        return notificationBuilder.build();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    private boolean requestAudioFocus() {
        if (mAudioFocus) {
            Log.d(TAG, "requestAudioFocus AudioFocus already granted");
            return true;
        }
        Log.d(TAG, "requestAudioFocus");
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        int result = audioManager.requestAudioFocus(this, AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN);
        mAudioFocus = (result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED);
        return mAudioFocus;
    }

    private void releaseAudioFocus() {
        if (!mAudioFocus)
            return;
        Log.d(TAG, "releaseAudioFocus");
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if (audioManager.abandonAudioFocus(this) == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            mAudioFocus = false;
        }
    }

    /**
     * Called by AudioManager on audio focus changes.
     * Implementation of {@link android.media.AudioManager.OnAudioFocusChangeListener}.
     */
    @Override
    public void onAudioFocusChange(int focusChange) {
        if (focusChange == AudioManager.AUDIOFOCUS_LOSS) {
            Log.d(TAG, "AUDIOFOCUS_LOSS");
            // Permanent loss of audio focus
            // Pause playback immediately
            mAudioFocus = false;
            duckPlayback(false);
            handlePauseRequest();
            //TODO Wait 30 seconds before stopping playback
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
            Log.d(TAG, "AUDIOFOCUS_LOSS_TRANSIENT");
            // Pause playback
            mAudioFocus = false;
            duckPlayback(false);
            handlePauseRequest();
        } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK) {
            Log.d(TAG, "AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK");
            // Lower the volume, keep playing
            mAudioFocus = true;
            duckPlayback(true);
        } else if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
            Log.d(TAG, "AUDIOFOCUS_GAIN");
            // Your app has been granted audio focus again
            // Raise volume to normal, restart playback if necessary
            mAudioFocus = true;
            if (mDabStatus != DAB_STATUS_PLAYING)
                handlePlayRequest(null);
            duckPlayback(false);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    public final class DabReceiver extends BroadcastReceiver {
        public DabReceiver() {
            super();
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.i(TAG, "onReceive global: " + action);

            if (ACTION_SDR_DEVICE_DETACHED.equals(action)) {
                String name = intent.getStringExtra(EXTRA_DEVICE_NAME);
                Log.d(TAG, "SDR detached: " + name);
                if (mDabDevice != null && mDabDevice.name.equals(name)) {
                    Log.i(TAG, "SDR closed");
                    mDabDevice = null;
                    mChannelScanProgress = -1;
                    mTrack = null;
                    mCurrentStation = null;
                    mCurrentChannel = null;
                    mDabStatus = DAB_STATUS_ERROR;
                    mError = getResources().getString(R.string.error_rtl_sdr_unplugged);
                    updatePlaybackState();
                    stopForeground(true);
                }
            } else if (ANDROID_AUTO_STATUS.equals(action)) {
                String status = intent.getStringExtra(ANDROID_AUTO_MEDIA_CONNECTION_STATUS);
                boolean isConnectedToCar = ANDROID_AUTO_MEDIA_CONNECTED.equals(status);
                Log.i(TAG, "AA is connected: " + isConnectedToCar);
                Bundle aa_extras = intent.getExtras();
                for (String key : aa_extras.keySet())
                    Log.i(TAG, "AA key:" + key + " connected: " + aa_extras.getString(key));
            } else {
                handleCustomAction(action, intent.getExtras());
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    /** Called when the service is being created. */
    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "Service created");

        instance = this;

        // Start a new MediaSession.
        mSession = new MediaSessionCompat(this, TAG);
        mSession.setCallback(new MediaSessionCallback());
        mSession.setFlags(MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS | MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS);

        Context context = getApplicationContext();

        // This is an Intent to launch the app's UI, used primarily by the ongoing notification.
//TODO        Intent intent = new Intent(context, DabDelegate.class);
//        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
//        PendingIntent pi = PendingIntent.getActivity(context, ACTION_OPEN, intent,
//                PendingIntent.FLAG_UPDATE_CURRENT);
//        mSession.setSessionActivity(pi);

        Bundle sessionExtras = new Bundle();
        sessionExtras.putBoolean(SLOT_RESERVATION_QUEUE, true);
        sessionExtras.putBoolean(SLOT_RESERVATION_SKIP_TO_PREV, true);
        sessionExtras.putBoolean(SLOT_RESERVATION_SKIP_TO_NEXT, true);
        mSession.setExtras(sessionExtras);

        if (!mSession.isActive()) {
            mSession.setActive(true);
            Log.d(TAG, "media session is active");
        }

        // Register receiver
        mDabReceiver = new DabReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(ACTION_SDR_DEVICE_DETACHED);
        filter.addAction(Intent.ACTION_MEDIA_BUTTON);
        filter.addAction(CUSTOM_ACTION_PLAY);
        filter.addAction(CUSTOM_ACTION_PAUSE);
        filter.addAction(CUSTOM_ACTION_SKIP_NEXT);
        filter.addAction(CUSTOM_ACTION_SKIP_PREV);
        filter.addAction(CUSTOM_ACTION_SCAN_START);
        filter.addAction(CUSTOM_ACTION_SCAN_STOP);
        filter.addAction(CUSTOM_ACTION_FAVORITE);
        filter.addAction(CUSTOM_ACTION_RECORD);
        filter.addAction(CUSTOM_ACTION_NEXT_CHANNEL);
        registerReceiver(mDabReceiver,filter);

        // Notification
        mNotificationManager = NotificationManagerCompat.from(this);
        startForeground(NOTIFICATION_ID, createNotification());

        // Playback state
        updatePlaybackState();
    }

    /** The service is starting, due to a call to startService() */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        Log.i(TAG, "Service started, startId: " + startId);
        if (intent == null)
            return Service.START_STICKY;
        
        String action = intent.getAction();
        if (action == null)
            return Service.START_STICKY;
        
        if (ACTION_SDR_DEVICE_ATTACHED.equals(action)) {
            String name = intent.getStringExtra(EXTRA_DEVICE_NAME);
            String host = SDR_ADDRESS;
            int port = SDR_PORT;

            Log.d(TAG, "SDR attached: " + host + ":" + port);

            if (host != null && port > 0) {
                mDabDevice = new DabDevice(name, host, port);
                openTcpConnection(host, port);
            }
        } else {
            Log.d(TAG, "onStartCommand action: " + action);
            MediaButtonReceiver.handleIntent(mSession, intent);
        }
        return Service.START_STICKY;
    }

    /** A client is binding to the service with bindService() */
    @Override
    public IBinder onBind(Intent intent) {
        super.onBind(intent);
        Log.i(TAG, "Service bound");
        return mBinder;
    }


    /** Called when all clients have unbound with unbindService() */
    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }

    /** Called when a client is binding to the service with bindService()*/
    @Override
    public void onRebind(Intent intent) {
        super.onRebind(intent);
    }

    /** Called when The service is no longer used and is being destroyed */
    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "Service destroyed");

        unregisterReceiver(mDabReceiver);
        stopForeground(true);
    }

    private void initDabCallback() {
        if (mDabCallback == null)
            return;
        mDabCallback.updateStationList(mStationList);
        mDabCallback.updateFavoriteList(mFavoriteList);
        mDabCallback.updateDabStatus(mDabStatus);
        mDabCallback.updateScanProgress(mChannelScanProgress);
        if (DAB_STATUS_ERROR == mDabStatus)
            mDabCallback.showError(mError);
    }
}
