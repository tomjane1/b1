/*

Background music:
Blipotron by Kevin MacLeod (incompetech.com)

 */


#define UFO_SPEED 8

#include "GameLayer.h"



using namespace cocos2d;


GameLayer::~GameLayer () {
    
    
    CC_SAFE_RELEASE(_growBomb);
    CC_SAFE_RELEASE(_rotateSprite);
    CC_SAFE_RELEASE(_shockwaveSequence);
    CC_SAFE_RELEASE(_swingHealth);
    CC_SAFE_RELEASE(_groundHit);
    CC_SAFE_RELEASE(_explosion);
    CC_SAFE_RELEASE(_ufoAnimation);
    CC_SAFE_RELEASE(_blinkRay);
    
    CC_SAFE_RELEASE(_clouds);
    CC_SAFE_RELEASE(_meteorPool);
    CC_SAFE_RELEASE(_healthPool);
    CC_SAFE_RELEASE(_fallingObjects);
	
}

CCScene* GameLayer::scene()
{
    // 'scene' is an autorelease object
    CCScene *scene = CCScene::create();
    
    // 'layer' is an autorelease object
    GameLayer *layer = GameLayer::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool GameLayer::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !CCLayer::init() )
    {
        return false;
    }
    
    //get screen size
	_screenSize = CCDirector::sharedDirector()->getWinSize();
	
    _running = false;
    
    //create game screen elements
    this->createGameScreen();
    
    //create object pools
    this->createPools();
    this->createActions();

    _fallingObjects = CCArray::createWithCapacity(40);
    _fallingObjects->retain();
    
	//listen for touches
    this->setTouchEnabled(true);
    
    //create main loop
    this->schedule(schedule_selector(GameLayer::update));
    
    
    SimpleAudioEngine::sharedEngine()->playBackgroundMusic("background.mp3", true);
    return true;
}


void GameLayer::update (float dt) {
    
    
    if (!_running) return;
    
    int count;
    int i;
    CCSprite * sprite;
    
    //update timers
    _meteorTimer += dt;
    if (_meteorTimer > _meteorInterval) {
        _meteorTimer = 0;
        this->resetMeteor();
    }
    
    _ufoTimer += dt;
    if (_ufoTimer > _ufoInterval) {
        _ufoTimer = 0;
        this->resetUfo();
    }
    
    _healthTimer += dt;
    if (_healthTimer > _healthInterval) {
        _healthTimer = 0;
        this->resetHealth();
    }
    
    _difficultyTimer += dt;
    if (_difficultyTimer > _difficultyInterval) {
        _difficultyTimer = 0;
        this->increaseDifficulty();
    }
    
    
    if (_bomb->isVisible()) {
        if (_bomb->getScale() > 0.3f) {
            if (_bomb->getOpacity() != 255)
                _bomb->setOpacity(255);
        }
    }
    
    CCSprite * ray = (CCSprite *) _ufo->getChildByTag(kSpriteRay);
    if (_ufo->isVisible() && ray->isVisible()) {
        
        if( _ufo->getPositionX() > _screenSize.width * 0.1f
           && _ufo->getPositionX() <= _screenSize.width * 0.9f) {
            changeEnergy(-0.25);
        }
        
    }

    
    float diffx;
    float diffy;
    
    //check collision with shockwave
    if (_shockWave->isVisible()) {
        count = _fallingObjects->count();
        
        for (i = count-1; i >= 0; i--) {
            sprite = (CCSprite *) _fallingObjects->objectAtIndex(i);
            diffx = _shockWave->getPositionX() - sprite->getPositionX();
            diffy = _shockWave->getPositionY() - sprite->getPositionY();
            if (pow(diffx, 2) + pow(diffy, 2) <= pow(_shockWave->boundingBox().size.width * 0.5f, 2)) {
                sprite->stopAllActions();
                sprite->runAction((CCAction *) _explosion->copy()->autorelease());
                SimpleAudioEngine::sharedEngine()->playEffect("boom.wav");
                if (sprite->getTag() == kSpriteMeteor) {
                    _shockwaveHits++;
                    _score += _shockwaveHits * 13 + _shockwaveHits * 2;
                }
                //play sound
                _fallingObjects->removeObjectAtIndex(i);
            }
        }
        if (_ufo->isVisible() && !_ufoKilled) {
            
            diffx = _shockWave->getPositionX() - _ufo->getPositionX();
            diffy = _shockWave->getPositionY() - _ufo->getPositionY();
            if (pow(diffx, 2) + pow(diffy, 2) <= pow(_shockWave->boundingBox().size.width * 0.6f, 2)) {
                _ufoKilled = true;
                SimpleAudioEngine::sharedEngine()->stopAllEffects();
                _ufo->stopAllActions();
                ray->stopAllActions();
                ray->setVisible(false);
                _ufo->runAction((CCAction *) _explosion->copy()->autorelease());
                SimpleAudioEngine::sharedEngine()->playEffect("boom.wav");
                
                 _shockwaveHits++;
                 _score += _shockwaveHits * 13 + _shockwaveHits * 4;
            } 
        }
        
        
        char szValue[100] = {0};
        sprintf(szValue, "%i", _score);
        _scoreDisplay->setString(szValue);
        
    }
    
    //move clouds
    count = _clouds->count();
    for (i = 0; i < count; i++) {
        sprite = (CCSprite *) _clouds->objectAtIndex(i);
        sprite->setPositionX(sprite->getPositionX() + dt * 20);
        if (sprite->getPositionX() > _screenSize.width + sprite->boundingBox().size.width * 0.5f)
            sprite->setPositionX(-sprite->boundingBox().size.width * 0.5f);
    }
        
}

