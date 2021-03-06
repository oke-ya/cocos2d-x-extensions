#include "ViewModel.h"
#include "ui/CocosGUI.h"
#include "SceneManager.h"
#include "SupportFunctions.h"
#include "editor-support/cocostudio/CocoStudio.h"
#include "BGMPlayer.h"

using namespace experimental;
using namespace cocos2d::ui;

const std::string ViewModel::LabelPrefix  = "s_";
const std::string ViewModel::SpritePrefix = "i_";
const std::string ViewModel::ProgressPrefix = "p_";
const std::string ViewModel::LinkPhysicsPrefix = "g_";
const std::vector<std::string> ViewModel::ObserveSuffixes = {"Layer", "Area", "Animation", "View"};
const std::vector<std::string> ViewModel::ObservePrefixes = {"Character"};

const int ViewModel::DefaultSize = 1000;

std::stack<Node*> ViewModel::invisibleNodes;

ViewModel::ViewModel()
: _pParent(nullptr)
, _moved(false)
, _name("")
, _pRoot(nullptr)
, _pNode(nullptr)
{
}

ViewModel::~ViewModel()
{
    _children.clear();
    _watches.clear();
    _pParent = nullptr;
    _pNode = nullptr;
}

bool ViewModel::init()
{
    return true;
}

void ViewModel::setRoot(ViewModel* pVm)
{
    _pRoot = pVm;
}

void ViewModel::setParent(ViewModel* pParent)
{
    _pParent = pParent;
}

ViewModel* ViewModel::getParent()
{
    return _pParent;
}

void ViewModel::setNode(Node* pNode)
{
    _pNode = pNode;
}

Vector<ViewModel*>& ViewModel::getChildren()
{
    return _children;
}

void ViewModel::observeEvent()
{
    auto* listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [&](Touch* pTouch, Event* pEvent){
        auto pTarget = pEvent->getCurrentTarget();
        auto pWidget = static_cast<Widget*>(pTarget);
        if(!pWidget->isEnabled()){
            log("%s is disabled.", pTarget->getName().c_str());
            return false;
        }
        if(isRectContains(pTarget, pTouch->getLocation())){
            return onTouchBegan(pTouch, pEvent);
        }
        return false;
    };
    listener->onTouchMoved = [&](Touch* pTouch, Event* pEvent){
        onTouchMoved(pTouch, pEvent);
    };
    listener->onTouchEnded = [&](Touch* pTouch, Event* pEvent){
        onTouchEnded(pTouch, pEvent);
    };
    auto pWidget = dynamic_cast<Widget*>(_pNode);
    if(pWidget){
        pWidget->setTouchEnabled(false);
    }
    Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, _pNode);
}

bool ViewModel::isRectContains(Node* pNode, const Vec2& point)
{
    auto touchLocation = pNode->convertToNodeSpace(point);
    auto s = pNode->getContentSize();
    auto rect = Rect(0, 0, s.width, s.height);
    return rect.containsPoint(touchLocation);
    if(static_cast<Widget*>(pNode)->getRotation() == 0){
        if(rect.containsPoint(touchLocation)){
            return true;
        }else{
            return false;
        }
    }
//
//    auto size = pNode->getContentSize();
//    Point v[4] = {
//        Point(0,0),
//        Point(0, size.height),
//        Point(size.width, size.height),
//        Point(size.width, 0),
//    };
//    
//    auto affine = pNode->getNodeToParentAffineTransform();
//    auto name = pNode->getName();
//    for(int i = 0; i < 4; i++) {
//        v[i] = PointApplyAffineTransform(v[i], affine);
//    }
//    Point vec[4] = {
//        v[1] - v[0],
//        v[2] - v[1],
//        v[3] - v[2],
//        v[0] - v[3],
//    };
//    
//    Point vecP[4] = {
//        v[0] - touchLocation,
//        v[1] - touchLocation,
//        v[2] - touchLocation,
//        v[3] - touchLocation,
//    };
//    
//    for(int i = 0; i < 4; i++){
//        if((vec[i].x * vecP[i].y - vec[i].y * vecP[i].x) < 0){
//            return false;
//        }
//    }
    return true;
}

