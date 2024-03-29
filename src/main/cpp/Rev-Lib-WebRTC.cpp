//
// Created by rev on 12/30/22.
//

#include "Rev-Lib-WebRTC.hpp"

#include <jni.h>
#include <android/log.h>

#include <rtc/rtc.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <iostream>
#include <sstream>
#include <memory>

#include "rev_client/rev_client_init.hpp"

using namespace std::chrono_literals;
using std::shared_ptr;
using std::weak_ptr;

JavaVM *gJvm = nullptr;
static jobject gClassLoader;
static jmethodID gFindClassMethod;

shared_ptr<rtc::WebSocket> revWS = std::make_shared<rtc::WebSocket>();

JNIEnv *getEnv() {
    JNIEnv *env;

    int status = gJvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0) {
        status = gJvm->AttachCurrentThread(&env, NULL);
        if (status < 0) {
            return nullptr;
        }
    }
    return env;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *pjvm, void *reserved) {
    __android_log_print(ANDROID_LOG_ERROR, "MyApp", ">>> Rev-Lib-WebRTC >>> JNI_OnLoad <<<");

    gJvm = pjvm;  // cache the JavaVM pointer
    auto env = getEnv();

    //replace with one of your classes in the line below
    auto randomClass = env->FindClass("rev/ca/rev_lib_webrtc/RevReactJNIData");

    jclass classClass = env->GetObjectClass(randomClass);

    if (classClass == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "MyApp", ">>> classClass - Class NOT Found!");
        return JNI_VERSION_1_6;
    }

    auto classLoaderClass = env->FindClass("java/lang/ClassLoader");

    if (classLoaderClass == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "MyApp", ">>> classLoaderClass - Class NOT Found!");
        return JNI_VERSION_1_6;
    }

    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");

    if (getClassLoaderMethod == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, "MyApp", ">>> getClassLoaderMethod - METHOD NOT Found!");
        return JNI_VERSION_1_6;
    }

    jobject tempGClassLoader = env->CallObjectMethod(randomClass, getClassLoaderMethod);
    gClassLoader = (jclass) env->NewGlobalRef(tempGClassLoader);

    gFindClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    return JNI_VERSION_1_6;
}

jclass findClass(const char *name) {
    __android_log_print(ANDROID_LOG_ERROR, "MyApp", ">>> Find Class : %s", name);

    return static_cast<jclass>(getEnv()->CallObjectMethod(gClassLoader, gFindClassMethod, getEnv()->NewStringUTF(name)));
}

void revWebServerConnectedCallBack(char *revData) {
    __android_log_print(ANDROID_LOG_ERROR, "MyApp", ">>> revData : %s", revData);

    revInitNativeEvent("revWebServerConnected", revData);
}

extern "C"
JNIEXPORT void JNICALL
Java_rev_ca_rev_1lib_1webrtc_RevWebRTCInit_revInitWS(JNIEnv *env, jobject thiz, jstring _url, jstring rev_local_id) {
    const char *url = env->GetStringUTFChars(_url, 0);
    const char *revLocalId = env->GetStringUTFChars(rev_local_id, 0);

    revInitWS(revWS, url, revLocalId, revWebServerConnectedCallBack);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_rev_ca_rev_1lib_1webrtc_RevWebRTCInit_revInitDataChannel(JNIEnv *env, jobject thiz, jstring rev_local_id, jstring rev_target_id) {
    const char *localId = env->GetStringUTFChars(rev_local_id, 0);
    const char *revTargetId = env->GetStringUTFChars(rev_target_id, 0);

    std::shared_ptr<rtc::DataChannel> revDataChannel = revInitDataChannel(localId, revTargetId);

    std::stringstream ss;
    ss << &revDataChannel;
    std::string pStr = ss.str();

    return env->NewStringUTF(pStr.c_str());
}


extern "C"
JNIEXPORT jint JNICALL
Java_rev_ca_rev_1lib_1webrtc_RevWebRTCInit_revSendMessage(JNIEnv *env, jobject thiz, jstring rev_target_id, jstring rev_data) {
    const char *revTargetId = env->GetStringUTFChars(rev_target_id, 0);
    const char *revData = env->GetStringUTFChars(rev_data, 0);

    return revSendMessage(revTargetId, revData);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_rev_ca_rev_1lib_1webrtc_RevWebRTCInit_revSetTestStr(JNIEnv *env, jobject thiz, jstring rev_key, jstring rev_val) {
    const char *revKey = env->GetStringUTFChars(rev_key, 0);
    const char *revVal = env->GetStringUTFChars(rev_val, 0);

    std::string revRetVal = revSetTestStr(revKey, revVal);

    return env->NewStringUTF(revRetVal.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_rev_ca_rev_1lib_1webrtc_RevWebRTCInit_revGetTestStr(JNIEnv *env, jobject thiz, jstring rev_key) {
    const char *revKey = env->GetStringUTFChars(rev_key, 0);

    std::string revRet = revGetTestStr(revKey);

    return env->NewStringUTF(revRet.c_str());
}

void revInitNativeEvent(std::string revEventName, std::string revData) {
    JNIEnv *g_env = getEnv();

    jstring revJEventName = g_env->NewStringUTF(revEventName.c_str());
    jstring revJEventData = g_env->NewStringUTF(revData.c_str());

    jclass RevWebRTCEventsReactModule = findClass("com/owki/rev_react_modules/rev_web_rtc/RevWebRTCEventsReactModule");

    if (RevWebRTCEventsReactModule == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "MyApp", ">>> Class NOT Found!");
        return;
    }

    jmethodID revInitNativeEvent = g_env->GetStaticMethodID(RevWebRTCEventsReactModule, "revInitNativeEvent", "(Ljava/lang/String;Ljava/lang/String;)V");

    if (revInitNativeEvent == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, "MyApp", ">>> METHOD NOT Found!");
        return;
    }

    g_env->CallStaticVoidMethod(RevWebRTCEventsReactModule, revInitNativeEvent, revJEventName, revJEventData);

    if (g_env->ExceptionCheck()) {
        g_env->ExceptionDescribe();
    }

    gJvm->DetachCurrentThread();
}

extern "C"
JNIEXPORT jint JNICALL
Java_rev_ca_rev_1lib_1webrtc_RevWebRTCInit_revWebRTCLogIn(JNIEnv *env, jobject thiz, jstring rev_target_id, jstring rev_message) {
    const char *revTargetId = env->GetStringUTFChars(rev_target_id, 0);
    const char *revMessage = env->GetStringUTFChars(rev_message, 0);

    int revLoggedInState = (int) revWebRTCLogIn(revWS, revTargetId, revMessage);

    return revLoggedInState;
}