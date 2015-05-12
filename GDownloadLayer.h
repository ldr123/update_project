//
//  GDownloadLayer.h
//  dz3gz
//
//  Created by ldr123 on 10/21/13.
//
//

#ifndef __dz3gz__GDownloadLayer__
#define __dz3gz__GDownloadLayer__

#include "GAbstractLayer.h"
#include "GUpdateDelegate.h"
#include "NetworkImpl.h"
#include "GAbstractTouchLayer.h"
#include "Utility/Singleton.h"

//Roderick Zheng 2014.2.18
#include "RdSampleGameImpl.h"

#define kLoginTokenKey  "LoginToken"

class CustomMenuItemFont : public cocos2d::CCMenuItemFont
{
    typedef std::function<void(CustomMenuItemFont*)> FN;
public:
    static CustomMenuItemFont* create(const char *value, const FN& fk)
    {
        if (!fk)
        {
            return nullptr;
        }

        CustomMenuItemFont *pRet = new CustomMenuItemFont();
        pRet->initWithString(value, nullptr, nullptr);
        pRet->setFunc(fk);
        pRet->autorelease();

        return pRet;
    }

    void activate()
    {
        if (m_bEnabled && m_pFunc)
        {
            m_pFunc(this);
        }
    }

    void setFunc(const FN& fk){ m_pFunc = fk; }
private:
    FN m_pFunc = nullptr;
};

class GTouchToLogin : public GAbstractLayer
{
    typedef std::function<void()> TOUCH_FUNCTION;
public:
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event)
    {
        if (!isVisible())
        {
            return false;
        }

        return true;
    }

    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event)
    {
        GAbstractLayer::ccTouchEnded(touch, event);

        if (m_touchFunc)
        {
            m_touchFunc();
        }
    }
public:
    void setTouchFunc(const TOUCH_FUNCTION& func){ m_touchFunc = func; }
private:
    TOUCH_FUNCTION m_touchFunc = nullptr;
};

class ServerDataMgr : public Singleton<ServerDataMgr>
{
public:
    std::string strServerDesc = "";
    std::string strServerIp = "";
    int nServerPort = 0;
    std::string strLoginToken = "";
    bool bUseServerDataMgr = false;
};

class GActivityView;
class RdSampleGameLayer;
class GDownloadLayer :public GAbstractLayer, public UpdateDelegate
{
public:
	GDownloadLayer();
	~GDownloadLayer();

    virtual void onEnter();
    virtual void onExit();
    virtual void onNodeLoaded(cocos2d::CCNode* pNode, cocos2d::extension::CCNodeLoader* pNodeLoader);
    virtual cocos2d::SEL_MenuHandler BindMenuItemSelector(cocos2d::CCObject *pTarget, const char *pSelectorName);
    virtual void SendMessage(UpdateMessage* pMsg);
    virtual void PlayAni(const std::string& strName);
    virtual void Refresh();

    void onRetry(cocos2d::CCObject* sender);
    void onListenerSelectForce(cocos2d::CCObject* sender);
    void waitForLoadingComplete(float dt);

    void onClickAlertAppOk();
private:
    void DoLogin();
    void newDevice();
    void loadingCompleted();
    void onStartGame();
private:
    cocos2d::CCNode* m_pDownloadNode = nullptr;
    cocos2d::CCLabelTTF* m_downloadInfo = nullptr;
    cocos2d::CCProgressTimer* m_loadingProgress = nullptr;
    cocos2d::CCMenuItemImage* m_menuRetry = nullptr;
    UpdateState m_lastUpdateState = UpdateState::eUpdateStateUnknow;

    GTouchToLogin* m_pLoginNode = nullptr;

    std::string m_strLastServerIp = "";
    int m_nLastServerPort = 0;
    GActivityView* m_pActivityView = nullptr;

    std::mutex m_mtxResourceInited;
    bool m_bResourceInited = false;

	//add by Roderick.Zheng 2014.8.18
	CCNode* m_pSampleGameNode;
};

class GSelectServerLayer : public GAbstractLayer
{
public:
    virtual void onNodeLoaded(cocos2d::CCNode* pNode, cocos2d::extension::CCNodeLoader* pNodeLoader);
private:
    cocos2d::CCArray* m_ServerLstMenuArr = nullptr;
    cocos2d::CCMenu* m_pServerLstMenu = nullptr;
    cocos2d::CCLabelTTF* m_pServerInfo = nullptr;
    cocos2d::CCArray* m_pLoginTokenArr = nullptr;
    cocos2d::CCMenu* m_menuLoginToken = nullptr;
};

#endif /* defined(__dz3gz__GDownloadLayer__) */
