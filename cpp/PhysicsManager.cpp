//
// Created by cjf12 on 2019-10-12.
//

#include <Box2D/Box2D/Common/b2Math.h>
#include <Box2D/Box2D/Collision/Shapes/b2ChainShape.h>
#include <Box2D/Box2D/Dynamics/b2Body.h>
#include <Box2D/Box2D/Common/b2Settings.h>
#include <Box2D/Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Box2D/Dynamics/b2Fixture.h>
#include "include/PhysicsManager.h"
#include "include/Log.h"

static const int32_t VELOCITY_ITER = 6;
static const int32_t POSITION_ITER = 2;

PhysicsManager::PhysicsManager(TimeManager &pTimeManager, GraphicsManager &pGraphicsManager) :
        mTimeManager(pTimeManager), mGraphicsManager(pGraphicsManager),
        mWorld(b2Vec2_zero), mBodies(),
        mLocations(),
        mBoundsBodyObj(NULL) {
    Log::info("Creating PhysicsManager.");
    mWorld.SetContactListener(this);
}

void PhysicsManager::start() {
    if (mBoundsBodyObj == NULL) {
        //define the boundary object for the world?

        /// A body definition holds all the data needed to construct a rigid body.
        /// You can safely re-use body definitions. Shapes are added to a body after construction.
        b2BodyDef boundsBodyDef;
        /// A chain shape is a free form sequence of line segments.
        /// The chain has two-sided collision, so you can use inside and outside collision.
        /// Therefore, you may use any winding order.
        /// Since there may be many vertices, they are allocated using b2Alloc.
        /// Connectivity information is used to create smooth collisions.
        b2ChainShape boundsShapeDef;
        float32 renderWidth = mGraphicsManager.getRenderWidth() / PHYSICS_SCALE;
        float32 renderHeight = mGraphicsManager.getRenderHeight() / PHYSICS_SCALE;

        b2Vec2 boundaries[4];
        boundaries[0].Set(0.0f, 0.0f);
        boundaries[1].Set(renderWidth, 0.0f);
        boundaries[2].Set(renderWidth, renderHeight);
        boundaries[3].Set(0.0f, renderHeight);

        //Create a loop. This automatically adjusts connectivity.
        boundsShapeDef.CreateLoop(boundaries, 4);

        //Create a rigid body given a definition. Currently the definition is empty
        mBoundsBodyObj = mWorld.CreateBody(&boundsBodyDef);
        //Creates a fixture from a shape and attach it to this body. This is a convenience function.
        // Use b2FixtureDef if you need to set parameters like friction, restitution, user data,
        // or filtering.
        // If the density is non-zero, this function automatically updates the mass of the body.
        // If the density is zero, the body is static

        /// A fixture is used to attach a shape to a body for collision detection. A fixture
        /// inherits its transform from its parent. Fixtures hold additional non-geometric data
        /// such as friction, collision filters, etc.
        /// Fixtures are created via b2Body::CreateFixture.
        mBoundsBodyObj->CreateFixture(&boundsShapeDef, 0);
    }
}

b2Body *PhysicsManager::loadBody(Location &pLocation,
                                 uint16 pCategory, uint16 pMask, int32_t pSizeX, int32_t pSizeY,
                                 float pRestitution) {
    PhysicsCollision *userData = new PhysicsCollision();

    b2BodyDef mBodyDef;
    b2Body *mBodyObj;
    b2CircleShape mShapeDef;
    b2FixtureDef mFixtureDef;

    //Define the body property
    mBodyDef.type = b2_dynamicBody; // dynamic: positive mass, non-zero velocity determined by forces, moved by solver
    mBodyDef.userData = userData; //the collision record
    mBodyDef.awake = true; // dynamic: positive mass, non-zero velocity determined by forces, moved by solver
    mBodyDef.fixedRotation = true; // Should this body be prevented from rotating? Useful for characters.

    mShapeDef.m_p = b2Vec2_zero; //the position
    int32_t diameter = (pSizeX + pSizeY) / 2; //the diameter
    mShapeDef.m_radius = diameter / (2.0f * PHYSICS_SCALE); //the radius of the circle shape

    /// A fixture definition is used to create a fixture. This class defines an
    /// abstract fixture definition. You can reuse fixture definitions safely.
    mFixtureDef.shape = &mShapeDef;  // A fixture definition is used to create a fixture. This class defines an abstract fixture definition. You can reuse fixture definitions safely.
    mFixtureDef.density = 1.0f; // The density, usually in kg/m^2.
    mFixtureDef.friction = 0.0f; // The friction coefficient, usually in the range [0,1].
    mFixtureDef.restitution = pRestitution; // The restitution (elasticity) usually in the range [0,1].

    //Each body is assigned one or more category (each being
    //represented by one bit in a short integer, the categoryBits member) and a mask
    //describing categories of the body they can collide with (each filtered category being
    //represented by a bit set to 0, the maskBits member), as shown in the following figure:
    mFixtureDef.filter.categoryBits = pCategory; // The collision category bits. Normally you would just set one bit.
    mFixtureDef.filter.maskBits = pMask; //the category that this fixture interacts with.
    mFixtureDef.userData = userData; //the collision data

    mBodyObj = mWorld.CreateBody(&mBodyDef);
    mBodyObj->CreateFixture(&mFixtureDef); //create a fixture associate with this body.
    mBodyObj->SetUserData(userData);
    mLocations.push_back(&pLocation);
    mBodies.push_back(mBodyObj);
    return mBodyObj;
}

