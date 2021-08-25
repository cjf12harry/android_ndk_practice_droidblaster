//
// Created by cjf12 on 2019-10-10.
//

#include "include/EventLoop.h"
#include "include/Log.h"

EventLoop::EventLoop(android_app *pApplication,
                     ActivityHandler &pActivityHandler,
                     InputHandler &pInputHandler) :
        mApplication(pApplication),
        mEnabled(false), mQuit(false),
        mActivityHandler(pActivityHandler),
        mInputHandler(pInputHandler),
        mSensorPollSource(), mSensorManager(NULL),
        mSensorEventQueue(NULL), mAccelerometer(NULL) {
    mApplication->userData = this;
    mApplication->onAppCmd = callback_appEvent;

    // Fill this in with the function to process input events.  At this point
    // the event has already been pre-dispatched, and it will be finished upon
    // return.  Return 1 if you have handled the event, 0 for any default
    // dispatching.
    mApplication->onInputEvent = callback_input;
}

void EventLoop::run() {
    int32_t result, events;
    android_poll_source *source;

    //Makes sure native glue is not stripped by the linker.
    app_dummy();

    Log::info("Starting event loop");
    while (true) {
        //Event processing loop.
        while ((result = ALooper_pollAll(mEnabled ? 0 : -1, NULL, &events, (void **) &source)) >=
               0) {
            if (source != NULL) {
                source->process(mApplication, source);
            }
            // Application is getting destroyed.
            if (mApplication->destroyRequested) {
                Log::info("Exiting event loop");
                return;
            }
        }

        if ((mEnabled) && (!mQuit)) {
            if (mActivityHandler.onStep() != STATUS_OK) {
                mQuit = true;
                ANativeActivity_finish(mApplication->activity);
            }
        }
    }
}

void EventLoop::activate() {
    if ((!mEnabled) && (mApplication->window != NULL)) {
        // Data associated with an ALooper fd that will be returned as the "outData"
        // when that source has data ready.
        mSensorPollSource.id = LOOPER_ID_USER; //this source is user defined.
        mSensorPollSource.app = mApplication; //the app that this source is associated with.
        mSensorPollSource.process = callback_sensor; //callback for processing data from this poll source.
        mSensorManager = ASensorManager_getInstance();
        if (mSensorManager != NULL) {
            /*Creates a new sensor event queue and associate it with a looper.
            *  "ident" is a identifier for the events that will be returned when
            * calling ALooper_pollOnce(). The identifier must be >= 0, or
            * ALOOPER_POLL_CALLBACK if providing a non-NULL callback.
             */
            mSensorEventQueue = ASensorManager_createEventQueue(
                    mSensorManager, mApplication->looper,
                    LOOPER_ID_USER, NULL, &mSensorPollSource);
            if (mSensorEventQueue == NULL) goto ERROR;
        }
        activateAccelerometer();

        mQuit = false;
        mEnabled = true;
        if (mActivityHandler.onActivate() != STATUS_OK) {
            goto ERROR;
        }
    }
    return;

    ERROR:
    mQuit = true;
    deactivate();
    ANativeActivity_finish(mApplication->activity);
}

void EventLoop::deactivate() {
    if (mEnabled) {
        deactivateAccelerometer();
        if (mSensorEventQueue != NULL) {
            // Destroys the event queue and free all resources associated to it.
            ASensorManager_destroyEventQueue(mSensorManager, mSensorEventQueue);
            mSensorEventQueue = NULL;
        }
        mSensorManager = NULL;

        mActivityHandler.onDeactivate();
        mEnabled = false;
    }
}

void EventLoop::callback_appEvent(android_app *pApplication, int32_t pCommand) {
    EventLoop &eventLoop = *(EventLoop *) pApplication->userData;
    eventLoop.processAppEvent(pCommand);
}

int32_t EventLoop::callback_input(android_app *pApplication, AInputEvent *pEvent) {
    EventLoop &eventLoop = *(EventLoop *) pApplication->userData;
    return eventLoop.processInputEvent(pEvent);
}

