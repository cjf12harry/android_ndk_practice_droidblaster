//
// Created by cjf12 on 2019-10-12.
//

#include "include/Asteroid.h"
#include "include/Log.h"

static const float BOUNDS_MARGIN = 128 /PHYSICS_SCALE;
static const float MIN_VELOCITY = 30.0f/PHYSICS_SCALE;
static const float VELOCITY_RANGE = 60.0f/PHYSICS_SCALE;


Asteroid::Asteroid(android_app *pApplication,
                   TimeManager &mTImeManager,
                   GraphicsManager &mGraphicsManager,
                   PhysicsManager &mPhysicsManager) :
        mTImeManager(mTImeManager),
        mGraphicsManager(mGraphicsManager),
        mPhysicsManager(mPhysicsManager),
        mBodies(),
        mMinBound(0.0f),
        mUpperBound(0.0f), mLowerBound(0.0f),
        mLeftBound(0.0f), mRightBound(0.0f) {
}

void Asteroid::registerAsteroid(Location &pLocation, int32_t pSizeX, int32_t pSizeY) {
    //Asteroid is a b2Body with cat1, and only collide with cat 2 objects
    mBodies.push_back(mPhysicsManager.loadBody(pLocation, 0x1, 0x2, pSizeX, pSizeY, 2.0f));
}

void Asteroid::initialize() {
    mMinBound = mGraphicsManager.getRenderHeight() / PHYSICS_SCALE;
    mUpperBound = mMinBound * 2;
    mLowerBound = -BOUNDS_MARGIN;
    mLeftBound = -BOUNDS_MARGIN;
    mRightBound = (mGraphicsManager.getRenderWidth()/PHYSICS_SCALE) + BOUNDS_MARGIN;

    std::vector<b2Body*>::iterator bodyIt;
    for (bodyIt = mBodies.begin(); bodyIt < mBodies.end(); ++bodyIt){
        spawn(*bodyIt);
    }
}

void Asteroid::update() {
    std::vector<b2Body*>::iterator bodyIt;
    for (bodyIt = mBodies.begin(); bodyIt < mBodies.end(); ++bodyIt){
        b2Body* body = *bodyIt;
        if ((body->GetPosition().x < mLeftBound)
        || (body->GetPosition().x > mRightBound)
        || (body->GetPosition().y < mLowerBound)
        || (body->GetPosition().y > mUpperBound)){
            spawn(body);
        }
    }
}

void Asteroid::spawn(b2Body *pBody) {
    float velocity = -(RAND(VELOCITY_RANGE) + MIN_VELOCITY);
    float posX = mLeftBound + RAND(mRightBound - mLeftBound);
    float posY = mMinBound + RAND(mUpperBound - mMinBound);

    //Set the position of the body's origin and rotation. Manipulating a body's transform may cause non-physical behavior. Note: contacts are updated on the next call to b2World::Step.
    pBody->SetTransform(b2Vec2(posX, posY), 0.0f);
    //Set the velocity.
    pBody->SetLinearVelocity(b2Vec2(0.0f, velocity));
}
