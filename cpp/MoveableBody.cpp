//
// Created by cjf12 on 2019-10-25.
//

#include "include/MoveableBody.h"
#include "include/Log.h"

static const float MOVE_SPEED = 10.0f/PHYSICS_SCALE;

MoveableBody::MoveableBody(android_app *pApplication,
                           InputManager &pInputManager,
                           PhysicsManager &pPhysicsManager,
                           GraphicsManager &pGraphicsManager
) :
        mInputManager(pInputManager),
        mPhysicsManager(pPhysicsManager),
        mBody(NULL), mTarget(NULL),
        mGraphicsManager(pGraphicsManager) {

}

b2Body *
MoveableBody::registerMoveableBody(Location &pLocation, int32_t pSizeX, int32_t pSizeY) {
    //the space ship is cat 2, will collide with cat 1 objects
    mBody = mPhysicsManager.loadBody(pLocation, 0x2, 0x1, pSizeX, pSizeY, 0.0f);
    //define the ship is move towards a mouse anchor point. The touch point is like a imaginary object that the ship is joint.
    mTarget = mPhysicsManager.loadTarget(mBody);
    mInputManager.setRefPoint(&pLocation);
    return mBody;
}

void MoveableBody::initialize() {
    //Set the linear speed of the body to be zero
    mBody->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
    mUpperBound = mGraphicsManager.getRenderHeight();
    mLowerBound = 0;
    mLeftBound = 0;
    mRightBound = mGraphicsManager.getRenderWidth();
}

void MoveableBody::update() {
    static const float MOVE_SPEED = 320.0f;
    b2Vec2 target = mBody->GetPosition() + b2Vec2(
            mInputManager.getDirectionX()*MOVE_SPEED,
            mInputManager.getDirectionY()*MOVE_SPEED);
    //Set the target location to move the body towards.
    mTarget->SetTarget(target);
   Log::info("Velocity: %f %f \n", mBody->GetLinearVelocity().x, mBody->GetLinearVelocity().y);
}