void GameLayer::ccTouchesBegan(CCSet* pTouches, CCEvent* event) {
    
    if (!_running) {
        if (_introMessage->isVisible()) {
            
            _introMessage->setVisible(false);
            this->resetGame();
            
        } else if (_gameOverMessage->isVisible()) {
            
            SimpleAudioEngine::sharedEngine()->stopAllEffects();
            _gameOverMessage->setVisible(false);
            this->resetGame();
            
        }
        return;
    }
	
	CCTouch *touch = (CCTouch *)pTouches->anyObject();
    
    if (touch) {
        
        if (_bomb->isVisible()) {
            
            _bomb->stopAllActions();
            CCSprite * child;
            child = (CCSprite *) _bomb->getChildByTag(kSpriteHalo);
            child->stopAllActions();
            child = (CCSprite *) _bomb->getChildByTag(kSpriteSparkle);
            child->stopAllActions();
            
            if (_bomb->getScale() > 0.25f) {
                
                _shockWave->setScale(0.1f);
                _shockWave->setPosition(_bomb->getPosition());
                _shockWave->setVisible(true);
                _shockWave->runAction(CCScaleTo::create(0.5f, _bomb->getScale() * 2.0f));
                _shockWave->runAction((CCFiniteTimeAction*) _shockwaveSequence->copy()->autorelease());
                SimpleAudioEngine::sharedEngine()->playEffect("bombRelease.wav");

            } else {
                SimpleAudioEngine::sharedEngine()->playEffect("bombFail.wav");
            }
            _bomb->setVisible(false);
            _shockwaveHits = 0;
        
        } else {
            
            CCPoint tap = touch->getLocation();
            _bomb->stopAllActions();
            _bomb->setScale(0.1f);
            _bomb->setPosition(tap);
            _bomb->setVisible(true);
            _bomb->setOpacity(50);
            _bomb->runAction((CCAction *) _growBomb->copy()->autorelease());
            
            CCSprite * child;
            child = (CCSprite *) _bomb->getChildByTag(kSpriteHalo);
            child->runAction((CCAction *) _rotateSprite->copy()->autorelease());
            child = (CCSprite *) _bomb->getChildByTag(kSpriteSparkle);
            child->runAction((CCAction *) _rotateSprite->copy()->autorelease());
        }
	}
}

void GameLayer::fallingObjectDone (CCNode* pSender) {
    
    
    _fallingObjects->removeObject(pSender);
    pSender->stopAllActions();
    pSender->setRotation(0);
    
    if (pSender->getTag() == kSpriteMeteor) {
        changeEnergy(-15);
        pSender->runAction((CCAction*)_groundHit->copy()->autorelease());
        //play explosion sound
        SimpleAudioEngine::sharedEngine()->playEffect("boom.wav");
    } else {
        pSender->setVisible(false);

        if (_energy == 100) {
            
            _score += 25;
            char score[100] = {0};
            sprintf(score, "%i", _score);
            _scoreDisplay->setString(score);
            
        } else {
            changeEnergy(10);
        }
        //play health bonus sound
        SimpleAudioEngine::sharedEngine()->playEffect("health.wav");
    }
   
    
    
}