bool ViewModel::onTouchBegan(Touch* pTouch, Event* pEvent){
    log("ViewModel::onTouchBegan, %s", pEvent->getCurrentTarget()->getName().c_str());
    return false;
}

void ViewModel::onTouchMoved(Touch* pTouch, Event* pEvent){
    log("ViewModel::onTouchMoved, %s", pEvent->getCurrentTarget()->getName().c_str());
}

void ViewModel::onTouchEnded(Touch* pTouch, Event* pEvent){
    log("ViewModel::onTouchEnded, %s", pEvent->getCurrentTarget()->getName().c_str());
}

const Rect ViewModel::getBoundingBox() const
{
    return _pNode->getBoundingBox();
}

void ViewModel::appendNode(Node* pNode)
{
    _pNode->getParent()->addChild(pNode);
}

void ViewModel::bind(Node* pNode, Factory<ViewModel>& factory)
{
    if(pNode == nullptr){
        return;
    }
    _pRoot->fixName(pNode);
    auto description =  pNode->getDescription();
    if(description == "ListView" || description == "ScrollView"){
        jumpToTop(static_cast<ScrollView*>(pNode));
    }
    static const std::string instancePrefix = "c_";
    static const std::string togglePrefix = "t_";
    static const std::string linkPrefix = "l_";

    static const std::string animationPrefix = "a_";
    static const std::string watchPrefix = "w_";
    static const std::string sep = "_";
    static const std::string parentSep = ".";
    const std::string parentName = pNode->getName();

    auto const name = pNode->getName();
    const auto desc = pNode->getDescription();
    if(desc == "ListView" || desc == "ScrollView"){
        auto height = 0;
        auto& children = pNode->getChildren();
        for(auto& child: children){
            height += child->getContentSize().height;
        }
        static_cast<ScrollView*>(pNode)->setInnerContainerPosition(Vec2(0, -height));
    }
//    log("%s, %x", name.c_str(), pNode->isVisible());
    auto prefix = name.substr(0, 2);
    if(prefix == instancePrefix){
        auto i = name.find_last_of(sep);
        if(i < 2){
            i = -1;
        }
        auto const signName = name.substr(0, i);
        bindInstance(factory, pNode, signName);
        return;
    }else if(prefix == togglePrefix){
        bindToggle(factory, pNode);
    }else if(prefix == linkPrefix){
        bindLink(factory, pNode);
    }else if(prefix == ViewModel::LinkPhysicsPrefix){
        bindLink(factory, pNode);
    }else if(prefix == watchPrefix || prefix == ViewModel::ProgressPrefix){
        _watches.insert({name, pNode});
    }else if(prefix == ViewModel::LabelPrefix  ||
             prefix == ViewModel::SpritePrefix ||
             prefix == animationPrefix){
        
        if(prefix == ViewModel::LabelPrefix){
            auto pText = dynamic_cast<Text*>(pNode);
            if(pText && pText->getType() == Text::Type::TTF){
                pText->enableShadow(Color4B::BLACK, Size(2, -2));
//                pText->enableOutline(Color4B(68, 34, 0, 255), 1);
            }
        }
        
        auto posItr = _watches.find(name);
        if(posItr == _watches.end()){
            _watches.insert({name, pNode});
        }else{
            _watches.insert({pNode->getParent()->getName() + "." + name, pNode});
            auto pExisted = std::get<1>(*posItr);
            auto k = pExisted->getParent()->getName() + "." + pExisted->getName();
            auto posItr2 = _watches.find(k);
            if(posItr2 == _watches.end()){
                _watches.insert({k, pExisted});
            }
        }
    }else{
        for(auto& observeSuffix: ViewModel::ObserveSuffixes){
            auto i = name.find_last_of(observeSuffix.at(0));
            if(i == std::string::npos){
                continue;
            }
            std::string suffix = name.substr(i, -1);
            if(suffix == observeSuffix){
                _watches.insert({name, pNode});
                break;
            }
        }
        for(auto& observePrefix: ViewModel::ObservePrefixes){
            std::string prefix = name.substr(0, observePrefix.size());
            if(prefix == observePrefix){
                _watches.insert({name, pNode});
                break;
            }
        }
    }
    auto children = pNode->getChildren();
    for(auto& pChild: children){
        bind(pChild, factory);
    }
}

