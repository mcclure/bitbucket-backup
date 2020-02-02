// MAIN LOOP -- ANDROID

// File contains code from Google "NativeActivity" sample-- notice applies to that code only
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// File also contains additions as part of "Jumpcore"-- notice applies to that code only
/* Copyright (C) 2008-2010 Andi McClure
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//BEGIN_INCLUDE(all)
#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

// for native audio
#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/storage_manager.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>

#include "kludge.h"
#include <string>

#include "chipmunk.h"
#include "program.h"
#include "glCommon.h"
#include "internalfile.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

void opensl_init();

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;
    AAssetManager* assetManager;
    AStorageManager* storageManager;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
};

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, dummy, format;
    EGLint numConfigs;
#define CONFIGBUFFER_SIZE 16
    EGLConfig allconfig[CONFIGBUFFER_SIZE];
    EGLConfig &config = allconfig[0];
    EGLSurface surface;
    EGLContext context;

#if 0&&SELF_EDIT
    // I use this to give myself a chance to attach gdb
	ERR("Purestart");
	time_t until = time(NULL) + 15; int ld = -1;
	do {
		int nd = until - time(NULL);
		if (ld != nd) {
			ERR("%d...", nd);
			ld = nd;
		}
	} while(ld > 0);
	ERR("Awake\n");
#endif

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    eglChooseConfig(display, attribs, allconfig, CONFIGBUFFER_SIZE, &numConfigs);

    ERR("Numconfigs %d", numConfigs);

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, NULL);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    // Initialize GL state.
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    //glEnable(GL_CULL_FACE);
    //glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h);

    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
        return;
    }
    // Just fill the screen with a color.
#if 1
    display();

    program_update();
#endif

    eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
	ERR("Destroy surface");
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

list<touch_rec> pointersToList(AInputEvent *event) {
    list<touch_rec> values;
    int count = AMotionEvent_getPointerCount(event);
    for(int c = 0; c < count; c++) {
    	int id = AMotionEvent_getPointerId(event, c);
    	float x = AMotionEvent_getX(event, c);
    	float y = AMotionEvent_getY(event, c);
        values.push_back( touch_rec(id, cpv(x, y) ) );
    }
    return values;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        unsigned int flags = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        if (flags == AMOTION_EVENT_ACTION_DOWN || flags == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        	program_eventtouch(pointersToList(event), touch_down);
        } else if (flags == AMOTION_EVENT_ACTION_UP || flags == AMOTION_EVENT_ACTION_POINTER_UP) {
        	program_eventtouch(pointersToList(event), touch_up);
        } else if (flags == AMOTION_EVENT_ACTION_MOVE) {
        	program_eventtouch(pointersToList(event), touch_move);
        } else if (flags == AMOTION_EVENT_ACTION_CANCEL) {
        	program_eventtouch(pointersToList(event), touch_cancel);
        } else {
        	return 0;
        }
    } else {
    	return 0;
    }

	return 1;
}

// Passing both app and engine is technically redundant, but makes things easier
static void setup_internalfiles(struct android_app* app, struct engine* engine);

static void jumpcore_init(struct android_app* app, struct engine* engine) {
	ERR("jumpcore_init");
#if 1
    gl2 = false; // FIXME
    //fullscreenw = engine->width; fullscreenh = engine->height;
    surfacew = engine->width; surfaceh = engine->height;
    aspect = double(surfaceh)/surfacew;

    setup_internalfiles(app, engine);

    if (!gl2)
        glPointSize(3.0);

    glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl2Basic();

    display_init(false);
    audio_init();
    opensl_init();

    program_init();
    program_interface();

    engine->animating = 1;
#endif
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
#if 0
            // The system has asked us to save our current state.  Do so.
        	// No state to save right now...
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
#endif
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);

                jumpcore_init(app, engine);

                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            engine->animating = 1;
        	program_wake();
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                        engine->accelerometerSensor, (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            //engine_draw_frame(engine);
            program_sleep();

            // TODO: The current android code here outright crashes on suspend,
            // I think because of changes I made to how "animating" is used
            // (the original native-activity sample code does not *seem* to have
            // this problem). As a stopgap I simply exit() on lose focus; this is
            // equivalent to the current jumpcore/iPhone behavior and should be ok
            // since you ought to be saving your state in program_sleep anyway
            Quit(0);
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine;

    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
            state->looper, LOOPER_ID_USER, NULL, NULL);
    engine.assetManager = state->activity->assetManager;
    engine.storageManager = AStorageManager_new();

#if 0
    // Saved state not used currently
    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }
#endif

    // loop waiting for stuff to do.

    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                            &event, 1) > 0) {
						program_eventaccel(event.acceleration.x, event.acceleration.y, event.acceleration.z);
//                        LOGI("accelerometer: x=%f y=%f z=%f",
//                                event.acceleration.x, event.acceleration.y,
//                                event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }
}

static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    assert(bq == bqPlayerBufferQueue);
    assert(NULL == context);
    // The worrisome thing here: We have one (1) buffer for storing audio playback in.
    // This works if my assumption is correct that we will only ever have one player
    // queue request in flight at any one time-- i.e., we push a buffer on the queue,
    // it plays, then they ask us for more. I'm not sure if this is actually how it
    // works or not. Meanwhile moving up to two  buffers seems like it would be a
    // good idea to reduce the risk of gap artifacts at some point...
    // TODO: 1024?
#define BUFFERSIZE 1024
    static char temp[BUFFERSIZE];
    //ERR("CALLBACK");

    audio_callback(NULL, (unsigned char *)temp, BUFFERSIZE);

    // for streaming playback, replace this test by logic to find and fill the next buffer
    SLresult result;
    // enqueue another buffer
    result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, temp, BUFFERSIZE);
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
    assert(SL_RESULT_SUCCESS == result);
//    ERR("F%d\n",(int)result);
}

void opensl_init() {
    SLresult result;
    // engine interfaces
    static SLObjectItf engineObject = NULL;
    static SLEngineItf engineEngine;

    // output mix interfaces
    static SLObjectItf outputMixObject = NULL;
    static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

    // buffer queue player interfaces
    static SLObjectItf bqPlayerObject = NULL;
    static SLPlayItf bqPlayerPlay;
    //SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    static SLEffectSendItf bqPlayerEffectSend;

    // aux effect on the output mix, used by the buffer queue player
    static const SLEnvironmentalReverbSettings reverbSettings =
        SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    // URI player interfaces
    static SLObjectItf uriPlayerObject = NULL;
    static SLPlayItf uriPlayerPlay;
    static SLSeekItf uriPlayerSeek;

    // file descriptor player interfaces
    static SLObjectItf fdPlayerObject = NULL;
    static SLPlayItf fdPlayerPlay;
    static SLSeekItf fdPlayerSeek;
#if 0
    // recorder interfaces
    static SLObjectItf recorderObject = NULL;
    static SLRecordItf recorderRecord;
    static SLAndroidSimpleBufferQueueItf recorderBufferQueue;
#endif
    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids2[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req2[1] = {SL_BOOLEAN_FALSE}; // Notice: Length 0, first id skipped
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, ids2, req2);
    assert(SL_RESULT_SUCCESS == result);

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
#if 0
    // Original NDK example used environmental reverb... we don't.
    // I should remove this and also the "notice:" cruft above and below
    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
            &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
    }
    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example
#endif

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_44_1,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND}; // Notice: Length 1, second ID skipped
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
            1, ids, req);
    assert(SL_RESULT_SUCCESS == result);

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
            &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
    assert(SL_RESULT_SUCCESS == result);

#if 0
    // get the effect send interface
    // (part of environmental reverb)
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
            &bqPlayerEffectSend);
    assert(SL_RESULT_SUCCESS == result);
#endif

    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);

    // Kick the thing to wake it up
    bqPlayerCallback(bqPlayerBufferQueue,NULL); // Notice: Change the callback setup you may need to change these args
}

// TODO: Android put these comments here. What do they mean?
//END_INCLUDE(all)

// Jumpcore basic stuff follows 
// Should the basic variables be in a util file or something?

//int fullscreenw = 0, fullscreenh = 0;
int surfacew = 0, surfaceh = 0;
int ticks = 0;
double aspect = 0;
bool paused = false;
bool completelyHalted;

// TODO: Throw up an android "dialog box" and then either halt frozen on a single frame or quit
void BombBox(string why) {
	// TODO
    completelyHalted = true;
    ERR("BOMB BOX %s\n", why.c_str());
    Quit(0);
}

string internalfile_path;

#include <time.h>

static JavaVM* vm = NULL;
static JNIEnv* env = NULL; 		// Current Java environment
static jobject obj = NULL;		// Current Activity object

static void setup_internalfiles(struct android_app* app, struct engine* engine) {
	// internalDataPath / externalDataPath are broken on 2.3, so get the files dir from JNI:
	vm = app->activity->vm;
	env = app->activity->env; 		// Current Java environment
	obj = app->activity->clazz;		// Current Activity object

	ERR("Start\n");
	// First make self the VM thread so that we can call JNI at all. (Should only be done once...)
	jint rtn = vm->AttachCurrentThread(&env, 0);

	jmethodID getFilesDir = env->GetMethodID(env->GetObjectClass(obj),
			"getFilesDir","()Ljava/io/File;");
	jobject file = env->CallObjectMethod(obj, getFilesDir); // Get File for user files directory
	jmethodID getAbsolutePath = env->GetMethodID(env->GetObjectClass(file),
			"getAbsolutePath","()Ljava/lang/String;");
	jobject jpath = env->CallObjectMethod(file, getAbsolutePath); // Get String
    const char *path = env->GetStringUTFChars((jstring)jpath, false);
    internalfile_path = path;

    ERR("PATH: %s\n", internalfile_path.c_str());

    char filepath[FILENAMESIZE];
    snprintf(filepath, FILENAMESIZE, "%s/Internal", internalfile_path.c_str());
    mkdir(filepath, 0777); // Will be limited by umask. Will likely fail.

	AAssetDir* root = AAssetManager_openDir(engine->assetManager, "");
	const char *filename;
	while(filename = AAssetDir_getNextFileName(root)) {
	    snprintf(filepath, FILENAMESIZE, "%s/Internal/%s", internalfile_path.c_str(), filename); // Slightly duplicates internalPath
	    // FIXME: Check for presence; skip as appropriate
	    ERR("FILE: [%s] INTO: [%s]\n", filename, filepath);
	    AAsset *asset = AAssetManager_open(engine->assetManager, filename, AASSET_MODE_UNKNOWN); // What does mode even do here
        FILE *outfile = fopen(filepath, "w");
        int toread;
        while (toread = AAsset_getRemainingLength(asset)) {
#define CPBUFSIZE (1024*8)
        	char buffer[CPBUFSIZE];
        	int chunk = min<int>(toread, CPBUFSIZE);
        	int result = AAsset_read(asset, buffer, min<int>(toread, chunk));
        	fwrite(buffer, 1, chunk, outfile);
        }
        fclose(outfile);
        AAsset_close(asset);
	}
	AAssetDir_close(root);
}

void internalPath(char *dst, const char *fmt, int arg1, int arg2) {
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, fmt, arg1, arg2);
    snprintf(dst, FILENAMESIZE, "%s/Internal/%s", internalfile_path.c_str(), filename); // LOOK HOW SIMPLE THAT WAS
}

void userPath(char *dst, const char *fmt, int arg1, int arg2) {
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, fmt, arg1, arg2);
    snprintf(dst, FILENAMESIZE, "%s/%s", internalfile_path.c_str(), filename);
}

void open_url(string url) {
	if (!vm) return;
	
	// GET CLASSES
	jclass uriClass = env->FindClass("android/net/Uri");
	jclass intentClass = env->FindClass("android/content/Intent");
	jclass activityClass = env->GetObjectClass(obj);
	
	// LOCATE AND CALL PARSE METHOD
	jstring urlStr = env->NewStringUTF(url.c_str());
	
    jmethodID parse =
	env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
	
    jvalue parseArgs[1]; parseArgs[0].l = urlStr;
	jobject uri = env->CallStaticObjectMethodA(uriClass, parse, parseArgs);
	
	// LOCATE AND CALL INTENT CONSTRUCTOR
    jmethodID intentInit = env->GetMethodID(intentClass,
											"<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
	
    jfieldID actionViewField = env->GetStaticFieldID(intentClass, "ACTION_VIEW",
													 "Ljava/lang/String;");
    jobject actionView = env->GetStaticObjectField(intentClass, actionViewField);
	
    jvalue intentInitArgs[2]; intentInitArgs[0].l = actionView; intentInitArgs[1].l = uri;
    jobject intent = env->NewObjectA(intentClass, intentInit, intentInitArgs);
	
    // STARTACTIVITY WITH INTENT
	jmethodID startActivity = env->GetMethodID(activityClass,
											   "startActivity","(Landroid/content/Intent;)V");
	jvalue startActivityArgs[1]; startActivityArgs[0].l = intent;
	env->CallVoidMethodA(obj, startActivity, startActivityArgs); // Get File for user files directory
}

void Quit(int code) {
	AboutToQuit();

    exit(code);
}
