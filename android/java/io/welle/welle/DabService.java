package io.welle.welle;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.text.TextUtils;

import org.qtproject.qt5.android.bindings.QtService;

import android.app.NotificationManager;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.session.MediaSession;
import android.media.session.MediaSession.Token;
import android.media.session.PlaybackState;
import android.util.Log;
import android.view.KeyEvent;

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
    
    // Bundle extras
    public static final String BUNDLE_KEY_DAB_TYPE = "DAB_TYPE";
    public static final String BUNDLE_KEY_STATION = "STATION";
    public static final String BUNDLE_KEY_CHANNEL = "CHANNEL";

    public static final String MEDIA_ID_CHANNEL_SCAN = "####";
    public static final String MEDIA_ID_ERROR = "****";
    public static final String MEDIA_ID_UNKNOWN = "__";

    public static final int TYPE_DAB_STATION = 1;
    public static final int TYPE_DAB_CHANNEL = 2;
    public static final int TYPE_DAB_CHANNEL_SCAN = 3;

    // ID for our MediaNotification.
    private static final int NOTIFICATION_ID = 416;

    // Request code for starting the UI.
    private static final int REQUEST_CODE = 99;

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

    private static List<MediaSession.QueueItem> fakeScanList = null;

    public static String toMediaId(String station, String channel) {
        if (station == null)
            station = MEDIA_ID_UNKNOWN;
        if (channel == null)
            channel = MEDIA_ID_UNKNOWN;
        return station + channel;
    }

    public static int compareStation(MediaDescription ld, MediaDescription rd) {
        String lStation = ld.getTitle().toString();
        String rStation = rd.getTitle().toString();

        int comp = lStation.compareToIgnoreCase(rStation);
        if (comp == 0) {
            String lChannel = ld.getExtras().getString(BUNDLE_KEY_CHANNEL);
            String rChannel = rd.getExtras().getString(BUNDLE_KEY_CHANNEL);
            comp = lChannel.compareToIgnoreCase(rChannel);
        }
        return comp;
    }

    public static MediaDescription createStation(String station, String channel) {
        Bundle extras = new Bundle();
        extras.putInt(BUNDLE_KEY_DAB_TYPE, TYPE_DAB_STATION);
        extras.putString(BUNDLE_KEY_STATION, station);
        extras.putString(BUNDLE_KEY_CHANNEL, channel);
        return new MediaDescription.Builder()
                .setMediaId(toMediaId(station, channel))
                .setTitle(station.trim())
                .setSubtitle(channel)
                .setExtras(extras)
                .build();
    }

    // Native

    private static DabService instance = null;

    public static native void openTcpConnection(String host, int port);
    public static native void closeTcpConnection();

    public static native boolean isFavoriteStation(String station, String channel);
    public static native void setFavoriteStation(String station, String channel, boolean value);

    public static native void play(String station, String channel);
    public static native String lastStation();

    public static native void duckPlayback(boolean duck);
    public static native void pausePlayback();
    public static native void stopPlayback();

    public static native void nextChannel();
    public static native void setManualChannel(String channelName);

    public static native void startChannelScan();
    public static native void stopChannelScan();

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

    public static void deviceClosed() {
        Log.i(TAG, "Device closed");
        if (instance == null)
            return;

        instance.handleDeviceClosed();
    }

    public static void addStation(String station, String channel) {
        Log.i(TAG, "Add station: " + station + " channel: " + channel);
        if (instance == null)
            return;

        instance.handleAddStation(station, channel);
    }

    public static void clearStations() {
        Log.i(TAG, "Clear stations");
        if (instance == null)
            return;

        instance.handleClearStations();
    }

    public static void channelScanStopped() {
        Log.i(TAG, "Channel scan stopped");
        if (instance == null)
            return;

        instance.mChannelScanProgress = -1;

        instance.updatePlaybackState();
    }

    public static void channelScanProgress(int progress) {
        Log.i(TAG, "Channel scan progress: " + progress);
        if (instance == null)
            return;

        instance.mChannelScanProgress = progress;

        instance.updatePlaybackState();
    }

    public static void updateGuiData(int status, String channel, String station, String title, String label, String type) {
        Log.i(TAG, "Update display data: status: " + status + " channel: " + channel
                + ", station: " + station + ", title: " + title + ", label: " + label
                + ", type: " + type);
        if (instance == null)
            return;

        instance.mDabStatus = status;
        instance.mCurrentStation = station.isEmpty() ? null : station;
        instance.mCurrentChannel = channel.isEmpty() ? null : channel;
        instance.mDisplayTitle = title;
        instance.mDisplaySubTitle = label;
        instance.mGenre = type;

        instance.updatePlaybackState();
    }

    public static void updateMOT(Bitmap bitmap) {
        Log.i(TAG, "Update MOT bitmap");
        if (instance == null)
            return;

        instance.mDisplayArt = bitmap;

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

    public class DabBinder extends Binder {

        boolean isDeviceAvailable() {
            return (mDabDevice != null);
        }

        Token getSessionToken() {
            return mSession.getSessionToken();
        }
    }

    private class DabDevice {
        DabDevice(String name, String host, int port) {
            this.name = name;
            this.host = host;
            this.port = port;
            this.connected = false;
        }
        String name;
        String host;
        int port;
        boolean connected;
    }

    private DabDevice mDabDevice = null;
    private MediaSession mSession = null;
    private BroadcastReceiver mDabReceiver = null;

    private int mChannelScanProgress = -1;
    private int mDabStatus = DAB_STATUS_UNKNOWN;
    private String mCurrentStation = null;
    private String mCurrentChannel = null;
    private String mDisplayTitle = null;
    private String mDisplaySubTitle = null;
    private String mGenre = null;
    private Bitmap mDisplayArt = null;

    private String mError = null;

    private boolean mServiceReady = false;
    private boolean mAudioFocus = false;

    List<MediaSession.QueueItem> mStationList = new ArrayList<>();

    private IBinder mBinder = new DabBinder();

    private void handleServiceReady() {
        Log.d(TAG, "Handle service ready");

        if (mDabDevice != null) {
            if (!mDabDevice.connected)
                openTcpConnection(mDabDevice.host, mDabDevice.port);
        }
        mServiceReady = true;
    }

    private void handleDeviceReady() {
        Log.d(TAG, "Handle device ready");
        if (mDabDevice != null) mDabDevice.connected = true;

        // Activate media session
        if (!mSession.isActive()) {
            mSession.setActive(true);
            Log.d(TAG, "media session is active");
        }

        // Open notification
        startForeground(NOTIFICATION_ID, createNotification());

        // Update state
        updatePlaybackState();

        // Play last station
        playLastStation();
    }

    private void handleDeviceClosed() {
        Log.d(TAG, "Handle device closed");

        // Release audio focus
        releaseAudioFocus();

        // Deactivate media session
        if (mSession.isActive()) {
            mSession.setActive(false);
        }

        // Close notification
        NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        notificationManager.cancel(NOTIFICATION_ID);
        stopForeground(true);

        mDabDevice = null;
        mDabStatus = DAB_STATUS_ERROR;
        mChannelScanProgress = -1;
        mCurrentStation = null;
        mCurrentChannel = null;
        mDisplayTitle = null;
        mDisplaySubTitle = null;
        mDisplayArt = null;
        mGenre = null;

        mError = getResources().getString(R.string.error_rtl_sdr_unplugged);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    private final class MediaSessionCallback extends MediaSession.Callback {

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
            handleSkipRequest(true);
        }

        @Override
        public void onSkipToPrevious() {
            Log.d(TAG, "skipToPrevious");
            handleSkipRequest(false);
        }

        @Override
        public void onSkipToQueueItem(long queueId) {
            Log.d(TAG, "OnSkipToQueueItem:" + queueId);
            if (queueId == MEDIA_ID_CHANNEL_SCAN.hashCode()) {
                handleScanStartRequest();
                return;
            }

            for (MediaSession.QueueItem queueItem: mStationList) {
                if (queueId == queueItem.getQueueId()) {
                    handlePlayRequest(queueItem.getDescription().getExtras());
                    return;
                }
            }
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
            Log.d(TAG, "onPlayFromSearch query: " + query + " extras: " + extras);
            if (query == null)
                return;

            if (TextUtils.isEmpty(query)) {
                Log.d(TAG, "onPlayFromSearch last station");
                playLastStation();
            } else {
                for (MediaSession.QueueItem queueItem: mStationList) {
                    if (query.equalsIgnoreCase(queueItem.getDescription().getExtras().getString(BUNDLE_KEY_STATION))) {
                        handlePlayRequest(queueItem.getDescription().getExtras());
                        return;
                    }
                }
            }
        }
    }

    private void handleAddStation(String station, String channel) {
        mStationList.add(new MediaSession.QueueItem(createStation(station, channel),
                toMediaId(station, channel).hashCode()));
        Collections.sort(mStationList, new Comparator<MediaSession.QueueItem>() {
            @Override
            public int compare(MediaSession.QueueItem lhs, MediaSession.QueueItem rhs) {
                return compareStation(lhs.getDescription(), rhs.getDescription());
            }
        });
        mSession.setQueue(mStationList);
        updatePlaybackState();
    }

    private void handleClearStations() {
        mStationList.clear();

        if (fakeScanList == null) {
            fakeScanList = new ArrayList<>();
            Bundle extras = new Bundle();
            extras.putInt(BUNDLE_KEY_DAB_TYPE, TYPE_DAB_CHANNEL_SCAN);
            extras.putString(BUNDLE_KEY_STATION, "");
            extras.putString(BUNDLE_KEY_CHANNEL, "");

            fakeScanList.add(new MediaSession.QueueItem(new MediaDescription.Builder()
                    .setMediaId(MEDIA_ID_CHANNEL_SCAN)
                    .setTitle(getResources().getString(R.string.menu_channel_scan))
                    .setExtras(extras)
                    .build(), MEDIA_ID_CHANNEL_SCAN.hashCode()));
        }
        mSession.setQueue(fakeScanList);

        updatePlaybackState();
    }

    private void handlePlayRequest(Bundle extras) {
        if (mDabDevice == null) {
            Intent intent = new Intent(this, DabDelegate.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
            return;
        }

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
        if (mDabDevice == null)
            return;

        Log.d(TAG, "handlePauseRequest");
        if (0 <= mChannelScanProgress)
            stopChannelScan();
        else
            pausePlayback();

        releaseAudioFocus();
    }

    private void handleStopRequest() {
        if (mDabDevice == null)
            return;

        Log.d(TAG, "handleStopRequest");
        if (0 <= mChannelScanProgress)
            stopChannelScan();
        else
            stopPlayback();

        releaseAudioFocus();
    }

    private void handleSkipRequest(boolean next) {
        if (mDabDevice == null)
            return;

        Log.d(TAG, "handleSkipRequest: " + next);
        if (mCurrentStation == null || mCurrentChannel == null || mStationList.isEmpty())
            return;

        int index = -1;
        int count = -1;
        String id = toMediaId(mCurrentStation, mCurrentChannel);
        for (MediaSession.QueueItem queueItem: mStationList) {
            ++count;
            if (id.equals(queueItem.getDescription().getMediaId())) {
                index = count;
                Log.d(TAG, "handleSkipRequest: found: " + id + " index: " + index);
                break;
            }
        }

        MediaSession.QueueItem queueItem;
        if (index < 0) {
            queueItem = mStationList.get(0);
        } else {
            if (next) {
                ++index;
                // wrap around
                if (index >= mStationList.size()) {
                    index = 0;
                }
            } else {
                --index;
                // wrap around
                if (index < 0) {
                    index = mStationList.size() - 1;
                }
            }
            queueItem = mStationList.get(index);
        }

        if (queueItem != null) {
            handlePlayRequest(queueItem.getDescription().getExtras());
        }
    }

    private void handleScanStartRequest() {
        if (mDabDevice == null)
            return;

        Log.d(TAG, "handleScanStartRequest");
        startChannelScan();
    }

    private void handleScanStopRequest() {
        if (mDabDevice == null)
            return;

        Log.d(TAG, "handleScanStopRequest");
        stopChannelScan();
    }

    private boolean handleCustomAction(String action, Bundle extras) {
        if (!mServiceReady)
            return false;

        if (CUSTOM_ACTION_PLAY.equals(action)) {
            Log.i(TAG, "handleCustomAction: play");
            handlePlayRequest(extras);
        } else if (CUSTOM_ACTION_PAUSE.equals(action)) {
            Log.i(TAG, "handleCustomAction: pause");
            handlePauseRequest();
        } else if (CUSTOM_ACTION_SKIP_NEXT.equals(action)) {
            Log.i(TAG, "handleCustomAction: skip next");
            handleSkipRequest(true);
        } else if (CUSTOM_ACTION_SKIP_PREV.equals(action)) {
            Log.i(TAG, "handleCustomAction: skip prev");
            handleSkipRequest(false);
        } else if (CUSTOM_ACTION_SCAN_START.equals(action)) {
            Log.i(TAG, "handleCustomAction: scan start");
            handleScanStartRequest();
        } else if (CUSTOM_ACTION_SCAN_STOP.equals(action)) {
            Log.i(TAG, "handleCustomAction: scan stop");
            handleScanStopRequest();
        } else if (CUSTOM_ACTION_FAVORITE.equals(action)) {
            Log.i(TAG, "handleCustomAction: favorite");

            if (mCurrentStation != null && mCurrentChannel != null) {
                setFavoriteStation(mCurrentStation, mCurrentChannel,
                        !isFavoriteStation(mCurrentStation, mCurrentChannel));
                updatePlaybackState();
            }
        } else if (CUSTOM_ACTION_NEXT_CHANNEL.equals(action)) {
            Log.i(TAG, "handleCustomAction: nex channel");
            if(mDabDevice != null) nextChannel();
        } else if (CUSTOM_ACTION_RECORD.equals(action)) {
            Log.i(TAG, "handleCustomAction: record");
        } else {
            return false;
        }
        return true;
    }

    private void playLastStation() {
        if (mDabDevice == null)
            return;

        Log.d(TAG, "playLastStation");
        String id = lastStation();

        if (id.isEmpty()) {
            if (!mStationList.isEmpty()) {
                handlePlayRequest(mStationList.get(0).getDescription().getExtras());
            }
            return;
        }

        for (MediaSession.QueueItem queueItem: mStationList) {
            if (id.equals(queueItem.getDescription().getMediaId())) {
                Log.d(TAG, "playLastStation: station " + id + " found");
                handlePlayRequest(queueItem.getDescription().getExtras());
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
        MediaMetadata.Builder metaData = new MediaMetadata.Builder();
        PlaybackState.Builder stateBuilder = new PlaybackState.Builder();

        long playbackActions = 0;

        // Update playback state
        if (!mServiceReady || DAB_STATUS_UNKNOWN == mDabStatus) {
            String error =  getResources().getString(R.string.error_not_initialised);
            stateBuilder.setErrorMessage(error);
            metaData.putString(MediaMetadata.METADATA_KEY_MEDIA_ID, MEDIA_ID_ERROR);
            metaData.putString(MediaMetadata.METADATA_KEY_TITLE, error);
            metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_TITLE, error);
        } else if (DAB_STATUS_ERROR == mDabStatus) {
            String error =  mError != null ? mError : resources.getString(R.string.error_unknown);
            stateBuilder.setErrorMessage(error);
            metaData.putString(MediaMetadata.METADATA_KEY_MEDIA_ID, MEDIA_ID_ERROR);
            metaData.putString(MediaMetadata.METADATA_KEY_TITLE, error);
            metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_TITLE, error);
        } else {
            playbackActions |= PlaybackState.ACTION_PLAY_FROM_SEARCH
                    | PlaybackState.ACTION_PLAY_FROM_MEDIA_ID
                    | PlaybackState.ACTION_SKIP_TO_QUEUE_ITEM;

            if (mCurrentStation != null)
                metaData.putString(MediaMetadata.METADATA_KEY_TITLE, mCurrentStation);
            if (mCurrentChannel != null)
                metaData.putString(MediaMetadata.METADATA_KEY_ALBUM, mCurrentChannel);
            if (mGenre != null)
                metaData.putString(MediaMetadata.METADATA_KEY_GENRE, mGenre);
            if (mDisplayArt != null)
                metaData.putBitmap(MediaMetadata.METADATA_KEY_ART, mDisplayArt);
            metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_ICON_URI,
                    "android.resource://" + "io.welle.welle/drawable/ic_radio");

            if (0 > mChannelScanProgress) {
                // Playback
                if (mStationList.size() > 1) {
                    playbackActions |= PlaybackState.ACTION_SKIP_TO_PREVIOUS
                            | PlaybackState.ACTION_SKIP_TO_NEXT;
                }

                if (mDabStatus == DAB_STATUS_PLAYING) {
                    playbackActions |= PlaybackState.ACTION_PAUSE;
                } else if (mDabStatus >= DAB_STATUS_INITIALISED) {
                    playbackActions |= PlaybackState.ACTION_PLAY;
                }

                stateBuilder.addCustomAction(CUSTOM_ACTION_SCAN_START, resources.getString(R.string.action_scan),
                        R.drawable.ic_search);

//TODO next channel                stateBuilder.addCustomAction(CUSTOM_ACTION_NEXT_CHANNEL, resources.getString(R.string.action_next_channel),
//                        R.drawable.ic_skip_up);

//TODO record                stateBuilder.addCustomAction(CUSTOM_ACTION_RECORD,
//                        resources.getString(R.string.record), R.drawable.ic_radio_button_checked);

                if (mCurrentStation != null && mCurrentChannel != null) {
                    stateBuilder.setActiveQueueItemId(toMediaId(mCurrentStation, mCurrentChannel).hashCode());
                    stateBuilder.addCustomAction(CUSTOM_ACTION_FAVORITE,
                            resources.getString(R.string.action_favorite),
                            (isFavoriteStation(mCurrentStation, mCurrentChannel)
                                    ? R.drawable.ic_favorite
                                    : R.drawable.ic_favorite_border));
                }

                metaData.putString(MediaMetadata.METADATA_KEY_MEDIA_ID, toMediaId(mCurrentStation, mCurrentChannel));
                if (mDisplayTitle != null) {
                    metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_TITLE, mDisplayTitle);
                    if (mDisplaySubTitle != null) {
                        metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_SUBTITLE, mDisplaySubTitle);
                    }
                } else if (mCurrentStation != null) {
                    metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_TITLE, mCurrentStation);
                    if (mCurrentChannel != null) {
                        metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_SUBTITLE, mCurrentChannel);
                    }
                }
            } else {
                // Scanning
                playbackActions |= PlaybackState.ACTION_PAUSE;
                stateBuilder.addCustomAction(CUSTOM_ACTION_SCAN_STOP, resources.getString(R.string.action_scan),
                        R.drawable.ic_close);

                metaData.putString(MediaMetadata.METADATA_KEY_MEDIA_ID, MEDIA_ID_CHANNEL_SCAN);
                if (mDisplayTitle != null) {
                    metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_TITLE, mDisplayTitle);
                    if (mDisplaySubTitle != null) {
                        metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_SUBTITLE, mDisplaySubTitle);
                    }
                } else {
                    metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_TITLE,
                            resources.getString(R.string.label_scanning) + " " + mChannelScanProgress);
                    if (mCurrentChannel != null) {
                        metaData.putString(MediaMetadata.METADATA_KEY_DISPLAY_SUBTITLE, mCurrentChannel);
                    }
                }
            }

            int playbackState;
            switch (mDabStatus) {
                case DAB_STATUS_ERROR: playbackState = PlaybackState.STATE_ERROR; break;
                case DAB_STATUS_INITIALISED: playbackState = PlaybackState.STATE_NONE; break;
                case DAB_STATUS_PLAYING: playbackState = PlaybackState.STATE_PLAYING; break;
                case DAB_STATUS_PAUSED: playbackState = PlaybackState.STATE_PAUSED; break;
                case DAB_STATUS_STOPPED: playbackState = PlaybackState.STATE_STOPPED; break;
                case DAB_STATUS_SCANNING: playbackState = PlaybackState.STATE_PLAYING; break;
                default: playbackState = PlaybackState.STATE_NONE;
            }

            stateBuilder.setActions(playbackActions);
            stateBuilder.setState(playbackState, PlaybackState.PLAYBACK_POSITION_UNKNOWN, 1.0f,
                    SystemClock.elapsedRealtime());
        }

        mSession.setMetadata(metaData.build());
        mSession.setPlaybackState(stateBuilder.build());

        // Update notification
        NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        notificationManager.notify(NOTIFICATION_ID, createNotification());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    private Notification createNotification() {
        Intent intent;
        Resources resources = getResources();

        // Update Notification
        Notification.Builder notificationBuilder = new Notification.Builder(this);

        int shown = 0;
        if (!mServiceReady || DAB_STATUS_UNKNOWN == mDabStatus) {
            notificationBuilder.setContentTitle(getResources().getString(R.string.error_not_initialised));
        } else if (DAB_STATUS_ERROR == mDabStatus) {
            // Error
            String error = mError != null ? mError : getResources().getString(R.string.error_unknown);
            notificationBuilder.setContentTitle(error);
        } else if (0 <= mChannelScanProgress) {
            // Scanning
            shown++;

            // Stop scan button
            intent = new Intent(CUSTOM_ACTION_SCAN_STOP);
            notificationBuilder.addAction(new Notification.Action(R.drawable.ic_close,
                    resources.getString(R.string.action_scan),
                    PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Title & Text
            if (mDisplayTitle != null) {
                notificationBuilder.setContentTitle(mDisplayTitle);
                notificationBuilder.setContentText((mDisplaySubTitle == null) ? "" : mDisplaySubTitle);
            } else {
                notificationBuilder.setContentTitle(resources.getString(R.string.label_scanning) + " " + mChannelScanProgress);
                notificationBuilder.setContentText((mCurrentChannel == null) ? "" : mCurrentChannel);
            }
        } else {
            // Playback

            // Skip prev button
            if (mStationList.size() > 1) {
                shown++;
                intent = new Intent(CUSTOM_ACTION_SKIP_PREV);
                notificationBuilder.addAction(new Notification.Action(R.drawable.ic_skip_previous,
                        resources.getString(R.string.action_skip_prev),
                        PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT)
                ));
            }

            // Play/Pause toggle button
            if (DAB_STATUS_PLAYING == mDabStatus) {
                shown++;
                intent = new Intent(CUSTOM_ACTION_PAUSE);
                notificationBuilder.addAction(new Notification.Action(R.drawable.ic_pause,
                        resources.getString(R.string.action_pause),
                        PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT)
                ));
            } else if (mDabStatus >= DAB_STATUS_INITIALISED && !mStationList.isEmpty()) {
                shown++;
                intent = new Intent(CUSTOM_ACTION_PLAY);
                notificationBuilder.addAction(new Notification.Action(R.drawable.ic_play,
                        resources.getString(R.string.action_play),
                        PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT)
                ));
            }

            // Skip next button
            if (mStationList.size() > 1) {
                shown++;
                intent = new Intent(CUSTOM_ACTION_SKIP_NEXT);
                notificationBuilder.addAction(new Notification.Action(R.drawable.ic_skip_next,
                        resources.getString(R.string.action_skip_next),
                        PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT)
                ));
            }

            // Start scan button
            shown++;
            intent = new Intent(CUSTOM_ACTION_SCAN_START);
            notificationBuilder.addAction(new Notification.Action(R.drawable.ic_search,
                    resources.getString(R.string.action_scan),
                    PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT)
            ));

            // Next channel button