ViewModel* ViewModel::bindInstance(Factory<ViewModel>& factory, Node* pNode, const std::string& name, bool customEventDispatcher )
{
    auto* pViewModel = factory.create(name);
    CCASSERT(pViewModel, ("factory " + name + " is not defined").c_str());
    pViewModel->setName(pNode->getName());
    pViewModel->setParent(this);
    pViewModel->setRoot(_pRoot);
    pViewModel->setNode(pNode);
    auto children = pNode->getChildren();
    for(auto& pChild: children){
        pViewModel->bind(pChild, factory);
    }
    if(customEventDispatcher){
        pViewModel->observeEvent();
    }
    pViewModel->init();
    this->getChildren().pushBack(pViewModel);
    return pViewModel;
}

void ViewModel::removeFromParent(ViewModel* pViewModel)
{
    auto& children = getChildren();
    ssize_t index = children.getIndex(pViewModel);
    if( index != CC_INVALID_INDEX ){
        pViewModel->getNode()->removeFromParent();
        pViewModel->setParent(nullptr);
        pViewModel->release();
        children.erase(index);
    }

}

void ViewModel::bindToggle(Factory<ViewModel>& factory, Node* pNode)
{
    auto pWidget = static_cast<Widget*>(pNode);
    pWidget->addTouchEventListener([&](Ref* pRef, Widget::TouchEventType type){
        if(type == Widget::TouchEventType::BEGAN){
            BGMPlayer::play2d("sound/se_button_push_01.mp3");
            auto pNode = static_cast<Node*>(pRef);
            auto name = pNode->getName().substr(2, -1);
            auto pTarget = getNode(name);
            toggle(pTarget);
        }
    });
    pWidget->setSwallowTouches(true);
}

void ViewModel::bindLink(Factory<ViewModel>& factory, Node* pNode)
{
    auto pWidget = static_cast<Widget*>(pNode);
    pWidget->addTouchEventListener([&](Ref* pRef, Widget::TouchEventType type){
        if (type == Widget::TouchEventType::BEGAN) {
            auto pNode = static_cast<Node*>(pRef);
            pNode->setScale(1.25f);
        }
        if (type == Widget::TouchEventType::CANCELED) {
            auto pNode = static_cast<Node*>(pRef);
            pNode->setScale(1.0f);
        }
        if (type == Widget::TouchEventType::ENDED) {
            disableTouch();
            BGMPlayer::play2d("sound/se_button_push_01.mp3");
            auto pNode = static_cast<Node*>(pRef);
            auto prefix = pNode->getName().substr(0, 2);
            auto name = pNode->getName().substr(2, -1);
            Scene* scene;
            if(prefix == ViewModel::LinkPhysicsPrefix){
                scene = SceneManager::getInstance()->get(name, -1.0f);
            }else{
                scene = SceneManager::getInstance()->get(name);
            }
            Director::getInstance()->replaceScene(scene);
        }
    });
}

Node* ViewModel::getNode(const std::string& name)
{
    auto pNode = _watches[name];
    if(pNode == nullptr){
        std::queue<Node*> targets;
        Node* pNode = _pRoot->getNode();
        targets.push(pNode);
        while(targets.size() > 0){
            auto children = pNode->getChildren();
            for(auto pChild: children){
                if(pChild->getName() == name){
                    _watches[name] = pChild;
                    return pChild;
                }else{
                    targets.push(pChild);
                }
            }
            targets.pop();
            pNode = targets.front();
        }
    }
    return pNode;
}