void GameLayer::animationDone (CCNode* pSender) {
    pSender->setVisible(false);
}


void GameLayer::shockwaveDone() {
    _shockWave->setVisible(false);
}


void GameLayer::resetMeteor(void) {
    
    if (_fallingObjects->count() > 30) return;
    
    CCSprite * meteor = (CCSprite *) _meteorPool->objectAtIndex(_meteorPoolIndex);
	_meteorPoolIndex++;
	if (_meteorPoolIndex == _meteorPool->count()) _meteorPoolIndex = 0;
	
    
	int meteor_x = rand() % (int) (_screenSize.width * 0.8f) + _screenSize.width * 0.1f;
    int meteor_target_x = rand() % (int) (_screenSize.width * 0.8f) + _screenSize.width * 0.1f;
    
    meteor->stopAllActions();
    meteor->setPosition(ccp(meteor_x, _screenSize.height + meteor->boundingBox().size.height * 0.5));
	
    //create action
    CCActionInterval*  rotate = CCRotateBy::create(0.5f ,  -90);
    CCAction*  repeatRotate = CCRepeatForever::create( rotate );
    CCFiniteTimeAction*  sequence = CCSequence::create(
               CCMoveTo::create(_meteorSpeed, ccp(meteor_target_x, _screenSize.height * 0.15f)),
               CCCallFuncN::create(this, callfuncN_selector(GameLayer::fallingObjectDone)),
               NULL);
    
    meteor->setVisible ( true );
    meteor->runAction(repeatRotate);
    meteor->runAction(sequence);
    _fallingObjects->addObject(meteor);
    
}

void GameLayer::resetUfo(void) {
    
    if (_ufo->isVisible()) return;
    
    float newX;
    float newY;
    
    //pick side
    if (rand() % 100 < 50) {
        newX = 0.0;
    } else {
        newX = _screenSize.width;
    }
    
    //pick height
    newY = rand() % (int) (_screenSize.height * 0.3f) + _screenSize.height * 0.3f;
    if (newY > _screenSize.height * 0.7f) newY = _screenSize.height * 0.7f;
    
    _ufo->stopAllActions();
    _ufo->setPosition(ccp(newX, newY));
    _ufo->runAction((CCAction *) _ufoAnimation->copy()->autorelease());
    
    //set ray
    CCSprite *ray = (CCSprite *) _ufo->getChildByTag(kSpriteRay);
    ray->setVisible(false);
    ray->stopAllActions();
    ray->runAction((CCAction *) _blinkRay->copy()->autorelease());
    
    CCFiniteTimeAction *  sequence;
    if (newX == 0.0) {
        sequence = CCSequence::create ( CCMoveTo::create(UFO_SPEED, ccp(_screenSize.width * 1.1, newY)),
                                       CCCallFuncN::create(this, callfuncN_selector(GameLayer::animationDone)),
                                       NULL);
    } else {
        sequence = CCSequence::create ( CCMoveTo::create(UFO_SPEED, ccp(-_screenSize.width * 0.1, newY)),
                                       CCCallFuncN::create(this, callfuncN_selector(GameLayer::animationDone)),
                                       NULL);
    }
    
    _ufo->setVisible(true);
    _ufo->runAction(sequence);
    _ufoKilled = false;
    SimpleAudioEngine::sharedEngine()->playEffect("pew.wav", true);
    
    
}


void GameLayer::resetHealth(void) {
    
    if (_fallingObjects->count() > 30) return;
    
    CCSprite * health = (CCSprite *) _healthPool->objectAtIndex(_healthPoolIndex);
	_healthPoolIndex++;
	if (_healthPoolIndex == _healthPool->count()) _healthPoolIndex = 0;
	
    
	int health_x = rand() % (int) (_screenSize.width * 0.8f) + _screenSize.width * 0.1f;
    int health_target_x = rand() % (int) (_screenSize.width * 0.8f) + _screenSize.width * 0.1f;
    
    health->stopAllActions();
    health->setPosition(ccp(health_x, _screenSize.height + health->boundingBox().size.height * 0.5));
	
    //create action
    CCFiniteTimeAction*  sequence = CCSequence::create(
           CCMoveTo::create(_healthSpeed, ccp(health_target_x, _screenSize.height * 0.15f)),
           CCCallFuncN::create(this, callfuncN_selector(GameLayer::fallingObjectDone)),
           NULL);
    
    health->setVisible ( true );
    health->runAction((CCAction *)_swingHealth->copy()->autorelease());
    health->runAction(sequence);
    _fallingObjects->addObject(health);
}

