//
// Created by cjf12 on 2019-10-25.
//

#include "include/InputManager.h"
#include "include/Log.h"

#include <android_native_app_glue.h>
#include <cmath>

InputManager::InputManager(android_app *pApplication,
                           GraphicsManager &pGraphicsManager) :
        mApplication(pApplication),
        mGraphicsManager(pGraphicsManager),
        mDirectionX(0.0f),
        mDirectionY(0.0f),
        mRefPoint(NULL) {
    Configuration configuration(pApplication);
    mRotation = configuration.getRotation();

}

void InputManager::start() {
    Log::info("Starting input manager");
    mDirectionX = 0.0f;
    mDirectionY = 0.0f;
    mScaleFactor =
            float(mGraphicsManager.getRenderWidth()) / float(mGraphicsManager.getScreenWidth());
}

bool InputManager::onTouchEvent(AInputEvent *pEvent) {
    static const float TOUCH_MAX_RANGE = 65.0f; //in game units;

    if (mRefPoint != NULL) {
        /* Get the combined motion event action code and pointer index. */
        if (AMotionEvent_getAction(pEvent) == AMOTION_EVENT_ACTION_MOVE) {
            /*
             * Get the current X coordinate of this event for the given pointer index.
             * Whole numbers are pixels; the value may have a fraction for input devices
             * that are sub-pixel precise.
             */
            float x = AMotionEvent_getX(pEvent, 0) * mScaleFactor;
            float y = (float(mGraphicsManager.getScreenHeight()) - AMotionEvent_getY(pEvent, 0)) *
                      mScaleFactor; //Everythings needs to be in the rendering scale

            Log::info("Touch X:%f Y:%f \n", AMotionEvent_getX(pEvent, 0),
                      AMotionEvent_getY(pEvent, 0));

            //Needs a conversion to proper coordinates
            // (origin at bottom/left). Only moveY needs it.
            float moveX = x - mRefPoint->x;
            float moveY = y - mRefPoint->y;
            float moveRange = sqrt((moveX * moveX) + (moveY * moveY));

            if (moveRange > TOUCH_MAX_RANGE) {
                float cropFactor = TOUCH_MAX_RANGE / moveRange;
                moveX *= cropFactor;
                moveY *= cropFactor;
            }

            mDirectionX = moveX / TOUCH_MAX_RANGE;
            mDirectionY = moveY / TOUCH_MAX_RANGE;
        } else {
            mDirectionX = 0.0f;
            mDirectionY = 0.0f;
        }
        return true;
    }
    return false;
}

bool InputManager::onKeyboardEvent(AInputEvent *pEvent) {
    static const float ORTHOGONAL_MOVE = 1.0f;
    if (AKeyEvent_getAction(pEvent) == AKEY_EVENT_ACTION_DOWN) {
        switch (AKeyEvent_getKeyCode(pEvent)) {
            case AKEYCODE_DPAD_LEFT:
                mDirectionX = -ORTHOGONAL_MOVE;
                return true;
            case AKEYCODE_DPAD_RIGHT:
                mDirectionX = ORTHOGONAL_MOVE;
                return true;
            case AKEYCODE_DPAD_DOWN:
                mDirectionY = -ORTHOGONAL_MOVE;
                return true;
            case AKEYCODE_DPAD_UP:
                mDirectionY = ORTHOGONAL_MOVE;
                return true;
        }
    } else {
        switch (AKeyEvent_getKeyCode(pEvent)) {
            case AKEYCODE_DPAD_LEFT:
            case AKEYCODE_DPAD_RIGHT:
                mDirectionX = 0.0f;
                return true;
            case AKEYCODE_DPAD_DOWN:
            case AKEYCODE_DPAD_UP:
                mDirectionY = 0.0f;
                return true;
        }
    }
    return false;
}