Node* ViewModel::getNode()
{
    return _pNode;
}

void ViewModel::set(const std::string& name, const std::string& value){
    auto pos = name.find_first_of(".");
    std::string prefix;
    if(pos != std::string::npos){
        prefix = name.substr(pos + 1, 2);
    }else{
        prefix = name.substr(0, 2);
    }
    auto pNode = getNode(name);
    if(pNode == nullptr){
        auto pParent = getParent();
        if(pParent == nullptr){
            auto children = getChildren();
            for(auto child: children){
                pNode = child->getNode(name);
                if(pNode){ break; }
            }
        }else{
            pNode = getParent()->getNode(name);
        }
        
    }
    CCASSERT(pNode, ("Invalid node name " + name).c_str());
    if(prefix == ViewModel::SpritePrefix){
        auto pSprite = static_cast<ImageView*>(pNode);

        if(value == ""){
            pSprite->setVisible(false);
        }else{
            if(SpriteFrameCache::getInstance()->getSpriteFrameByName(value)){
                pSprite->loadTexture(value, ImageView::TextureResType::PLIST);
            }else{
                pSprite->loadTexture(value, ImageView::TextureResType::LOCAL);
            }
            pSprite->setVisible(true);
        }
    }else if(prefix == ViewModel::LabelPrefix){
        auto bmFont = dynamic_cast<TextBMFont*>(pNode);
        if(bmFont){
            bmFont->setString(value);
        }else{
            auto pText = static_cast<Text*>(pNode);
            pText->setString(value);
        }
    }else if(prefix == ViewModel::ProgressPrefix){
        static_cast<LoadingBar*>(pNode)->setPercent(std::stoi(value));
    }else{
        auto visible = value == "0" ? false : true;
        pNode->setVisible(visible);
    }
}

void ViewModel::set(const std::string& name, int value)
{
    set(name, supportfunctions::to_string(value));
}

int ViewModel::get(const std::string& name)
{
    auto node = getNode(name);
    auto bmText = dynamic_cast<TextBMFont*>(node);
    std::string str;
    if(bmText){
        str = bmText->getString();
    }else{
        str = static_cast<Text*>(node)->getString();

    }
    return std::stoi(str);
}

void ViewModel::setSize(const std::string& name, const int value)
{
    if(value > 0){
        auto pSprite = static_cast<Sprite*>(_watches[name]);
        auto f = float(value) / ViewModel::DefaultSize;
        pSprite->setScale(f);
    }
}

void ViewModel::setList(Factory<ViewModel>& factory, ValueVector* pVec, const ListConfig& config)
{
    auto pLayer = static_cast<cocos2d::ui::ScrollView*>(getNode(config.contentAreaName));
    auto children = pLayer->getChildren();
    for(auto& li : children){
        li->removeFromParent();
    }
    CCASSERT(pLayer, "Missing ScrollArea");
    auto fname = "ui/" + config.listTemplate  + ".ExportJson";
    auto pTmpl = static_cast<Layout*>(cocostudio::GUIReader::getInstance()->widgetFromJsonFile(fname.c_str()));
    pTmpl->setAnchorPoint(Vec2(0.5f, 0.5f));
    
    int i = 0;
    int y = config.origin.y;
    for(auto& values: (*pVec)){
        auto pClone = pTmpl->clone();
        auto name = "c_li_" + supportfunctions::to_string(i);
        pClone->setName(name);
        pClone->setPosition(Vec2(config.origin.x, y));
        auto box = pClone->getBoundingBox();
        log("%p", &box);
        pLayer->addChild(pClone);
        auto vm = _pRoot->bindInstance(factory, pClone, "li");
        vm->update(values);
        if(pClone->getParent() != nullptr){
            i++;
            y += config.verticalMargin;
        }
    }
    if (y > config.contentAreaSize.height) {
        pLayer->setInnerContainerSize(Size(config.contentAreaSize.width, y - 40));
    } else {
        pLayer->setInnerContainerSize(config.contentAreaSize);
    }
}

