/*
 * Kwaak3 - Java to quake3 interface
 * Copyright (C) 2010 Roderick Colenbrander
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <android/log.h>

#include "org_kwaak3_KwaakJNI.h"

enum keys
{
    QK_LEFT  = 134,
    QK_RIGHT = 135,
    QK_UP    = 132,
    QK_DOWN  = 133,
};
#define TOUCH_RANGE 1.09956// PI * 0.35
#define M_PI_X2 6.28319 // PI * 2

#define DOWN_MIN  (M_PI_X2 - TOUCH_RANGE)
#define DOWN_MAX  (TOUCH_RANGE)
#define UP_MIN    (M_PI - TOUCH_RANGE)
#define UP_MAX    (M_PI + TOUCH_RANGE)
#define LEFT_MIN  ((M_PI_2*3) - TOUCH_RANGE)
#define LEFT_MAX  ((M_PI_2*3) + TOUCH_RANGE)
#define RIGHT_MIN (M_PI_2 - TOUCH_RANGE)
#define RIGHT_MAX (M_PI_2 + TOUCH_RANGE)

/* Function pointers to Quake3 code */
int  (*q3main)(int argc, char **argv);
void (*drawFrame)();
void (*queueKeyEvent)(int key, int state);
void (*queueMotionEvent)(int action, float x, float y, float pressure);
void (*queueTrackballEvent)(int action, float x, float y);
void (*requestAudioData)();
void (*setAudioCallbacks)(void *func, void *func2, void *func3);
void (*setInputCallbacks)(void *func);
void (*setResolution)(int width, int height);

/* Callbacks to Android */
jmethodID android_getPos;
jmethodID android_initAudio;
jmethodID android_writeAudio;
jmethodID android_setMenuState;

/* Contains the game directory e.g. /mnt/sdcard/quake3 */
static char* game_dir="";

/* Containts the path to /data/data/(package_name)/libs */
static char* lib_dir=NULL;

static JavaVM *jVM;
static jboolean audioEnabled=1;
static jboolean benchmarkEnabled=0;
static jboolean lightmapsEnabled=0;
static jboolean showFramerateEnabled=0;
static jobject audioBuffer=0;
static jobject kwaakAudioObj=0;
static jobject kwaakRendererObj=0;
static void *libdl;
static int init=0;

//#define DEBUG
typedef enum fp_type 
{
     FP_TYPE_NONE = 0,
     FP_TYPE_VFP  = 1,
     FP_TYPE_NEON = 2
} fp_type_t;

static fp_type_t fp_support()
{
    char buf[80];
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if(!fp)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "Quake", "Unable to open /proc/cpuinfo\n");
        return FP_TYPE_NONE;
    }

    while(fgets(buf, 80, fp) != NULL)
    {
        char *features = strstr(buf, "Features");

        if(features)
        {
            fp_type_t fp_supported_type = FP_TYPE_NONE;
            char *feature;
            features += strlen("Features");
            feature = strtok(features, ": ");
            while(feature)
            {
                /* We prefer Neon if it is around, else VFP is also okay */
                if(!strcmp(feature, "neon"))
                    return FP_TYPE_NEON;
                else if(!strcmp(feature, "vfp"))
                    fp_supported_type = FP_TYPE_VFP;

                feature = strtok(NULL, ": ");
            }
            return fp_supported_type;
        }
    }
    return FP_TYPE_NONE;
}

int m_fabs(int i)
{
    if(i < 0)
        return -i;
    return i;
}

const char *get_quake3_library()
{
    /* We ship a library with Neon FPU support. This boosts performance a lot but it only works on a few CPUs. */
    fp_type_t fp_supported_type = fp_support();
    if(fp_supported_type == FP_TYPE_NEON)
        return "libquake3_neon.so";
    else if (fp_supported_type == FP_TYPE_VFP)
        return "libquake3_vfp.so";

    return "libquake3.so";
}

void get_quake3_library_path(char *path)
{
    const char *libquake3 = get_quake3_library();
    if(lib_dir)
    {
        sprintf(path, "%s/%s", lib_dir, libquake3);
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR, "Quake_JNI", "Library path not set, trying /data/data/org.kwaak3/lib");
        sprintf(path, "/data/data/org.kwaak3/lib/%s", libquake3);
    }
}