bool InputManager::onTrackballEvent(AInputEvent *pEvent) {
    static const float ORTHOGONAL_MOVE = 1.0f;
    static const float DIAGONAL_MOVE = 0.707f;
    static const float THRESHOLD = (1 / 100.0f);

    if (AMotionEvent_getAction(pEvent) == AMOTION_EVENT_ACTION_MOVE) {
        float directionX = AMotionEvent_getX(pEvent, 0);
        float directionY = AMotionEvent_getY(pEvent, 0);
        float horizontal, vertical;

        if (directionX < -THRESHOLD) {
            if (directionY < -THRESHOLD) {
                horizontal = -DIAGONAL_MOVE;
                vertical = DIAGONAL_MOVE;
            } else if (directionY > THRESHOLD) {
                horizontal = -DIAGONAL_MOVE;
                vertical = -DIAGONAL_MOVE;
            } else {
                horizontal = -ORTHOGONAL_MOVE;
                vertical = 0.0f;
            }
        } else if (directionX > THRESHOLD) {
            if (directionY < -THRESHOLD) {
                horizontal = DIAGONAL_MOVE;
                vertical = DIAGONAL_MOVE;
            } else if (directionY > THRESHOLD) {
                horizontal = DIAGONAL_MOVE;
                vertical = -DIAGONAL_MOVE;
            } else {
                horizontal = ORTHOGONAL_MOVE;
                vertical = 0.0f;
            }
        } else if (directionY < -THRESHOLD) {
            horizontal = 0.0f;
            vertical = ORTHOGONAL_MOVE;
        } else if (directionY > THRESHOLD) {
            horizontal = 0.0f;
            vertical = -ORTHOGONAL_MOVE;
        }

        //Ends movement if there is a counter movement.
        if ((horizontal < 0.0f) && (mDirectionX > 0.0f)) {
            mDirectionX = 0.0f;
        } else if ((horizontal > 0.0f) && (mDirectionX < 0.0f)) {
            mDirectionX = horizontal;
        }

        if ((vertical < 0.0f) && (mDirectionY > 0.0f)) {
            mDirectionY = 0.0f;
        } else if ((vertical > 0.0f) && (mDirectionY < 0.0f)) {
            mDirectionY = 0.0f;
        } else {
            mDirectionY = vertical;
        }
    } else {
        mDirectionX = 0.0f;
        mDirectionY = 0.0f;
    }
    return true;
}

bool InputManager::onAccelerometerEvent(ASensorEvent *pEvent) {
    static const float GRAVITY = ASENSOR_STANDARD_GRAVITY;
    static const float MIN_X = -1.0f;
    static const float MAX_X = 1.0f;
    static const float MIN_Y = -1.0f;
    static const float MAX_Y = 1.0f;
    static const float CENTER_X = (MAX_X + MIN_X) / 2.0f;
    static const float CENTER_Y = (MAX_Y + MIN_Y) / 2.0f;
    // Converts from canonical to screen coordinates.
    ASensorVector vector;
    toScreenCoord(mRotation, &pEvent->vector, &vector);

    // The acceleration directions during portrait mode
    // Center -> Home Button(Bottom)  Positive Y; Center -> Camera (Top) Negative Y.
    // Center -> Left Edge Positive X; Center -> Right Edge Negative X;
    // Laying on table , into the screen Positive Z; out from the screen Negative Z;

    //Roll tilt.
    float rawHorizontal = vector.x / GRAVITY;
    if (rawHorizontal > MAX_X) {
        rawHorizontal = MAX_X;
    } else if (rawHorizontal < MIN_X) {
        rawHorizontal = MIN_X;
    }
    mDirectionX = CENTER_X - rawHorizontal;

    // Pitch tilt. Final value needs to be inverted.
    float rawVertical = vector.y / GRAVITY;
    if (rawVertical > MAX_Y) {
        rawVertical = MAX_Y;
    } else if (rawVertical < MIN_Y) {
        rawVertical = MIN_Y;
    }
    mDirectionY = rawVertical;
    return true;
}

void InputManager::toScreenCoord(screen_rot pRotation, ASensorVector *pCanonical,
                                 ASensorVector *pScreen) {
    struct AxisSwap {
        int8_t negX;
        int8_t negY;
        int8_t xSrc;
        int8_t ySrc;
    };

    static const AxisSwap axisSwaps[] = {
            {1,  -1, 0, 1}, //ROTATION_0
            {-1, -1, 1, 0}, //ROTATION_90
            {-1, 1,  0, 1}, //ROTATION_180
            {1,  1,  1, 0}}; //ROTATION_270

    const AxisSwap &swap = axisSwaps[pRotation];

    pScreen->v[0] = swap.negX * pCanonical->v[swap.xSrc];
    pScreen->v[1] = swap.negY * pCanonical->v[swap.ySrc];
    pScreen->v[2] = pCanonical->v[2];

}
