#ifndef components
#define components
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <variant>
#include "uniqueFunctions.h"
#define decelleration 23.105
#define max_acceleration_coefficient 1.5

struct Vector3{float x;float y;float z;};
class GameStateManager;
class GameObject;
class force
{
public:
Vector3 orientation;
float magnitude;
float duration;
float endTime;



};
class TransformComponent
{
public:
    Vector3 position;
    std::vector<force> forces;
    std::vector<force> expiredforces;
    void applyForce(Vector3 orientation,float magnitude,float duration,float endTime);
    void cleanForces(float clockT);
    

};
class CollisionBody
{
public:
    Vector3 offset;
    Vector3 size;
    bool canBeHit;
    std::string colType;//what is  this for
    float iFrameTime;
    float iFrameEndTime;
    void updateCollider(float clockTime);
    void handleCollision(GameObject* owner,GameObject* other);
};
/*class SpriteRenderer
{
public:
    Rectangle bounds;
    std::string currentSprite;
    Vector3 singleFrameSize;
    void updateSprite();
};*/
class MovementContoller
{
public:
    Vector3 velocity;
    Vector3 netVelocity;
    Vector3 lastVelocity;
    float speed;
    void calculateHeading(Vector3 Destination,Vector3 position);
    void move(TransformComponent* component, float dt,float clockT);
};
class AttackComponent
{
    public:
    std::vector<std::string> possibleAttacks;
    Vector3 aimDirection;
    bool canAttack;
    std::string requstedVariant;
    float delay;
    float cooldown;//time that the clock will allow next hit 
    void attack(GameStateManager* manager,GameObject* setpayload);//attacks will be listed as data hence damage values e.t.c. be reasoned out bya amanager who owns the data
    void update(GameStateManager* manager);
};
class HealthComponent
{
    public:
    float healthPool;
    void takeDamage(float);

};
class StateMachineComponent
{
    public:
    enum generalState{ACTIVE,PASSIVE,STUNNED};
    enum innerState{TRACKING,FIND_TARGET,IDLE,CHOOSE_WANDER,WANDER};
    generalState currentState;
    innerState currentInnnerState;
    generalState lastState;
    float stateDelayTimer;

};

#endif