void ViewModel::setList(const std::string& listTemplateName, Factory<ViewModel>&factory, const ValueVector& array, bool hasBlank, bool hasScrollBar)
{
    setList("ScrollArea", "c_SummaryViewModel", listTemplateName, factory, array, hasBlank, hasScrollBar);
}

void ViewModel::setList(const std::string& areaName, const std::string& className, const std::string& listTemplateName, Factory<ViewModel>&factory, const ValueVector& array, bool hasBlank, bool hasScrollBar)
{
    auto pLayer = static_cast<cocos2d::ui::ListView*>(getNode(areaName));
    pLayer->removeAllChildren();
    auto pTmpl = static_cast<Widget*>(CSLoader::createNode(listTemplateName.c_str())->getChildByName("w_PanelSummary"));
    pTmpl->retain();
    for(auto& values: array){
        auto pNode = static_cast<Widget*>(pTmpl->clone());
        pLayer->addChild(pNode);
        auto vm = _pRoot->bindInstance(factory, pNode->getChildByName(className), className, false);
        vm->update(values);
    }
    if(hasBlank){
        auto blank = pTmpl->clone();
        blank->removeAllChildren();
        pLayer->addChild(blank);
    }
    pLayer->setScrollBarEnabled(hasScrollBar);
    jumpToTop(pLayer);
    pTmpl->release();
    pLayer->refreshView();
}

void ViewModel::setTable(const std::string& listTemplateName, Factory<ViewModel>&factory, const ValueVector& array)
{
    setTable("ScrollArea", "c_SummaryViewModel", listTemplateName, factory, array);
}
void ViewModel::setTable(const std::string& areaName, const std::string& className, const std::string& listTemplateName, Factory<ViewModel>&factory, const ValueVector& array, bool hasMargin, bool hasScrollBar)
{
    auto pLayer = static_cast<cocos2d::ui::ListView*>(getNode(areaName));
    pLayer->removeAllChildren();
    auto pTmpl = static_cast<Widget*>(CSLoader::createNode(listTemplateName.c_str())->getChildByName("w_PanelSummary"));
    pTmpl->retain();
    auto row = Widget::create();
    row->setContentSize(Size(pLayer->getContentSize().width, pTmpl->getContentSize().height));
    auto x = 0;
    for(auto& values: array){
        auto pNode = static_cast<Widget*>(pTmpl->clone());
        pNode->setPositionX(x);
        auto vm = _pRoot->bindInstance(factory, pNode->getChildByName(className), className, false);
        vm->update(values);
        row->addChild(pNode);
        x += pNode->getContentSize().width;
        if(x >= pLayer->getContentSize().width){
            pLayer->addChild(row);
            x = 0;
            row = Widget::create();
            row->setContentSize(Size(pLayer->getContentSize().width, pTmpl->getContentSize().height));
        }
    }
    if(row->getChildren().size() > 0){
        pLayer->addChild(row);
    }
    if(hasMargin){
        auto margin = Widget::create();
        margin->setContentSize(Size(pLayer->getContentSize().width, pTmpl->getContentSize().height));
        pLayer->addChild(margin);
    }
    pLayer->setScrollBarEnabled(hasScrollBar);
    jumpToTop(pLayer);
    pTmpl->release();
    pLayer->refreshView();
}


void ViewModel::jumpToTop(cocos2d::ui::ScrollView* pList)
{
    auto& children = pList->getChildren();
    if(children.empty()){
        return;
    }
    auto height = children.at(0)->getContentSize().height * children.size();
    pList->setInnerContainerPosition(Vec2(0, -height));
}


void ViewModel::onExit()
{
    log("ViewModel::onExit");
}

void ViewModel::onEnter()
{
    log("ViewModel::onEnter");
}

