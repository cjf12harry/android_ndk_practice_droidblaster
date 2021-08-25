//
// Created by cjf12 on 2019-10-10.
//

#include "include/DroidBlaster.h"
#include "include/Log.h"
#include "include/Sound.h"
#include <unistd.h>

static const int32_t SHIP_SIZE = 64;
static const int32_t SHIP_FRAME_1 = 0;
static const int32_t SHIP_FRAME_COUNT = 8;
static const float SHIP_ANIM_SPEED = 8.0f;

static const int32_t ASTEROID_COUNT = 16;
static const int32_t ASTEROID_SIZE = 64;
static const int32_t ASTEROID_FRAME_1 = 0;
static const int32_t ASTEROID_FRAME_COUNT = 16;
static const float ASTEROID_MIN_ANIM_SPEED = 16.0f;
static const float ASTEROID_ANUM_SPEED_RANGE = 32.0f;

static const int32_t STAR_COUNT = 50;

DroidBlaster::DroidBlaster(android_app *pApplication) :
        mTimeManager(),
        mGraphicsManager(pApplication),
        mPhysicsManager(mTimeManager, mGraphicsManager),
        mSoundManager(pApplication),
        mInputManager(pApplication, mGraphicsManager),
        mEventLoop(pApplication, *this, mInputManager),
        mAsteroidTexture(pApplication, "droidblaster/asteroid.png"),
        mShipTexture(pApplication, "droidblaster/ship.png"),
        mStarTexture(pApplication, "droidblaster/star.png"),
        mBGM(pApplication, "droidblaster/bgm.mp3"),
        mCollisionSound(pApplication, "droidblaster/collision.pcm"),
        mShip(pApplication, mGraphicsManager, mSoundManager),
        mStarField(pApplication, mTimeManager, mGraphicsManager, STAR_COUNT, mStarTexture),
        mAsteroids(pApplication, mTimeManager, mGraphicsManager, mPhysicsManager),
        mSpriteBatch(mTimeManager, mGraphicsManager),
        mMoveableBody(pApplication, mInputManager, mPhysicsManager, mGraphicsManager) {
    Log::info("Creating DroidBlaster");
    Sprite *shipGraphics = mSpriteBatch.registerSprite(mShipTexture, SHIP_SIZE, SHIP_SIZE);
    shipGraphics->setAnimation(SHIP_FRAME_1, SHIP_FRAME_COUNT, SHIP_ANIM_SPEED, true);
    Sound *collisionSOund = mSoundManager.registerSound(mCollisionSound);
    b2Body* shipBody = mMoveableBody.registerMoveableBody(shipGraphics->location, SHIP_SIZE, SHIP_SIZE);
    mShip.registerShip(shipGraphics, collisionSOund, shipBody);


    for (int i = 0; i < ASTEROID_COUNT; ++i) {
        Sprite *asteroidGraphics = mSpriteBatch.registerSprite(mAsteroidTexture, ASTEROID_SIZE,
                                                               ASTEROID_SIZE);
        float animSpeed = ASTEROID_MIN_ANIM_SPEED + RAND(ASTEROID_ANUM_SPEED_RANGE);
        asteroidGraphics->setAnimation(ASTEROID_FRAME_1, ASTEROID_FRAME_COUNT, animSpeed, true);
        mAsteroids.registerAsteroid(
                asteroidGraphics->location, ASTEROID_SIZE, ASTEROID_SIZE);
    }

}

void DroidBlaster::run() {
    mEventLoop.run();
}

status DroidBlaster::onActivate() {
    Log::info("Activating DroidBlaster");
    if (mGraphicsManager.start() != STATUS_OK) return STATUS_KO;
    if (mSoundManager.start() != STATUS_OK)
        return STATUS_KO;

    mInputManager.start();
    mPhysicsManager.start();
    mSoundManager.playBGM(mBGM);
    //mSoundManager.recordSound();

    //Initializes game objects.
    mAsteroids.initialize();
    mShip.initialize();
    mMoveableBody.initialize();

    mTimeManager.reset();
    return STATUS_OK;
}

void DroidBlaster::onDeactivate() {
    Log::info("Deactivating DroidBlaster");
    mGraphicsManager.stop();
    mSoundManager.stop();
}


status DroidBlaster::onStep() {
    mTimeManager.update();
    mPhysicsManager.update();

    mAsteroids.update();
    mMoveableBody.update();
    mShip.update();

    if (mShip.isDestroyed()) return STATUS_EXIT;
    return mGraphicsManager.update();
}

void DroidBlaster::onStart() {
    Log::info("onStart");
}

void DroidBlaster::onResume() {
    Log::info("onResume");
}

void DroidBlaster::onPause() {
    Log::info("onPause");
}

void DroidBlaster::onStop() {
    Log::info("onStop");
}

void DroidBlaster::onDestroy() {
    Log::info("onDestroy");
}

void DroidBlaster::onSaveInstanceState(void **pData, size_t *pSize) {
    ActivityHandler::onSaveInstanceState(pData, pSize);
}

void DroidBlaster::onConfigurationChanged() {
    ActivityHandler::onConfigurationChanged();
}

void DroidBlaster::onLowMemory() {
    ActivityHandler::onLowMemory();
}

void DroidBlaster::onCreateWindow() {
    ActivityHandler::onCreateWindow();
}

void DroidBlaster::onDestroyWindow() {
    ActivityHandler::onDestroyWindow();
}

void DroidBlaster::onGainFocus() {
    ActivityHandler::onGainFocus();
}

void DroidBlaster::onLostFocus() {
    ActivityHandler::onLostFocus();
}
