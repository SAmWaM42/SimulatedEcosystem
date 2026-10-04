#ifndef entities
#define entities
#include "../Components/components.h"
#define sight 400 //will replace this with an actual attribute to be passed
class GameStateManager;
class Tag
{public:
    std::string groupTag;
    std::string variantTag;
    std::string Id;
    void setTags(std::string groupTag,std::string variantTag,std::string Id);
    

};
class GameObject
{
    public:
    TransformComponent transform;
    //SpriteRenderer renderer;
    GameStateManager* manager;
    bool isDead;
    Tag tag; 
    virtual void update(float time,float dt);
    virtual void handleCollision(GameObject* other);
    ~GameObject();

};
class Player:public GameObject
{public:
    CollisionBody collider;
    HealthComponent health;
    AttackComponent attack;
    MovementContoller controller;
    StateMachineComponent stateMachine;
    void getInput();
    void update(float time,float dt);

};
class Mob:public GameObject
{public:
    CollisionBody collider;
    HealthComponent health;
    AttackComponent attack;
    MovementContoller controller;
    StateMachineComponent stateMachine;
    Vector3 lastTarget;
    bool lastTargetSet;
    bool viewEnv(float view,GameObject* target);
    void update(float time,float dt);
    void handleCollision(GameObject* other);
   

};
class Projectile:public GameObject
{public:
    CollisionBody collider;
    MovementContoller controller;
    GameObject* owner;
    StateMachineComponent stateMachine;
    float damage;
    float lifetime;//how long the projectile lasts
    float expirytime;//when it will expire
    void update(float time,float dt);
    void handleCollision(GameObject* other);
    void handleHit(std::vector<GameObject*> objects,Vector3 pastPos);

};
class staticObj:public GameObject
{
    public:
    CollisionBody collider;
    void update(float time,float dt);
    void handleCollision(GameObject* other);// might use this for somethinf
};

#endif