void ViewModel::onEnterTransitionDidFinish()
{
    log("ViewModel::onEnterTransitionDidFinish");
}

bool ViewModel::fixName(Node* pNode)
{
    return true;
}

void ViewModel::save(const std::string& name, int value)
{
    UserDefault::getInstance()->setIntegerForKey(name.c_str(), value);
}

int ViewModel::load(const std::string& name){
    return UserDefault::getInstance()->getIntegerForKey(name.c_str());
}

View* ViewModel::getView()
{
    if(getParent() == nullptr){
        return static_cast<View*>(_pNode);
    }else{
        return getParent()->getView();
    }
}

void ViewModel::setName(const std::string name){
    _name = name;
}

const std::string ViewModel::getName() const
{
    return _name;
}

ViewModel* ViewModel::getChildByName(const std::string& name)
{
    auto children = getChildren();
    for(auto pChild: children){
        if(pChild->getName() == name){
            return pChild;
        }
    }
    return nullptr;
}

void ViewModel::update(const Value& value)
{
}

void ViewModel::enable(const std::string& name)
{
    static_cast<Widget*>(getNode(name))->setEnabled(true);
}

void ViewModel::disable(const std::string& name)
{
    static_cast<Widget*>(getNode(name))->setEnabled(false);
}

void ViewModel::enable()
{
    static_cast<Widget*>(getNode())->setEnabled(true);
}

void ViewModel::disable()
{
    static_cast<Widget*>(getNode())->setEnabled(false);
}

void ViewModel::hide()
{
    _pNode->setVisible(false);
}

void ViewModel::hide(const std::string& name)
{
    getNode(name)->setVisible(false);
}

void ViewModel::show()
{
    _pNode->setVisible(true);
}

void ViewModel::show(const std::string& name)
{
    getNode(name)->setVisible(true);
}

void ViewModel::disableTouch()
{
    Director::getInstance()->getEventDispatcher()->setEnabled(false);
}

void ViewModel::enableTouch()
{
    Director::getInstance()->getEventDispatcher()->setEnabled(true);
}

void ViewModel::disableTouch(const std::string& name)
{
    Director::getInstance()->getEventDispatcher()->pauseEventListenersForTarget(getNode(name), true);
}

void ViewModel::enableTouch(const std::string& name)
{
    Director::getInstance()->getEventDispatcher()->resumeEventListenersForTarget(getNode(name), true);
}

int ViewModel::random(const int max)
{
    std::random_device randomDevice;
    std::vector<uint32_t> randomSeedVector(10);
    std::generate(randomSeedVector.begin(), randomSeedVector.end(), std::ref(randomDevice));
    std::seed_seq randomSeed(randomSeedVector.begin(), randomSeedVector.end());
    std::mt19937 randomEngine(randomSeed);
    std::uniform_int_distribution<int> dist(0, max);
    return dist(randomEngine);
}

void ViewModel::toggle(const std::string& name)
{
    toggle(getNode(name));
}

void ViewModel::toggle(Node* pNode)
{
    if(pNode->isVisible()){
        Director::getInstance()->getEventDispatcher()->pauseEventListenersForTarget(pNode, true);
        auto effect = Sequence::create(FadeOut::create(0.2f),
                                       CallFuncN::create([&](Node* pNode){
            auto name = pNode->getName();
            pNode->getParent()->reorderChild(pNode, 0);
            disableTouch(name);
            hide(name);
        }), nullptr);
        pNode->runAction(effect);
    }else{
        Director::getInstance()->getEventDispatcher()->resumeEventListenersForTarget(pNode, true);
        auto effect = Sequence::create(FadeIn::create(0.2f),
                                       CallFuncN::create([&](Node* pNode){
            pNode->getParent()->reorderChild(pNode, 100);
            auto name = pNode->getName();
            enableTouch(name);
            show(name);
        }), nullptr);
        pNode->runAction(effect);
    }

}