void GameLayer::changeEnergy(float value) {
    _energy += value;
    if (_energy <= 0) {
        _energy = 0;
        this->stopGame();
        SimpleAudioEngine::sharedEngine()->playEffect("fire_truck.wav");
        //show GameOver
        _gameOverMessage->setVisible(true);
    }
    
    if (_energy > 100) _energy = 100;
    
    char szValue[100] = {0};
    sprintf(szValue, "%i", (int) _energy);
    _energyDisplay->setString(szValue);
    
    
}

void GameLayer::resetGame(void) {
    
    _score = 0;
    _energy = 100;
    
    _meteorInterval = 3.5;
	_meteorTimer = _meteorInterval * 0.99f;
	_meteorSpeed = 10;//in seconds to reach ground
    
    _ufoInterval = 20;
    _ufoTimer = -_ufoInterval;
    _ufoKilled = false;
    
    _healthInterval = 25;
    _healthTimer = -_healthInterval;
    _healthSpeed = 15;//in seconds to reach ground
    
    _difficultyInterval = 60;
    _difficultyTimer = 0;
    
    _running = true;
    
    char energy[100] = {0};
    sprintf(energy, "%i%s", (int) _energy, "%");
    _energyDisplay->setString(energy);
    
    char score[100] = {0};
    sprintf(score, "%i", _score);
    _scoreDisplay->setString(score);
    
}

void GameLayer::increaseDifficulty () {

    
    _meteorInterval -= 0.15f;
    if (_meteorInterval < 0.25f) _meteorInterval = 0.25f;
    _meteorSpeed -= 1;
    if (_meteorSpeed < 4) _meteorSpeed = 4;
    
    _ufoInterval -= 0.1f;
    if (_ufoInterval < 5) _ufoInterval = 5;
    
    _healthSpeed -= 1;
    if (_healthSpeed < 8) _healthSpeed = 8;
    
}


void GameLayer::stopGame() {
    
    _running = false;
    //stop all actions currently running (meteors, health drops, animations,
    int i;
    int count;
    CCSprite * sprite;
    count = _fallingObjects->count();
    for (i = count-1; i >= 0; i--) {
        sprite = (CCSprite *) _fallingObjects->objectAtIndex(i);
        sprite->stopAllActions();
        sprite->setVisible(false);
        _fallingObjects->removeObjectAtIndex(i);
    }
    if (_bomb->isVisible()) {
        _bomb->stopAllActions();
        _bomb->setVisible(false);
        CCSprite * child;
        child = (CCSprite *) _bomb->getChildByTag(kSpriteHalo);
        child->stopAllActions();
        child = (CCSprite *) _bomb->getChildByTag(kSpriteSparkle);
        child->stopAllActions();
    }
    if (_shockWave->isVisible()) {
        _shockWave->stopAllActions();
        _shockWave->setVisible(false);
    }
    if (_ufo->isVisible()) {
        _ufo->stopAllActions();
        _ufo->setVisible(false);
        CCSprite * ray = (CCSprite *) _ufo->getChildByTag(kSpriteRay);
        ray->stopAllActions();
        ray->setVisible(false);
    }

}