static void load_libquake3()
{
    char libquake3_path[80];
    get_quake3_library_path(libquake3_path);

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake", "Attempting to load %s\n", libquake3_path);
#endif

    libdl = dlopen(libquake3_path, RTLD_NOW);
    if(!libdl)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Quake", "Unable to load libquake3.so: %s\n", dlerror());
        return;
    }

    q3main = dlsym(libdl, "main");
    drawFrame = dlsym(libdl, "nextFrame");
    queueKeyEvent = dlsym(libdl, "queueKeyEvent");
    queueMotionEvent = dlsym(libdl, "queueMotionEvent");
    queueTrackballEvent = dlsym(libdl, "queueTrackballEvent");
    requestAudioData = dlsym(libdl, "requestAudioData");
    setAudioCallbacks = dlsym(libdl, "setAudioCallbacks");
    setInputCallbacks = dlsym(libdl, "setInputCallbacks");
    setResolution = dlsym(libdl, "setResolution");
    init=1;
}

int getPos()
{
    JNIEnv *env;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "getPos");
#endif
    return (*env)->CallIntMethod(env, kwaakAudioObj, android_getPos);
}

void initAudio(void *buffer, int size)
{
    JNIEnv *env;
    jobject tmp;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "initAudio");
#endif
    tmp = (*env)->NewDirectByteBuffer(env, buffer, size);
    audioBuffer = (jobject)(*env)->NewGlobalRef(env, tmp);

    if(!audioBuffer) __android_log_print(ANDROID_LOG_ERROR, "Quake_JNI", "yikes, unable to initialize audio buffer");

    return (*env)->CallVoidMethod(env, kwaakAudioObj, android_initAudio);
}

void writeAudio(int offset, int length)
{
    JNIEnv *env;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "writeAudio audioBuffer=%p offset=%d length=%d", audioBuffer, offset, length);
#endif

    (*env)->CallVoidMethod(env, kwaakAudioObj, android_writeAudio, audioBuffer, offset, length);
}

void setMenuState(int state)
{
    JNIEnv *env;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "setMenuState state=%d", state);
#endif
   // causes segfault, dunno why
   // (*env)->CallVoidMethod(env, kwaakRendererObj, android_setMenuState, state);
}

int JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv *env;
    jVM = vm;

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "JNI_OnLoad called");
#endif

    if((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Quake_JNI", "Failed to get the environment using GetEnv()");
        return -1;
    }

    if(!init) load_libquake3();

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_enableAudio(JNIEnv *env, jclass c, jboolean enable)
{
    audioEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_enableBenchmark(JNIEnv *env, jclass c, jboolean enable)
{
    benchmarkEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_enableLightmaps(JNIEnv *env, jclass c, jboolean enable)
{
    lightmapsEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_showFramerate(JNIEnv *env, jclass c, jboolean enable)
{
    showFramerateEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_setAudio(JNIEnv *env, jclass c, jobject obj)
{
    kwaakAudioObj = obj;
    jclass kwaakAudioClass;

    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    kwaakAudioObj = (jobject)(*env)->NewGlobalRef(env, obj);
    kwaakAudioClass = (*env)->GetObjectClass(env, kwaakAudioObj);

    android_getPos = (*env)->GetMethodID(env,kwaakAudioClass,"getPos","()I");
    android_initAudio = (*env)->GetMethodID(env,kwaakAudioClass,"initAudio","()V");
    android_writeAudio = (*env)->GetMethodID(env,kwaakAudioClass,"writeAudio","(Ljava/nio/ByteBuffer;II)V");
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_setRenderer(JNIEnv *env, jclass c, jobject obj)
{
    kwaakRendererObj = obj;
    jclass kwaakRendererClass;

    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    kwaakRendererObj = (jobject)(*env)->NewGlobalRef(env, obj);
    kwaakRendererClass = (*env)->GetObjectClass(env, kwaakRendererObj);

    android_setMenuState = (*env)->GetMethodID(env,kwaakRendererClass,"setMenuState","(I)V");
}


JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_initGame(JNIEnv *env, jclass c, jint width, jint height)
{
    char *argv[4];
    int argc=0;

    /* TODO: integrate settings with quake3, right now there is no synchronization */

    if(!audioEnabled)
    {
        argv[argc] = strdup("+set s_initsound 0");
        argc++;
    }

    if(lightmapsEnabled)
        argv[argc] = strdup("+set r_vertexlight 0");
    else
        argv[argc] = strdup("+set r_vertexlight 1");
    argc++;

    if(showFramerateEnabled)
        argv[argc] = strdup("+set cg_drawfps 1");
    else
        argv[argc] = strdup("+set cg_drawfps 0");
    argc++;

    if(benchmarkEnabled)
    {
        argv[argc] = strdup("+demo four +timedemo 1");
        argc++;
    }

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "initGame(%d, %d)", width, height);
#endif

    setAudioCallbacks(&getPos, &writeAudio, &initAudio);
    setInputCallbacks(&setMenuState);
    setResolution(width, height);

    /* In the future we might want to pass arguments using argc/argv e.g. to start a benchmark at startup, to load a mod or whatever */
    q3main(argc, argv);
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_drawFrame(JNIEnv *env, jclass c)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "nextFrame()");
#endif
    if(drawFrame) drawFrame();
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_queueKeyEvent(JNIEnv *env, jclass c, jint key, jint state)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "queueKeyEvent(%d, %d)", key, state);
#endif
    if(queueKeyEvent) queueKeyEvent(key, state);
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_queueMotionEvent(JNIEnv *env, jclass c, jint action, jfloat x, jfloat y, jfloat pressure)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "queueMotionEvent(%d, %f, %f, %f)", action, x, y, pressure);
#endif
    if(queueMotionEvent) queueMotionEvent(action, x, y, pressure);
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_queueTrackballEvent(JNIEnv *env, jclass c, jint action, jfloat x, jfloat y)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "queueTrackballEvent(%d, %f, %f)", action, x, y);
#endif
    if(queueTrackballEvent) queueTrackballEvent(action, x, y);
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_requestAudioData(JNIEnv *env, jclass c)
{
    if(requestAudioData) requestAudioData();
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_setGameDirectory(JNIEnv *env, jclass c, jstring jpath)
{
    jboolean iscopy;
    const jbyte *path = (*env)->GetStringUTFChars(env, jpath, &iscopy);
    game_dir = strdup(path);
    setenv("GAME_PATH", game_dir, 1);
    (*env)->ReleaseStringUTFChars(env, jpath, path);

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "game path=%s\n", game_dir);
#endif
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_setLibraryDirectory(JNIEnv *env, jclass c, jstring jpath)
{
    jboolean iscopy;
    const jbyte *path = (*env)->GetStringUTFChars(env, jpath, &iscopy);
    lib_dir = strdup(path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "library path=%s\n", lib_dir);
#endif
}

JNIEXPORT jbyte JNICALL Java_org_kwaak3_KwaakJNI_calculateMotion(JNIEnv *env, jclass c, jint baseX, jint baseY, jint X, jint Y, jshortArray curKeys)
{
    if(m_fabs(X-baseX) < 20 && m_fabs(Y-baseY) < 20)
        return 0;

    jshort *current = (jshort *)malloc(2);
    (*env)->GetShortArrayRegion(env, curKeys, 0, 2, current);
    jshort *new = (jshort *)malloc(2);

    int dx = Y - baseY;
    int dy = X - baseX;
    float ang = atan2(dy, dx);
    ang = (float) ((ang >= 0) ? ang : 2 * 3.141592 + ang);

    if(ang > DOWN_MIN || ang <  DOWN_MAX) // 0.7 PI range
        new[0] = QK_DOWN;
    else if(ang > UP_MIN && ang < UP_MAX)
        new[0] = QK_UP;
    else
        new[0] = 0;

    if(ang > LEFT_MIN && ang < LEFT_MAX)
        new[1] = QK_LEFT;
    else if(ang > RIGHT_MIN && ang < RIGHT_MAX)
        new[1] = QK_RIGHT;
    else
        new[1] = 0;

    short itr = 0;
    for(; itr < 2; ++itr)
    {
        if(new[itr] == current[itr])
            continue;

        if(current[itr] != 0) queueKeyEvent(current[itr], 0);
        if(new[itr] != 0)     queueKeyEvent(new[itr], 1);
    }
    free(current);
    (*env)->SetShortArrayRegion(env, curKeys, 0, 2, new );
    free(new);

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "calculate motion event base %u %u, current %u %u, angle %f", baseX, baseY, X, Y, ang);
#endif
    return 1;
}
