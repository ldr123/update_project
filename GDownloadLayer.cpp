//
//  GDownloadLayer.cpp
//  dz3gz
//
//  Created by ldr123 on 10/21/13.
//
//

#include "GDownloadLayer.h"
#include "GSceneManager.h"
#include "RdTcpSocket.h"
#include "GUpdateImpl.h"
#include "GFileUtility.h"
#include "GResourceHelper.h"
#include "GDynamicDataManager.h"
#include "ClientLib/ClientCtrl.h"
#include "GActivityView.h"
#include "GResourceDataManager.h"
#include "UserDataStorage.h"
#include "GSystemIntializer.h"
#include "GPlayerDataClient.h"
#include "GDownloadScene.h"
#include "GUtility.h"
#include "GameEventListener.h"
#include "GSelectForceLayer.h"
#include "CustomTip.h"
#include "LanguageManager.h"
#include "NewbieMgr.h"
#include "GUpdateConfig.h"
#include "GCommonAlertView.h"
#include "GOpenCcbiFile.h"
#include "GCCBMacro.h"

//Roderick.Zheng 2014.8.18
#include "RdSampleGameImpl.h"

using namespace std;
using namespace std::placeholders;
using namespace cocos2d;
using namespace cocos2d::extension;

#define SERVER_TOKEN_SIZE       "%s_size"
#define SERVER_TOKEN_WITH_POS   "%s_%d"

GDownloadLayer::GDownloadLayer() :m_pSampleGameNode(NULL){}
GDownloadLayer::~GDownloadLayer(){}
void GDownloadLayer::onEnter()
{
    GAbstractLayer::onEnter();

    m_menuRetry->setVisible(false);
    GameEventListener::shareGameEventListener()->addEventListener(this, callfuncO_selector(GDownloadLayer::onListenerSelectForce), SELECT_FORCE);

    if (GAME_CONFIG.canAutoUpdate())
    {
        UpdateImpl::Instance().threadContinue(true);
    }

    //Roderick.Zheng 2014.8.18
    RdSampleGameImpl::Instance().setGameLayerParent(m_pSampleGameNode);
}

void GDownloadLayer::onExit()
{
    GAbstractLayer::onExit();
    RdSampleGameImpl::Instance().setGameLayerParent(NULL);
    GameEventListener::shareGameEventListener()->removeEventListener(this, SELECT_FORCE);
}

void GDownloadLayer::onNodeLoaded(CCNode *pNode, CCNodeLoader *pNodeLoader)
{
    m_pDownloadNode = (CCLabelTTF*)GetElement("m_pDownloadNode");
    m_downloadInfo = (CCLabelTTF*)GetElement("m_downloadInfo");
    m_loadingProgress = (CCProgressTimer*)GetElement("m_loadingProgress");
    m_menuRetry = (CCMenuItemImage*)GetElement("m_pRetry");
    m_pSampleGameNode = (CCNode*)GetElement("m_pSampleGameNode");

    m_pLoginNode = (GTouchToLogin*)GetElement("m_pLoginNode");
    m_pLoginNode->setVisible(false);
    m_pLoginNode->setTouchFunc([this](){onStartGame(); });
}

void GDownloadLayer::Refresh()
{
    if (GAME_CONFIG.canAutoUpdate())
    {
        UpdateImpl::Instance().setDelegate(this);
        UpdateImpl::Instance().startUpdate();

        return;
    }

    loadingCompleted();
}

SEL_MenuHandler GDownloadLayer::BindMenuItemSelector(CCObject *pTarget, const char *pSelectorName)
{
    if (strcmp("onRetry", pSelectorName) == 0)
    {
        return menu_selector(GDownloadLayer::onRetry);
    }

    return nullptr;
}

void GDownloadLayer::onRetry(CCObject* sender)
{
    UpdateImpl::Instance().setDelegate(this);
    UpdateImpl::Instance().startUpdate();
}