void ViewModel::refresh(const int status, const std::string& callee)
{
    refresh(status);
}

void ViewModel::refresh(const int status)
{
}

View* ViewModel::pushView(const std::string& name, Factory<ViewModel>& factory, Scene* scene, Node* hideNode)
{
    auto ignoreNodeName = "PlayerStatus";
    if(!scene){
        scene = Director::getInstance()->getRunningScene();
    }
    if(hideNode){
        hideNode->setVisible(false);
    }
    ViewModel::invisibleNodes.push(hideNode);
    std::vector<Node*> nodes;
    auto& children = scene->getChildren();
    nodes.push_back(children.back());
    for(auto i = 0; i < nodes.size(); i++){
        auto& node = nodes.at(i);
        if(node->getName() == ignoreNodeName){
            continue;
        }
        node->pause();
        auto& children = node->getChildren();
        for(auto& c: children){
            nodes.push_back(c);
        }
    }
    auto view = View::create();
    view->setName(name);
    scene->addChild(view);
    view->initWithFactory(name, factory);
    return view;
}

void ViewModel::popView()
{
    auto name = getName();
    auto scene = Director::getInstance()->getRunningScene();
    auto& children = scene->getChildren();
    auto* view = static_cast<View*>(children.back());
    std::vector<Node*> nodes;
    nodes.push_back(view);
    for(auto i = 0; i < nodes.size(); i++){
        auto* node = nodes.at(i);
        if(node == _pNode){
            _pNode = nullptr;
            break;
        }
        auto& children = node->getChildren();
        for(auto& c: children){
            nodes.push_back(c);
        }
    }
    view->removeFromParent();

    nodes.clear();
    auto* node = scene->getChildren().back();
    if(node->getName() == ""){
        return;
    }
    nodes.push_back(node);
    for(auto i = 0; i < nodes.size(); i++){
        auto& node = nodes.at(i);
        node->resume();
        auto& children = node->getChildren();
        for(auto& c: children){
            nodes.push_back(c);
        }
    }
    if(!ViewModel::invisibleNodes.empty()){
        auto node = ViewModel::invisibleNodes.top();
        if(node != nullptr){
            node->setVisible(true);
        }
        ViewModel::invisibleNodes.pop();
    }
    
    static_cast<View*>(node)->getRootViewModel()->refresh(ViewModel::Status::POPVIEW, name);
}

ViewModel* ViewModel::getRoot(const std::string& name, Scene* scene)
{
    if(!scene){
        scene = Director::getInstance()->getRunningScene();
    }
    auto& children = scene->getChildren();
    for(auto& child: children){
        if(child->getName() == name){
            return static_cast<View*>(child)->getRootViewModel();
        }
    }
    return nullptr;
}

spine::SkeletonAnimation* ViewModel::replaceToAnimation(const std::string& nodeName, const std::string& animationName)
{
    auto skeletonNode = spine::SkeletonAnimation::createWithFile(animationName + ".json",
                                                                 animationName + ".atlas", 1);
    auto base = getNode(nodeName);
    auto pos = base->getPosition();
    skeletonNode->setPosition(base->getPosition());
    skeletonNode->setScale(base->getScale());
    base->getParent()->addChild(skeletonNode, base->getLocalZOrder());
    base->removeFromParent();
    _watches[nodeName] = skeletonNode;
    return skeletonNode;
}

void ViewModel::onTouch(Widget* pWidget, std::function<void (Ref* pRef)> fn)
{
    pWidget->addTouchEventListener([&, fn](Ref* pRef, Widget::TouchEventType type){
        switch (type) {
            case Widget::TouchEventType::ENDED: {
                fn(pRef);
                break;
            }
            case Widget::TouchEventType::BEGAN: {
                _moved = false;
            }
            case Widget::TouchEventType::MOVED: {
                _moved = true;
            }
            case Widget::TouchEventType::CANCELED: {
                if(_moved == false){
                    fn(pRef);
                }
            }
        }
    });
}