void GameLayer::createGameScreen() {

    //add bg
    CCSprite * bg = CCSprite::create("bg.png");
    bg->setPosition(ccp(_screenSize.width * 0.5f, _screenSize.height * 0.5f));
    this->addChild(bg);
    
    
    //create spritebatch node
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("sprite_sheet.plist");
    _gameBatchNode = CCSpriteBatchNode::create("sprite_sheet.png");
    this->addChild(_gameBatchNode);

    
    //create cityscape
    CCSprite * sprite;
    for (int i = 0; i < 2; i++) {
        sprite = CCSprite::createWithSpriteFrameName("city_dark.png");
        sprite->setPosition(ccp(_screenSize.width * (0.25f + i * 0.5f),
                                sprite->boundingBox().size.height * 0.5f));
        _gameBatchNode->addChild(sprite, kForeground);
        
        sprite = CCSprite::createWithSpriteFrameName("city_light.png");
        sprite->setPosition(ccp(_screenSize.width * (0.25f + i * 0.5f),
                                sprite->boundingBox().size.height * 0.9f));
        _gameBatchNode->addChild(sprite, kBackground);
    }
    
    //add trees
    for (int i = 0; i < 3; i++) {
        sprite = CCSprite::createWithSpriteFrameName("trees.png");
        sprite->setPosition(ccp(_screenSize.width * (0.2f + i * 0.3f),
                                sprite->boundingBox().size.height * 0.5f));
        _gameBatchNode->addChild(sprite, kForeground);
        
    }
    
    
    //add HUD
    _scoreDisplay = CCLabelBMFont::create("0", "font.fnt", _screenSize.width * 0.3f);
    _scoreDisplay->setAnchorPoint(ccp(1,0.5));
    _scoreDisplay->setPosition(ccp(_screenSize.width * 0.8f, _screenSize.height * 0.94f));
    this->addChild(_scoreDisplay);
    
    _energyDisplay = CCLabelBMFont::create("100%", "font.fnt", _screenSize.width * 0.1f, kCCTextAlignmentRight);
    _energyDisplay->setPosition(ccp(_screenSize.width * 0.3f, _screenSize.height * 0.94f));
    this->addChild(_energyDisplay);
    
    CCSprite * icon = CCSprite::createWithSpriteFrameName("health_icon.png");
    icon->setPosition( ccp(_screenSize.width * 0.15f,  _screenSize.height * 0.94f) );
    _gameBatchNode->addChild(icon, kBackground);
    
    //add clouds
    CCSprite * cloud;
    _clouds = CCArray::createWithCapacity(4);
    _clouds->retain();
    float cloud_y;
    for (int i = 0; i < 4; i++) {
        cloud_y = i % 2 == 0 ? _screenSize.height * 0.4f : _screenSize.height * 0.5f;
        cloud = CCSprite::createWithSpriteFrameName("cloud.png");
        cloud->setPosition(ccp (_screenSize.width * 0.1f + i * _screenSize.width * 0.3f,  cloud_y));
        _gameBatchNode->addChild(cloud, kBackground);
        _clouds->addObject(cloud);
    }
    
    //CREATE BOMB SPRITE
    _bomb = CCSprite::createWithSpriteFrameName("bomb.png");
    _bomb->getTexture()->generateMipmap();
    _bomb->setVisible(false);
    
    CCSize size = _bomb->boundingBox().size;
    
    //add sparkle
    CCSprite * sparkle = CCSprite::createWithSpriteFrameName("sparkle.png");
    sparkle->setPosition(ccp(size.width * 0.72f, size.height * 0.72f));
    _bomb->addChild(sparkle, kMiddleground, kSpriteSparkle);
    
    //add halo
    CCSprite * halo = CCSprite::createWithSpriteFrameName("halo.png");
    halo->setPosition(ccp(size.width * 0.4f, size.height * 0.4f));
    _bomb->addChild(halo, kMiddleground, kSpriteHalo);
    
    _gameBatchNode->addChild(_bomb, kForeground);
    
    //add shockwave
    _shockWave = CCSprite::createWithSpriteFrameName("shockwave.png");
    _shockWave->getTexture()->generateMipmap();
    _shockWave->setVisible(false);
    _gameBatchNode->addChild(_shockWave);
    
    
    //intro message
    _introMessage = CCSprite::createWithSpriteFrameName("logo.png");
    _introMessage->setPosition(ccp(_screenSize.width * 0.5f, _screenSize.height * 0.6f));
    _introMessage->setVisible(true);
    this->addChild(_introMessage, kForeground);
    
    //game over message
    _gameOverMessage = CCSprite::createWithSpriteFrameName("gameover.png");
    _gameOverMessage->setPosition(ccp(_screenSize.width * 0.5f, _screenSize.height * 0.65f));
    _gameOverMessage->setVisible(false);
    this->addChild(_gameOverMessage, kForeground);
    
    
        
}