void GDownloadLayer::onStartGame()
{
    auto funcClientCtrl = [](ServiceMgr& mgr, uint64_t peer_id, const std::string& ip, uint16_t port, std::function<void(uint32_t rc)> cb)
    {
        GPlayerDataClient::Instance().clientCtrlFunc(mgr, peer_id, ip, port, cb);
    };
    ClientCtrl *pLoginCtrl = new ClientCtrl(ServiceMgr::Instance(), funcClientCtrl);
    std::string loginToken = GAME_CONFIG.getDebugDataStorage()->getstring(kLoginTokenKey);
    if (ServerDataMgr::Instance().bUseServerDataMgr)
    {
        m_strLastServerIp = ServerDataMgr::Instance().strServerIp;
        m_nLastServerPort = ServerDataMgr::Instance().nServerPort;
        loginToken = ServerDataMgr::Instance().strLoginToken;
    }

    GDynamicDataManager::ShareInstance()->SetCurrentClientCtrl(pLoginCtrl);
    pLoginCtrl->SetLoginServer(m_strLastServerIp, m_nLastServerPort);
    if (loginToken.empty())
    {
        GDownloadScene *pScene = (GDownloadScene*)GSceneManager::ShareInstance()->GetCurrentScene();
        pScene->Navigation(EDownloadNavigation::eSelectForceLayer);

        return;
    }

    if (m_pActivityView)
    {
        m_pActivityView->HideActivityView();
    }

    m_pActivityView = GActivityViewCreator::create(EActivityView::eProcess);
    m_pActivityView->ShowActivityView(CCDirector::sharedDirector()->getRunningScene());

    DoLogin();
}

void GDownloadLayer::newDevice()
{
    m_pLoginNode->setVisible(false);

    if (m_pActivityView)
    {
        m_pActivityView->HideActivityView();
    }

    m_pActivityView = GActivityViewCreator::create(EActivityView::eProcess);
    m_pActivityView->ShowActivityView(CCDirector::sharedDirector()->getRunningScene());

    ClientCtrl *pLoginCtrl = GDynamicDataManager::ShareInstance()->GetCurrentClientCtrl();
    string strKey = client::Utility::getDeviceKeyWidthRandomKey();
    proto::client::CreatePlayer c;
    GDownloadScene *pScene = (GDownloadScene*)GSceneManager::ShareInstance()->GetCurrentScene();
    GSelectForceLayer *pGSelectForceLayer = pScene->GetGSelectForceLayer();
    if (pGSelectForceLayer)
    {
        c.set_camp(pGSelectForceLayer->GetCamp());
        c.set_name(pGSelectForceLayer->GetName());
    }

    pLoginCtrl->NewDevice(strKey, c, [this](uint32_t rc, CltPlayer* clt)
    {
        if (m_pActivityView)
        {
            m_pActivityView->HideActivityView();
            m_pActivityView = nullptr;
        }

        if (rc != ERR_OK)
        {
            //Login to LoginServer failed.
            return;
        }

        std::string tempToken = clt->GetConnectToken();

        DataStorage* pStorage = GAME_CONFIG.getDebugDataStorage();
        pStorage->setstring(kLoginTokenKey, tempToken);

        if (!ServerDataMgr::Instance().bUseServerDataMgr)
        {
            return;
        }
        ServerDataMgr::Instance().strLoginToken = tempToken;
        char buf[128] = { 0 };
        sprintf(buf, SERVER_TOKEN_SIZE, ServerDataMgr::Instance().strServerDesc.c_str());
        int nSize = pStorage->getint(buf);
        bool bFind = false;
        for (int i = 0; i < nSize; ++i)
        {
            sprintf(buf, SERVER_TOKEN_WITH_POS, ServerDataMgr::Instance().strServerDesc.c_str(), i);
            string strTmp = pStorage->getstring(buf);
            if (strTmp == tempToken)
            {
                bFind = true;
                break;
            }
        }

        if (!bFind)
        {
            sprintf(buf, SERVER_TOKEN_SIZE, ServerDataMgr::Instance().strServerDesc.c_str());
            pStorage->setint(buf, nSize + 1);

            sprintf(buf, SERVER_TOKEN_WITH_POS, ServerDataMgr::Instance().strServerDesc.c_str(), nSize);
            pStorage->setstring(buf, tempToken);
        }

        GDynamicDataManager::ShareInstance()->SetCurrentPlayer(clt);
        GDynamicDataManager::ShareInstance()->SetCurrentOffLine(false);

        GPlayerDataClient::Instance().RequestPlayerBaseData([](uint32_t)
        {
            GSceneManager::ShareInstance()->ChangeToScene(kGMainSceneName);
            NetworkImpl::Instance().SetNotFirstLogin();
        });
    });
}

