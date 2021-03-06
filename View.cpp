#include "View.h"
#include "cocostudio/CocoStudio.h"
#include "Factory.h"
#include "ViewModel.h"
#include "PluginManager.h"
#include "Env.h"

using namespace cocos2d::plugin;
using namespace cocostudio;

View::View()
: _pRootViewModel(nullptr)
, _pRootNode(nullptr)
{
}

View::~View()
{
    if(_pRootViewModel){
        _pRootViewModel->release();
        _pRootViewModel = nullptr;
    }
}

bool View::initWithFactory(const std::string fname, Factory<ViewModel>& factory)
{
    if ( !Layer::init() ) {
        return false;
    }
    auto ext = fname.substr(fname.find_last_of("."));
    if(ext == ".csb"){
        _pRootNode = CSLoader::createNode(fname.c_str());
    }else{
        _pRootNode = SceneReader::getInstance()->createNodeWithSceneFile(fname.c_str());
    }
    setName(fname);
    _pRootViewModel = factory.create("root");
    if(_pRootViewModel == nullptr){
        factory.sign("root", new Creator<ViewModel, ViewModel>);
        _pRootViewModel = factory.create("root");
    }
    _pRootViewModel->retain();
    _pRootViewModel->setRoot(_pRootViewModel);
    _pRootViewModel->setNode(this);
    _pRootViewModel->bind(_pRootNode, factory);
    _pRootViewModel->init();
    addChild(_pRootNode);
//    setTouchParticle();
//    this->schedule(schedule_selector(View::update));
    return true;
}

void View::setTouchParticle()
{
    if(_pRootNode->getChildrenCount() < 1){
        return;
    }
    const auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [&](Touch* pTouch, Event* pEvent){
        const auto location = pTouch->getLocation();
        auto* pParticle = ParticleSystemQuad::create("ui/Particles/Galaxy.plist");
        pParticle->setPosition(location);
        pParticle->setGlobalZOrder(100.0f);
        pParticle->setScale(0.5);
        pParticle->setDuration(0.2f);
        pParticle->setAutoRemoveOnFinish(true);
        addChild(pParticle);
        return true;
    };
    auto pBackground = _pRootNode->getChildren().at(0);
    getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, pBackground);
}

void View::onExit()
{
    Layer::onExit();
    if(_pRootViewModel != nullptr){
        _pRootViewModel->onExit();
    }
}

void View::onEnter()
{
    Layer::onEnter();
    if(_pRootViewModel != nullptr){
        _pRootViewModel->onEnter();
    }
}

void View::onEnterTransitionDidFinish()
{
    Layer::onEnterTransitionDidFinish();
    if(_pRootViewModel != nullptr){
        _pRootViewModel->onEnterTransitionDidFinish();
    }
}

void View::showAds()
{
    auto pluginManager = PluginManager::getInstance();
    
    auto plugin = pluginManager->loadPlugin("AdsAdmob");
    _admob = dynamic_cast<ProtocolAds*>(plugin);
    TAdsDeveloperInfo devInfo;

    devInfo["AdmobID"] = ADMOB_ID;
    _admob->configDeveloperInfo(devInfo);
#ifdef DEBUG
    _admob->setDebugMode(true);
#endif
    _adInfo["AdmobType"] = "1";
    _adInfo["AdmobSizeEnum"] = "1";
    
    _admob->showAds(_adInfo, ProtocolAds::kPosBottom);
}

void View::hideAds()
{
    _admob->hideAds(_adInfo);
}

ViewModel* View::getRootViewModel()
{
    return _pRootViewModel;
}

void View::setName(const std::string& name)
{
    auto start = name.find_last_of("/") + 1;
    auto end = name.find_last_of(".");
    Layer::setName(name.substr(start, end - start));
}

void MyAdsListener::onAdsResult(AdsResultCode code, const char* msg)
{
    log("OnAdsResult, code : %d, msg : %s", code, msg);
}

void MyAdsListener::onPlayerGetPoints(cocos2d::plugin::ProtocolAds* pAdsPlugin, int points)
{
    log("Player get points : %d", points);
    
    // @warning should add code to give game-money to player here
    
    // spend the points of player
    if (pAdsPlugin != NULL) {
        pAdsPlugin->spendPoints(points);
    }
}

