#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

// A custom class to handle the bullet movement and hitboxes
class BulletNode : public CCNode {
public:
    CCSprite* m_sprite;
    
    static BulletNode* create() {
        auto ret = new BulletNode();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        // Use a standard circle/particle texture for the bullet
        m_sprite = CCSprite::createWithSpriteFrameName("gj_commentClick_001.png");
        if (!m_sprite) return false;
        
        m_sprite->setScale(1.5f);
        m_sprite->setColor(ccCOLOR3B{255, 50, 50}); // Red laser color
        this->addChild(m_sprite);
        return true;
    }

    void update(float dt) override {
        // Fly straight to the right rapidly across the screen
        this->setPositionX(this->getPositionX() + 700.0f * dt);
        
        auto playLayer = PlayLayer::get();
        if (!playLayer) return;

        auto bulletBox = m_sprite->boundingBox();
        // Convert local bounding box to global world space coordinates
        bulletBox.origin = this->getParent()->convertToWorldSpace(this->getPosition());

        // Scan through all active obstacles currently on screen
        if (playLayer->m_objects) {
            CCObject* obj;
            CCARRAY_FOREACH(playLayer->m_objects, obj) {
                auto gameObject = typeinfo_cast<GameObject*>(obj);
                if (!gameObject || !gameObject->isVisible()) continue;

                // Check if the bullet intersects the obstacle's hitbox
                if (bulletBox.intersectsRect(gameObject->boundingBox())) {
                    bool shouldDestroy = true;

                    // If it is a coin, check if the user enabled the "Shoot Coins" setting toggle
                    if (gameObject->m_objectID == 142 || gameObject->m_objectID == 1329) {
                        shouldDestroy = Mod::get()->getSettingValue<bool>("shoot-coins");
                    }

                    if (shouldDestroy) {
                        // Blast it out of existence! Hide it and turn off its collision
                        gameObject->setVisible(false);
                        gameObject->deactivateObject();
                        
                        // Delete the bullet so it doesn't pierce forever
                        this->removeFromParentAndCleanup(true);
                        return;
                    }
                }
            }
        }

        // Clean up the bullet if it flies completely off the right edge of the screen
        if (this->getPositionX() > 2000.0f) {
            this->removeFromParentAndCleanup(true);
        }
    }
};

// Hook directly into the level gameplay mechanics
class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontRunActions) {
        if (!PlayLayer::init(level, useReplay, dontRunActions)) return false;
        
        // Schedule updates so our key listening engine stays active
        this->scheduleUpdate();
        return true;
    }

    void update(float dt) override {
        PlayLayer::update(dt);

        // Check if the player taps the 'Q' key on their keyboard
        if (CCKeyboardDispatcher::get()->getDeltaKeyState(enumKeyCodes::KEY_Q)) {
            // Find the player's position
            if (this->m_player1) {
                auto bullet = BulletNode::create();
                bullet->setPosition(this->m_player1->getPosition());
                
                // Add the bullet to the active map layer and turn on its physics framework
                this->m_objectLayer->addChild(bullet);
                bullet->scheduleUpdate();
            }
        }
    }
};