void GDownloadLayer::DoLogin()
{
    ClientCtrl* clientController = GDynamicDataManager::ShareInstance()->GetCurrentClientCtrl();
    CCAssert(clientController, "Error: clientControlloer is nullptr");
    std::string loginToken = GAME_CONFIG.getDebugDataStorage()->getstring(kLoginTokenKey);
    if (ServerDataMgr::Instance().bUseServerDataMgr)
    {
        loginToken = ServerDataMgr::Instance().strLoginToken;
    }
    GUILOG("loginToken:%s", loginToken.c_str());
    clientController->Relogin(loginToken, [this](uint32_t rc, CltPlayer* clt)
    {
        if (m_pActivityView)
        {
            m_pActivityView->HideActivityView();
            m_pActivityView = nullptr;
        }

        if (rc != ERR_OK)
        {
            //Login to LoginServer failed.
            return;
        }

        GDynamicDataManager::ShareInstance()->SetCurrentPlayer(clt);
        GDynamicDataManager::ShareInstance()->SetCurrentOffLine(false);
        GPlayerDataClient::Instance().RequestPlayerBaseData([](uint32_t)
        {
            GSceneManager::ShareInstance()->ChangeToScene(kGMainSceneName);
            NetworkImpl::Instance().SetNotFirstLogin();
        });
    });
}

void GDownloadLayer::waitForLoadingComplete(float dt)
{
    m_mtxResourceInited.lock();
    if (m_bResourceInited)
    {
        m_pDownloadNode->setVisible(false);
        m_pLoginNode->setVisible(true);
        CCScheduler* scheduler = CCDirector::sharedDirector()->getScheduler();
        scheduler->unscheduleSelector(schedule_selector(GDownloadLayer::waitForLoadingComplete), this);
    }
    m_mtxResourceInited.unlock();
}

void GDownloadLayer::loadingCompleted()
{
    CCFileUtils* pFileUtils = CCFileUtils::sharedFileUtils();

    string strDocumentCommonResourcePath = GResourceHelper::GetDocumentCommonResourcePath();
    {
        pFileUtils->removeSearchPath(strDocumentCommonResourcePath.c_str());
        pFileUtils->addSearchPath(strDocumentCommonResourcePath.c_str());
    }

    string strDocumentResourcePath = GResourceHelper::GetDocumentResourcePath();
    {
        pFileUtils->removeSearchPath(strDocumentResourcePath.c_str());
        pFileUtils->addSearchPath(strDocumentResourcePath.c_str());
    }

    if (GAME_CONFIG.canUseBuildin())
    {
        string strBuildinResourcePath = GResourceHelper::GetBuildinResourcePath();
        {
            pFileUtils->removeSearchPath(strBuildinResourcePath.c_str());
            pFileUtils->addSearchPath(strBuildinResourcePath.c_str());
        }
    }

    string strCommonResourcePath = GResourceHelper::GetBuildinCommonResourcePath();
    {
        pFileUtils->removeSearchPath(strCommonResourcePath.c_str());
        pFileUtils->addSearchPath(strCommonResourcePath.c_str());
    }

    string strGameResourcePath = GResourceHelper::GetBuildinGameResourcePath();
    {
        pFileUtils->removeSearchPath(strGameResourcePath.c_str());
        pFileUtils->addSearchPath(strGameResourcePath.c_str());
    }

//     pFileUtils->removeSearchPath(GResourceHelper::m_strWritableDir.c_str());

    m_bResourceInited = false;

    std::thread loadThread = std::thread([this]()
    {
        GSystemIntializer::ShareInstance()->InitConfig();
        m_mtxResourceInited.lock();
        m_bResourceInited = true;
        m_mtxResourceInited.unlock();
    });
    loadThread.detach();

    CCScheduler* scheduler = CCDirector::sharedDirector()->getScheduler();
    scheduler->scheduleSelector(schedule_selector(GDownloadLayer::waitForLoadingComplete), this, 0.0f, false);

    m_loadingProgress->setPercentage(0.f);
    m_loadingProgress->runAction(CCSequence::create(CCProgressTo::create(4.f, 100.f), nullptr));
}

