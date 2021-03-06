#ifndef __okeya__View__
#define __okeya__View__

#include "cocos2d.h"

#include "Factory.h"
#include "ProtocolAds.h"

class ViewModel;

using namespace cocos2d::plugin;
using namespace cocos2d;

class MyAdsListener : public cocos2d::plugin::AdsListener
{
public:
    virtual void onAdsResult(cocos2d::plugin::AdsResultCode code, const char* msg);
    virtual void onPlayerGetPoints(cocos2d::plugin::ProtocolAds* pAdsPlugin, int points);
};

class View : public Layer
{
public:
    View();
    virtual ~View();
    virtual bool initWithFactory(std::string fname  , Factory<ViewModel>& factory);
    virtual void onExit();
    virtual void onEnter();
    virtual void onEnterTransitionDidFinish();
    ViewModel* getRootViewModel();
    void setName(const std::string& name);
    CREATE_FUNC(View);
protected:
    ViewModel* _pRootViewModel;
    Node* _pRootNode;
    void showAds();
    void hideAds();
private:
    ProtocolAds* _admob;
    TAdsInfo _adInfo;
    void setTouchParticle();
};

#endif /* defined(__okeya__View__) */
