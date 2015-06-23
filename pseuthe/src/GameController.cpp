/*********************************************************************
Matt Marchant 2015
http://trederia.blogspot.com

pseuthe Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include <GameController.hpp>
#include <Scene.hpp>
#include <MessageBus.hpp>
#include <App.hpp>
#include <PhysicsWorld.hpp>
#include <Entity.hpp>
#include <ParticleSystem.hpp>
#include <Util.hpp>
#include <InputComponent.hpp>
#include <BodypartController.hpp>
#include <AnimatedDrawable.hpp>

#include <memory>

namespace
{
    const float partScale = 0.9f;
    const float partPadding = 1.f;
    const float playerSize = 32.f;
    const sf::Vector2f spawnPosition(960.f, 540.f);

    const int maxBodyParts = 6;
    const int maxPlankton = 5;
}

GameController::GameController(Scene& scene, MessageBus& mb, App& app, PhysicsWorld& pw)
    : m_scene               (scene),
    m_messageBus            (mb),
    m_appInstance           (app),
    m_physicsWorld          (pw),
    m_player                (nullptr),
    m_constraintLength      (0.f),
    m_nextPartSize          (0.f),
    m_nextPartScale         (0.f)
{
    //find two off sceen aresa for spawning teh planktons
    auto& worldBounds = m_physicsWorld.getWorldSize();
    m_planktonSpawns[0].left = worldBounds.left;
    m_planktonSpawns[0].width = std::abs(worldBounds.left);
    m_planktonSpawns[0].height = worldBounds.height;

    m_planktonSpawns[1].left = 1920.f;
    m_planktonSpawns[1].width = (worldBounds.width - 1920.f) / 2.f;
    m_planktonSpawns[1].height = worldBounds.height;
}

//public
void GameController::update(float dt)
{

}

void GameController::handleMessage(const Message& msg)
{
    if (msg.type == Message::Type::UI)
    {
        switch (msg.ui.type)
        {
        case Message::UIEvent::MenuClosed:
            if (msg.ui.stateId == States::ID::Menu && !m_player)
            {
                spawnPlayer();
                addBodyPart();
                addBodyPart();
                addBodyPart();
                
                spawnPlankton();
                spawnPlankton();
                spawnPlankton();
                spawnPlankton();
            }
            break;
        case Message::UIEvent::MenuOpened:
            if (m_player)
            {
                
            }
            break;
        default:break;
        }
    }
    else if (msg.type == Message::Type::Player)
    {
        switch (msg.player.action)
        {
        case Message::PlayerEvent::Died:
            m_player = nullptr;
            {
                Message newMessage;
                newMessage.type = Message::Type::UI;
                newMessage.ui.type = Message::UIEvent::RequestState;
                newMessage.ui.stateId = States::ID::Score;
                m_messageBus.send(newMessage);
            }
            break;
        case Message::PlayerEvent::PartRemoved:
            m_playerPhysicsComponents.pop_back();
            m_nextPartSize /= partScale;
            m_nextPartScale /= partScale;
            m_constraintLength /= partScale;
            break;
        default: break;
        }
    }
}

void GameController::spawnPlayer()
{
    auto entity = std::make_unique<Entity>(m_messageBus);
    entity->setWorldPosition(spawnPosition);

    auto cd = std::make_unique<AnimatedDrawable>(m_messageBus, m_appInstance.getTexture("assets/images/player/head.png"));
    cd->loadAnimationData("assets/images/player/head.cra");
    cd->setOrigin(sf::Vector2f(cd->getFrameSize()) / 2.f);
    cd->setBlendMode(sf::BlendAdd);
    cd->play(cd->getAnimations()[0]); //TODO safety check this
    cd->setName("drawable");
    entity->addComponent<AnimatedDrawable>(cd);



    auto physComponent = m_physicsWorld.addBody(playerSize);
    physComponent->setName("control");
    physComponent->setPosition(entity->getWorldPosition());
    m_playerPhysicsComponents.push_back(physComponent.get());
    entity->addComponent<PhysicsComponent>(physComponent);

    auto trailComponent = ParticleSystem::create(Particle::Type::Trail, m_messageBus);
    trailComponent->setTexture(m_appInstance.getTexture("assets/images/particles/circle.png"));
    trailComponent->setParticleSize({ 2.f, 2.f });
    trailComponent->setName("trail");
    entity->addComponent<ParticleSystem>(trailComponent);

    auto sparkle = ParticleSystem::create(Particle::Type::Sparkle, m_messageBus);
    sparkle->setTexture(m_appInstance.getTexture("assets/images/particles/spark.png"));
    sparkle->setName("sparkle");
    sparkle->start(1u, 0.f, 0.5f);
    entity->addComponent<ParticleSystem>(sparkle);

    auto controlComponent = std::make_unique<InputComponent>(m_messageBus);
    entity->addComponent<InputComponent>(controlComponent);

    m_constraintLength = playerSize + (playerSize * partScale) + partPadding;
    m_nextPartSize = playerSize * partScale;
    m_nextPartScale = partScale;
    
    m_player = entity.get();
    m_scene.getLayer(Scene::Layer::FrontMiddle).addChild(entity);
}

void GameController::addBodyPart()
{
    auto bodyPart = std::make_unique<Entity>(m_messageBus);
    bodyPart->setWorldPosition({ spawnPosition.x - (m_constraintLength * m_playerPhysicsComponents.size()), spawnPosition.y });

    auto drawable = std::make_unique<AnimatedDrawable>(m_messageBus, m_appInstance.getTexture("assets/images/player/bodypart01.png"));
    drawable->loadAnimationData("assets/images/player/bodypart01.cra");
    drawable->setOrigin(sf::Vector2f(drawable->getFrameSize()) / 2.f);
    drawable->setBlendMode(sf::BlendAdd);
    drawable->play(drawable->getAnimations()[0], drawable->getFrameCount() / maxBodyParts * m_playerPhysicsComponents.size()); // TODO safety check this
    drawable->setScale(m_nextPartScale, m_nextPartScale);
    drawable->setName("drawable");
    bodyPart->addComponent<AnimatedDrawable>(drawable);

    auto physComponent = m_physicsWorld.attachBody(m_nextPartSize, m_constraintLength, m_playerPhysicsComponents.back());
    physComponent->setPosition(bodyPart->getWorldPosition());
    physComponent->setVelocity(m_playerPhysicsComponents.back()->getVelocity());
    physComponent->setName("control");
    m_playerPhysicsComponents.push_back(physComponent.get());
    bodyPart->addComponent<PhysicsComponent>(physComponent);

    auto sparkle = ParticleSystem::create(Particle::Type::Sparkle, m_messageBus);
    sparkle->setTexture(m_appInstance.getTexture("assets/images/particles/spark.png"));
    sparkle->setName("sparkle");
    sparkle->start(1u, 0.f, 0.5f);
    bodyPart->addComponent<ParticleSystem>(sparkle);

    auto bpCont = std::make_unique<BodypartController>(m_messageBus);
    bodyPart->addComponent<BodypartController>(bpCont);

    m_player->addChild(bodyPart);

    m_nextPartSize *= partScale;
    m_nextPartScale *= partScale;
    m_constraintLength *= partScale;

    Message msg;
    msg.type = Message::Type::Player;
    msg.player.action = Message::PlayerEvent::PartAdded;
    msg.player.mass = m_playerPhysicsComponents.back()->getMass();
    m_messageBus.send(msg);
}

void GameController::spawnPlankton()
{
    //TODO despawn these when moving off screen? Or just keep count
    //and spawn when below X
    //TODO make these triggers only, no collision reaction

    auto& spawnArea = m_planktonSpawns[Util::Random::value(0, m_planktonSpawns.size() - 1)];
    const float posX = spawnArea.left + (spawnArea.width / 2.f);
    const float posY = static_cast<float>(Util::Random::value(0, 1080));

    auto entity = std::make_unique<Entity>(m_messageBus);
    entity->setWorldPosition({ posX, posY });
    auto physComponent = m_physicsWorld.addBody(32.f); //TODO vary size and scale drawable accordingly
    physComponent->setPosition(entity->getWorldPosition());
    entity->addComponent<PhysicsComponent>(physComponent);

    AnimatedDrawable::Ptr ad;
    switch (Util::Random::value(0, 1))
    {
    case 0:
        ad = std::make_unique<AnimatedDrawable>(m_messageBus, m_appInstance.getTexture("assets/images/player/food01_good.png"));
        ad->loadAnimationData("assets/images/player/food01.cra");
        break;
    case 1:
        ad = std::make_unique<AnimatedDrawable>(m_messageBus, m_appInstance.getTexture("assets/images/player/food02_good.png"));
        ad->loadAnimationData("assets/images/player/food02.cra");
        break;
    default: break;
    }
    ad->setBlendMode(sf::BlendAdd);
    ad->setOrigin(sf::Vector2f(ad->getFrameSize()) / 2.f);
    ad->play(ad->getAnimations()[0]);
    entity->addComponent<AnimatedDrawable>(ad);

    //TODO send a spawn message

    m_scene.getLayer(Scene::Layer::FrontRear).addChild(entity);
}