void GameLayer::createPools() {

    CCSprite * sprite;
    int i;

    //create meteor pool
    _meteorPool = CCArray::createWithCapacity(50);
    _meteorPool->retain();
    _meteorPoolIndex = 0;
    for (i = 0; i < 50; i++) {
        sprite = CCSprite::createWithSpriteFrameName("meteor.png");
        sprite->setVisible(false);
        _gameBatchNode->addChild(sprite, kMiddleground, kSpriteMeteor);
        _meteorPool->addObject(sprite);
    }
    
    //create health pool
    _healthPool = CCArray::createWithCapacity(20);
    _healthPool->retain();
    _healthPoolIndex = 0;
    for (i = 0; i < 20; i++) {
        sprite = CCSprite::createWithSpriteFrameName("health.png");
        sprite->setVisible(false);
        sprite->setAnchorPoint(ccp(0.5f, 0.8f));
        _gameBatchNode->addChild(sprite, kMiddleground, kSpriteHealth);
        _healthPool->addObject(sprite);
    }
    
}


void GameLayer::createActions() {
    //actions
    CCFiniteTimeAction* easeSwing = CCSequence::create(
                   CCEaseInOut::create(CCRotateTo::create(1.2f, -10), 2),
                   CCEaseInOut::create(CCRotateTo::create(1.2f, 10), 2),
                   NULL);
    _swingHealth = CCRepeatForever::create( (CCActionInterval*) easeSwing );
    _swingHealth->retain();
    
    
    _shockwaveSequence = CCSequence::create(
                    CCFadeOut::create(1.0f),
                    CCCallFunc::create(this, callfunc_selector(GameLayer::shockwaveDone)),
                    NULL);
    _shockwaveSequence->retain();
    
    
    _growBomb = CCScaleTo::create(4.0f, 1);
    _growBomb->retain();
    
    
    CCActionInterval*  rotate = CCRotateBy::create(0.5f ,  -90);
    _rotateSprite = CCRepeatForever::create( rotate );
    _rotateSprite->retain();
    
    
    //animations
    CCAnimation* animation;
    animation = CCAnimation::create();
    CCSpriteFrame * frame;
    int i;
    for(i = 1; i <= 10; i++) {
        char szName[100] = {0};
        sprintf(szName, "boom%i.png", i);
        frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(szName);
        animation->addSpriteFrame(frame);
    }
    
    animation->setDelayPerUnit(1 / 10.0f);
    animation->setRestoreOriginalFrame(true);
    _groundHit = CCSequence::create(
                CCMoveBy::create(0, ccp(0,_screenSize.height * 0.12f)),
                CCAnimate::create(animation),
                CCCallFuncN::create(this, callfuncN_selector(GameLayer::animationDone)),
                NULL
                );
   _groundHit->retain();

    animation = CCAnimation::create();
    for(i = 1; i <= 7; i++) {
        char szName[100] = {0};
        sprintf(szName, "explosion_small%i.png", i);
        frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(szName);
        animation->addSpriteFrame(frame);
    }
    
    animation->setDelayPerUnit(0.5 / 7.0f);
    animation->setRestoreOriginalFrame(true);
    _explosion = CCSequence::create(
            CCAnimate::create(animation),
            CCCallFuncN::create(this, callfuncN_selector(GameLayer::animationDone)),
            NULL
            );
    ;
    _explosion->retain();
    
    //add ufo
    _ufo = CCSprite::createWithSpriteFrameName("ufo_1.png");
    animation = CCAnimation::create();
    for(i = 1; i <= 4; i++) {
        char szName[100] = {0};
        sprintf(szName, "ufo_%i.png", i);
        frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(szName);
        animation->addSpriteFrame(frame);
    }
    
    animation->setDelayPerUnit(1.0 / 4.0f);
    animation->setLoops(-1);
    _ufoAnimation = CCAnimate::create(animation);
    _ufoAnimation->retain();
    
    CCSprite * ray = CCSprite::createWithSpriteFrameName("ray.png");
    ray->setAnchorPoint(ccp(0.5, 1.0));
    ray->setPosition(ccp(_ufo->boundingBox().size.width * 0.52f, _ufo->boundingBox().size.height * 0.5f));
    
    _blinkRay = CCRepeatForever::create(CCBlink::create(4, 6));
    _blinkRay->retain();
    _ufo->addChild(ray, -1, kSpriteRay);
    
    _ufo->setVisible(false);
    _gameBatchNode->addChild(_ufo, kMiddleground, kSpriteUfo);

}
