//
//  GUpdateImpl.h
//  dz3gz
//
//  Created by ldr123 on 11/04/13.
//
//

#ifndef __dz3gz__GUpdateImpl____
#define __dz3gz__GUpdateImpl____

#include "cocos2d.h"
#include "Utility/Singleton.h"

#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>

#include "GUpdateDelegate.h"

class UpdateDelegate;
class UpdateMessage;

enum class UpdateImplConfirm
{
    UPDATE_UNKNOW,
    UPDATE_CONTINUE,
    UPDATE_REJECT,
};

class UpdateImpl : public cocos2d::CCObject, public Singleton<UpdateImpl>
{
    friend class Singleton<UpdateImpl>;

    typedef unsigned char TByte;
    typedef std::unordered_map<std::string, std::string> FileNameWithMD5;
    typedef std::list<std::string> DeleteFileList;
    typedef std::atomic<UpdateImplConfirm> AUPDATECONFIRM;
public:
    ~UpdateImpl();
private:
    UpdateImpl();
public:
    void setDelegate(UpdateDelegate* pDelegate){ m_pUpdateDegate = pDelegate; }
    void startUpdate();
    void doUpdate(); //multi-thread
    void mainUpdate(float dt);
    void finish();

    void updateLocalClientResVersion(int nVer);
    void fullresUpdate();
    void diffUpdate();
    void postMessage(UpdateMessage* pMsg);

    void threadContinue(bool bContinue);

    void ConfirmContinue();	//线程继续
    void ConfirmReject();	//线程拒绝

    uint64_t getDownloadTime(){ return m_ulDownloadTime; }
    unsigned long getDownloadSize(){ return m_ulDownloadSize; }
    unsigned long getDownloadStartSize(){ return m_ulDownloadStartSize; }
    unsigned long getDownloadTotalSize(){ return m_ulDownloadTotalSize; }
    void setDownloadSize(unsigned long sz){ m_ulDownloadSize = sz; }
private:
    void initData();

    bool decompress(const std::string& strZipFile, const std::string& destPath);

    int getLocalClientResVersion();

    CURLcode download(const std::string& strFileUrl, const std::string& strSaveFile, bool bAllow3G);
    CURLcode _download(const std::string& strFileUrl, const std::string& strSaveFile, int nRangeFrom);

    CURLcode getRemoteFileSize(const std::string& strUrl, int &nLen);
    CURLcode getHttpText(const std::string& strUrl, std::string& strHttpText);

    CURLcode getServerVersion();
    bool parseDecompressData();
    void checkClient();
    void cleanUp();

    bool downloadAndDecompress(const std::string& downLoadFile);
private:
    std::mutex m_mtxConfirm;
    AUPDATECONFIRM m_eUpdateConfirm;
    std::thread* m_trdUpdate = nullptr;
    std::mutex m_mtxState;
    UpdateMessage* m_lastMsg = nullptr;
    std::mutex m_mtxContinue;
    bool m_bThreadContinue = true;
    bool m_bWaitForContinue = false;
    int m_nLocalSize = 0;
    UpdateDelegate*	m_pUpdateDegate = nullptr;	//weak ref

    std::string m_strDownloadingUrl = "";
    std::string m_strCheckBaseUrl = "";
    std::string m_strDownloadMd5 = "";
    int m_nClientResVersion = 0;
    int m_nServerResVersion = 0;
    int m_nDownloadType = 0;
    std::string m_strServerVersionText = "";

    FileNameWithMD5 m_mapFileWithMD5;
    uint64_t m_ulDownloadTime = 0;
    unsigned long m_ulDownloadStartSize = 0;
    unsigned long m_ulDownloadSize = 0;
    unsigned long m_ulDownloadTotalSize = 0;

    std::list<UpdateMessage*> m_lstSmallGameMsg;
};

#endif