/*Implement the loadTarget() method that creates a Box2D mouse joint to
simulate spaceship movements. Such a Joint defines an empty target toward
which the body (here specified in parameter) moves, like a kind of elastic. The
        settings used here (maxForce, dampingRatio, and frequencyHz) control
        how the ship reacts and can be determined by tweaking them, as shown in the
*/
b2MouseJoint *PhysicsManager::loadTarget(b2Body *pBodyObj) {
    b2BodyDef emptyBodyDef;
    b2Body *emptyBody = mWorld.CreateBody(&emptyBodyDef);

    b2MouseJointDef mouseJointDef;
    mouseJointDef.bodyA = emptyBody;
    mouseJointDef.bodyB = pBodyObj;
    /// The initial world target point. This is assumed
    /// to coincide with the body anchor initially.
    mouseJointDef.target = b2Vec2(0.0f, 0.0f);
    /// The maximum constraint force that can be exerted
    /// to move the candidate body. Usually you will express
    /// as some multiple of the weight (multiplier * mass * gravity).
    mouseJointDef.maxForce = 50.0f * pBodyObj->GetMass();
    /// The damping ratio. 0 = no damping, 1 = critical damping.
    mouseJointDef.dampingRatio = 0.15f;
    /// The response speed.
    mouseJointDef.frequencyHz = 3.5f;

    return (b2MouseJoint *) mWorld.CreateJoint(&mouseJointDef);
}

void PhysicsManager::update() {
    // Clears collision flags.
    int32_t size = mBodies.size();
    for (int i = 0; i < size; ++i) {
        PhysicsCollision *physicsCollision = ((PhysicsCollision *) mBodies[i]->GetUserData());
        physicsCollision->collide = false;
    }

    //Updates simulation
    float timeStep = mTimeManager.elapsed();
    //Take a time step.
    mWorld.Step(timeStep, VELOCITY_ITER, POSITION_ITER);

    //Caches the new state.
    for (int i = 0; i < size; ++i) {
        const b2Vec2 &position = mBodies[i]->GetPosition();
        mLocations[i]->x = position.x * PHYSICS_SCALE;
        mLocations[i]->y = position.y * PHYSICS_SCALE;
    }
}

/// The class manages contact between two shapes. A contact exists for each overlapping
/// AABB in the broad-phase (except if filtered). Therefore a contact object may exist
/// that has no contact points.
void PhysicsManager::BeginContact(b2Contact *pContact) {
    void *userDataA = pContact->GetFixtureA()->GetUserData();
    void *userDataB = pContact->GetFixtureB()->GetUserData();
    if (userDataA != NULL && userDataB != NULL) {
        ((PhysicsCollision *) userDataA)->collide = true;
        ((PhysicsCollision *) userDataB)->collide = true;
    }
}


PhysicsManager::~PhysicsManager() {
    Log::info("Destroying PhysicManager.");
    std::vector<b2Body *>::iterator bodyIt;
    for (bodyIt = mBodies.begin(); bodyIt < mBodies.end(); ++bodyIt) {
        delete (PhysicsCollision *) (*bodyIt)->GetUserData();
    }
}