void GDownloadLayer::onClickAlertAppOk()
{
    client::Utility::updateApp();
}

void GDownloadLayer::PlayAni(const std::string& strName)
{
    PlayAnimation(strName);
}

void GDownloadLayer::SendMessage(UpdateMessage* pMsg)
{
    if (!pMsg)
    {
        return;
    }

    UpdateState eState = pMsg->getState();
    if (!pMsg->getStopProcess())
    {
        m_lastUpdateState = eState;
    }

    if (eState == UpdateState::eUpdateFinish)
    {
        loadingCompleted();
    }
    else if (eState == UpdateState::eUpdateNeedUpdate)
    {
        //去平台更新
        GCommonAlertViewData alertViewData;
        char buf[256] = { 0 };
        sprintf(buf,
            sLanguageManager->getLanguageByKey("APP_VERSION_UPDATE").c_str(),
            pMsg->getMsg().c_str(), UPDATE_CLIENT_PACKAGE_VERSION_TXT
            );
        alertViewData.message = buf;
        alertViewData.iKnownButtonText = sLanguageManager->getLanguageByKey("Sure");

        GCommonAlertView* pAlertView = GAlertViewManager::CreateAlertView<GCommonAlertView>(&alertViewData);
        GAlertViewManager::ShareInstance()->ShowAlertView();
        pAlertView->SetIKnowCallBack(this, callfunc_selector(GDownloadLayer::onClickAlertAppOk), true);

        return;
    }

    m_downloadInfo->setColor(ccc3(255, 255, 0));
    m_downloadInfo->setString(pMsg->getMsg().c_str());
    if (eState == UpdateState::eUpdateStateFailed)
    {
        m_downloadInfo->setColor(ccc3(255, 0, 0));
        m_menuRetry->setVisible(true);
    }

    if (pMsg->getStopProcess())
    {
        return;
    }

    m_loadingProgress->setPercentage(pMsg->getRate());
}

void GDownloadLayer::onListenerSelectForce(cocos2d::CCObject* sender)
{
    newDevice();
}