//TODO next chan            intent = new Intent(CUSTOM_ACTION_NEXT_CHANNEL);
//            notificationBuilder.addAction(new Notification.Action(R.drawable.ic_skip_up,
//                    resources.getString(R.string.action_next_channel),
//                    PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT)
//            ));

            // Title & Text
            if (mDisplayTitle != null) {
                notificationBuilder.setContentTitle(mDisplayTitle);
                notificationBuilder.setContentText((mDisplaySubTitle == null) ? "" : mDisplaySubTitle);
            } else {
                notificationBuilder.setContentTitle((mCurrentStation == null) ? "" : mCurrentStation);
                notificationBuilder.setContentText((mCurrentChannel == null) ? "" : mCurrentChannel);
            }
        }

        int showInCompactView[];
        if (shown >= 3)
            showInCompactView = new int[]{0,1,2};
        else if (shown == 2)
            showInCompactView = new int[]{0,1};
        else if (shown == 1)
            showInCompactView = new int[]{0};
        else
            showInCompactView = new int[]{};

        notificationBuilder.setSmallIcon(R.drawable.ic_icon);
        if (mDisplayArt != null) notificationBuilder.setLargeIcon(mDisplayArt);
        notificationBuilder.setShowWhen(false);
        notificationBuilder.setVisibility(Notification.VISIBILITY_PUBLIC);
        notificationBuilder.setStyle(new Notification.MediaStyle()
                .setShowActionsInCompactView(showInCompactView)
                .setMediaSession(mSession.getSessionToken())
        );

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
                    closeTcpConnection();
                }
            } else if (ANDROID_AUTO_STATUS.equals(action)) {
                String status = intent.getStringExtra(ANDROID_AUTO_MEDIA_CONNECTION_STATUS);
                boolean isConnectedToCar = ANDROID_AUTO_MEDIA_CONNECTED.equals(status);
                Log.i(TAG, "AA is connected: " + isConnectedToCar);
                Bundle aa_extras = intent.getExtras();
                for (String key : aa_extras.keySet())
                    Log.i(TAG, "AA key:" + key + " connected: " + aa_extras.getString(key));
            } else if (Intent.ACTION_MEDIA_BUTTON.equals(action)) {
                KeyEvent event = intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
                Log.i(TAG, "media key event: " + event.getKeyCode());
                if (mServiceReady && event.getAction() == KeyEvent.ACTION_UP) {
                    // Check which key was pressed
                    switch (event.getKeyCode()) {
                        case KeyEvent.KEYCODE_MEDIA_PLAY:
                            handlePlayRequest(null);
                            break;
                        case KeyEvent.KEYCODE_MEDIA_PAUSE:
                            handlePauseRequest();
                            break;
                        case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                            if (mDabStatus == DAB_STATUS_PLAYING)
                                handlePauseRequest();
                            else
                                handlePlayRequest(null);
                            break;
                        case KeyEvent.KEYCODE_MEDIA_NEXT:
                            handleSkipRequest(true);
                            break;
                        case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
                            handleSkipRequest(false);
                            break;
                        default:
                            break;
                    }
                }
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
        mSession = new MediaSession(this, TAG);
        mSession.setCallback(new MediaSessionCallback());
        mSession.setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS
                | MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
        mSession.setQueueTitle(getResources().getString(R.string.menu_stations));

        Context context = getApplicationContext();

        // This is an Intent to launch the app's UI, used primarily by the ongoing notification.
        Intent intent = new Intent(context, org.qtproject.qt5.android.bindings.QtApplication.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        mSession.setSessionActivity(PendingIntent.getActivity(context, REQUEST_CODE, intent,
                PendingIntent.FLAG_UPDATE_CURRENT));

        Bundle sessionExtras = new Bundle();
        sessionExtras.putBoolean(SLOT_RESERVATION_QUEUE, true);
        sessionExtras.putBoolean(SLOT_RESERVATION_SKIP_TO_PREV, true);
        sessionExtras.putBoolean(SLOT_RESERVATION_SKIP_TO_NEXT, true);
        mSession.setExtras(sessionExtras);
        mSession.setMediaButtonReceiver(PendingIntent.getBroadcast(this, 0,
                new Intent(Intent.ACTION_MEDIA_BUTTON), PendingIntent.FLAG_UPDATE_CURRENT));

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

        // Init station list and playback state
        handleClearStations();
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
            Log.d(TAG, "SDR attached: " + name);
            mDabDevice = new DabDevice(name, SDR_ADDRESS, SDR_PORT);
            if (mServiceReady) {
                openTcpConnection(SDR_ADDRESS, SDR_PORT);
            }
        } else {
            Log.d(TAG, "onStartCommand action: " + action);
            //MediaButtonReceiver.handleIntent(mSession, intent);
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

        mSession.release();
        unregisterReceiver(mDabReceiver);
        stopForeground(true);
        instance = null;
    }
}