void EventLoop::processAppEvent(int32_t pCommand) {
    switch (pCommand) {
        case APP_CMD_CONFIG_CHANGED:
            mActivityHandler.onConfigurationChanged();
            break;
        case APP_CMD_INIT_WINDOW:
            mActivityHandler.onCreateWindow();
            break;
        case APP_CMD_DESTROY:
            mActivityHandler.onDestroy();
            break;
        case APP_CMD_GAINED_FOCUS:
            activate();
            mActivityHandler.onGainFocus();
            break;
        case APP_CMD_LOST_FOCUS:
            mActivityHandler.onLostFocus();
            deactivate();
            break;
        case APP_CMD_LOW_MEMORY:
            mActivityHandler.onLowMemory();
            break;
        case APP_CMD_PAUSE:
            mActivityHandler.onPause();
            deactivate();
            break;
        case APP_CMD_RESUME:
            mActivityHandler.onResume();
            break;
        case APP_CMD_SAVE_STATE:
            mActivityHandler.onSaveInstanceState(
                    &mApplication->savedState, &mApplication->savedStateSize);
            break;
        case APP_CMD_START:
            mActivityHandler.onStart();
            break;
        case APP_CMD_STOP:
            mActivityHandler.onStop();
            break;
        case APP_CMD_TERM_WINDOW:
            mActivityHandler.onDestroyWindow();
            deactivate();
            break;
        default:
            break;
    }
}

int32_t EventLoop::processInputEvent(AInputEvent *pEvent) {
    if (!mEnabled) return 0;

    int32_t eventType = AInputEvent_getType(pEvent);
    switch (eventType) {
        case AINPUT_EVENT_TYPE_MOTION:
            switch (AInputEvent_getSource(pEvent)) {
                case AINPUT_SOURCE_TOUCHSCREEN:
                    return mInputHandler.onTouchEvent(pEvent);
                case AINPUT_SOURCE_TRACKBALL:
                    return mInputHandler.onTrackballEvent(pEvent);
            }
            break;
        case AINPUT_EVENT_TYPE_KEY:
            return mInputHandler.onKeyboardEvent(pEvent);
    }
    return 0;
}

void EventLoop::callback_sensor(android_app *pApplication, android_poll_source *pSource) {
    EventLoop &eventLoop = *(EventLoop *) pApplication->userData;
    eventLoop.processSensorEvent();
}

void EventLoop::processSensorEvent() {
    ASensorEvent event;
    if (!mEnabled) return;

    /* Retrieve pending events in sensor event queue
     * Retrieve next available events from the queue to a specified event array.
     * The number is the number of event to be retrieved */
    while (ASensorEventQueue_getEvents(mSensorEventQueue, &event, 1) > 0) {
        switch (event.type) {
            case ASENSOR_TYPE_ACCELEROMETER:
                mInputHandler.onAccelerometerEvent(&event);
                break;
        }
    }
}

void EventLoop::activateAccelerometer() {
    mAccelerometer = ASensorManager_getDefaultSensor(
            mSensorManager, ASENSOR_TYPE_ACCELEROMETER);
    if (mAccelerometer != NULL) {
        /*
        * Enable the selected sensor at default sampling rate.
        * Start event reports of a sensor to specified sensor event queue at a default rate.
        */
        if (ASensorEventQueue_enableSensor(mSensorEventQueue, mAccelerometer) < 0) {
            Log::error("Could not enable accelerometer");
            return;
        }
        /*
         * Returns the minimum delay allowed between events in microseconds.
         * A value of zero means that this sensor doesn't report events at a
         * constant rate, but rather only when a new data is available.
         */
        int32_t minDelay = ASensor_getMinDelay(mAccelerometer);

        /*
         * Sets the delivery rate of events in microseconds for the given sensor.
         * This function has to be called after {@link ASensorEventQueue_enableSensor}.
         * Note that this is a hint only, generally event will arrive at a higher
         * rate. It is an error to set a rate inferior to the value returned by
         * ASensor_getMinDelay().
         */
        if (ASensorEventQueue_setEventRate(mSensorEventQueue, mAccelerometer, minDelay) < 0) {
            Log::error("Could not set accelerometer rate");
        }
    } else {
        Log::error("No accelerometer found");
    }
}

void EventLoop::deactivateAccelerometer() {
    if (mAccelerometer != NULL) {
        if (ASensorEventQueue_disableSensor(mSensorEventQueue, mAccelerometer) < 0) {
            Log::error("Error while deactivating sensor");
        }
        mAccelerometer = NULL;
    }
}
