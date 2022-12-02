package rev.ca.rev_lib_webrtc;

public class RevWebRTCInit {

    static {
        System.loadLibrary("Rev-Lib-WebRTC");
    }

    public native String revSetTestStr(String revKey, String revVal);

    public native String revGetTestStr(String revKey);

    public native String revInitWS(String url, String revLocalId);

    public native String revInitDataChannel(String revLocalId, String revTargetId);

    public native int revSendMessage(String revTargetId, String revData);
}