void GSelectServerLayer::onNodeLoaded(CCNode* pNode, CCNodeLoader* pNodeLoader)
{
    CCSize sz = CCDirector::sharedDirector()->getWinSize();
    auto refreshTokenLst = [this, sz](CustomMenuItemFont* p)
    {
        m_pLoginTokenArr = CCArray::create();

        DataStorage* pStorage = GAME_CONFIG.getDebugDataStorage();
        auto serverLstMap = GAME_CONFIG.getServerLst();
        auto serverInfo = serverLstMap.find(p->getTag());
        if (serverInfo != serverLstMap.end())
        {
            char buf[128] = { 0 };
            sprintf(buf, SERVER_TOKEN_SIZE, serverInfo->second->m_desc.c_str());
            int nSize = pStorage->getint(buf);
            for (int i = 0; i < nSize; ++i)
            {
                sprintf(buf, SERVER_TOKEN_WITH_POS, serverInfo->second->m_desc.c_str(), i);
                string strToken = pStorage->getstring(buf);
                string strTokenEnd = strToken.size() > 32 ? strToken.substr(0, 32) : strToken;
                CustomMenuItemFont *pTmp = CustomMenuItemFont::create(strTokenEnd.c_str(), [this, strToken](CustomMenuItemFont* pItem)
                {
                    ServerDataMgr::Instance().strLoginToken = strToken;
                    ServerDataMgr::Instance().bUseServerDataMgr = true;

                    GDownloadScene *pScene = (GDownloadScene*)GSceneManager::ShareInstance()->GetCurrentScene();
                    pScene->Navigation(EDownloadNavigation::eDownloadLayer);
                });
                m_pLoginTokenArr->addObject(pTmp);
            }
        }

        m_pLoginTokenArr->addObject(CustomMenuItemFont::create("new...", [this](CustomMenuItemFont*)
        {
            ServerDataMgr::Instance().strLoginToken = "";
            ServerDataMgr::Instance().bUseServerDataMgr = true;

            GDownloadScene *pScene = (GDownloadScene*)GSceneManager::ShareInstance()->GetCurrentScene();
            pScene->Navigation(EDownloadNavigation::eDownloadLayer);
        }));

        if (m_menuLoginToken)
        {
            m_menuLoginToken->removeFromParent();
            m_menuLoginToken = nullptr;
        }

        m_menuLoginToken = CCMenu::create();
        if (m_menuLoginToken->initWithArray(m_pLoginTokenArr))
        {
            m_menuLoginToken->alignItemsVerticallyWithPadding(15.f);
            m_menuLoginToken->setPosition(ccp(sz.width - 300.f, sz.height / 2));
        }
        addChild(m_menuLoginToken);
    };

    m_ServerLstMenuArr = new CCArray();
    auto serverLstMap = GAME_CONFIG.getServerLst();
    for (auto& x : serverLstMap)
    {
        if (ServerDataMgr::Instance().strServerIp.empty())
        {
            ServerDataMgr::Instance().strServerDesc = x.second->m_desc;
            ServerDataMgr::Instance().strServerIp = x.second->m_strIp;
            ServerDataMgr::Instance().nServerPort = x.second->m_nPort;
            ServerDataMgr::Instance().bUseServerDataMgr = true;
        }

        CustomMenuItemFont *pTmp1 = CustomMenuItemFont::create(x.second->m_desc.c_str(), [this, refreshTokenLst](CustomMenuItemFont* p)
        {
            refreshTokenLst(p);
            if (m_pServerInfo)
            {
                m_pServerInfo->removeFromParent();
                m_pServerInfo = nullptr;
            }

            auto serverLstMap = GAME_CONFIG.getServerLst();
            auto serverInfo = serverLstMap.find(p->getTag());
            if (serverInfo != serverLstMap.end())
            {
                m_pServerInfo = CCLabelTTF::create(serverInfo->second->m_desc.c_str(), "Marker Felt", 32);
                m_pServerInfo->setPosition(ccp(150.f, 30.f));

                addChild(m_pServerInfo);

                ServerDataMgr::Instance().strServerDesc = serverInfo->second->m_desc;
                ServerDataMgr::Instance().strServerIp = serverInfo->second->m_strIp;
                ServerDataMgr::Instance().nServerPort = serverInfo->second->m_nPort;
                ServerDataMgr::Instance().bUseServerDataMgr = true;
            }
        });
        pTmp1->setTag(x.first);
        m_ServerLstMenuArr->addObject(pTmp1);
    }

    m_pServerLstMenu = CCMenu::create();
    if (m_pServerLstMenu->initWithArray(m_ServerLstMenuArr))
    {
        m_pServerLstMenu->alignItemsVerticallyWithPadding(15.f);
        m_pServerLstMenu->setPosition(ccp(150.f, sz.height - 300.f));
    }
    m_ServerLstMenuArr->release();
    addChild(m_pServerLstMenu);
}
