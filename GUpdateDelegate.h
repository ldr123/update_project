//
//  GUpdateDelegate.h
//  dz3gz
//
//  Created by ldr123 on 11/04/13.
//
//

#ifndef __dz3gz__GUpdateDelegate____
#define __dz3gz__GUpdateDelegate____

#include "cocos2d.h"
#include "curl/curl.h"
#include "curl/easy.h"
#include "Utility.h"
#include "LanguageManager.h"

#define LANGUAGE_STRING_KEY(x) sLanguageManager->getLanguageByKey(x)

#define UPDATE_STATE_NAME_BEGIN         LANGUAGE_STRING_KEY("UPDATE_STATE_NAME_BEGIN")
#define UPDATE_STATE_NAME_CHECKNEWVER   LANGUAGE_STRING_KEY("UPDATE_STATE_NAME_CHECKNEWVER")   
#define UPDATE_STATE_NAME_DOWNLOADFILE  LANGUAGE_STRING_KEY("UPDATE_STATE_NAME_DOWNLOADFILE")  
#define UPDATE_STATE_NAME_DECOMPRESS    LANGUAGE_STRING_KEY("UPDATE_STATE_NAME_DECOMPRESS")    
#define UPDATE_STATE_NAME_PARSEDIFFFILE LANGUAGE_STRING_KEY("UPDATE_STATE_NAME_PARSEDIFFFILE")
#define UPDATE_STATE_NAME_CHECKCLIENT   LANGUAGE_STRING_KEY("UPDATE_STATE_NAME_CHECKCLIENT")  
#define UPDATE_STATE_NAME_CLEARCACHE    LANGUAGE_STRING_KEY("UPDATE_STATE_NAME_CLEARCACHE")   
#define UPDATE_STATE_NAME_NEEDUPDATE    LANGUAGE_STRING_KEY("UPDATE_STATE_NAME_NEEDUPDATE")   
#define UPDATE_CONNECT_ERROR            LANGUAGE_STRING_KEY("UPDATE_CONNECT_ERROR")    
#define UPDATE_RECVDATA_ERROR           LANGUAGE_STRING_KEY("UPDATE_RECVDATA_ERROR")        
#define UPDATE_TIMEOUT_ERROR            LANGUAGE_STRING_KEY("UPDATE_TIMEOUT_ERROR")         
#define UPDATE_UNKNOW_ERROR             LANGUAGE_STRING_KEY("UPDATE_UNKNOW_ERROR")      
#define UPDATE_START_UPDATE             LANGUAGE_STRING_KEY("UPDATE_START_UPDATE")      
#define UPDATE_GETSERVERVER             LANGUAGE_STRING_KEY("UPDATE_GETSERVERVER")      
#define UPDATE_DOWNLOAD_PACKAGE         LANGUAGE_STRING_KEY("UPDATE_DOWNLOAD_PACKAGE")      
#define UPDATE_DECOMPRESS_FILE          LANGUAGE_STRING_KEY("UPDATE_DECOMPRESS_FILE")     
#define UPDATE_PARSE_DIFF_FILE          LANGUAGE_STRING_KEY("UPDATE_PARSE_DIFF_FILE")     
#define UPDATE_CHECKCLIENT_FILE         LANGUAGE_STRING_KEY("UPDATE_CHECKCLIENT_FILE")    
#define UPDATE_UPDATE_ERROR             LANGUAGE_STRING_KEY("UPDATE_UPDATE_ERROR")    
#define UPDATE_USERREJECT               LANGUAGE_STRING_KEY("UPDATE_USERREJECT")     

enum class UpdateState
{
    eUpdateStateUnknow = 0,
    eUpdateStateBegin,
    eUpdateStateCheckNewVersion,
    eUpdateStateDownloadFile,
    eUpdateStateDecompress,
    eUpdateStateParseDiffFile,
    eUpdateStateCheckCilent,
    eUpdateStateClearCache,
    eUpdateStateFailed,
    eUpdateFinish,
    eUpdateNeedUpdate,
    eUpdateUserReject,
    eUpdateStateShowSmallGame,
    eUpdateStateHideSmallGame,
};

namespace
{
    std::string getHttpProxyErrCodeName(const CURLcode& e)
    {
        if (e == CURLE_OK)
        {
            return "";
        }

        switch (e)
        {
            case CURLE_COULDNT_CONNECT:
                return UPDATE_CONNECT_ERROR;
            case CURLE_READ_ERROR:
            case CURLE_RECV_ERROR:
                return UPDATE_RECVDATA_ERROR;
            case CURLE_OPERATION_TIMEDOUT:
                return UPDATE_TIMEOUT_ERROR;
            case CURL_LAST:
                return UPDATE_UNKNOW_ERROR;
        }

        return UPDATE_UNKNOW_ERROR;
    }

    std::string getUpdateStateName(const UpdateState& s)
    {
        switch (s)
        {
            case UpdateState::eUpdateStateBegin:
                return UPDATE_STATE_NAME_BEGIN;
            case UpdateState::eUpdateStateCheckNewVersion:
                return UPDATE_STATE_NAME_CHECKNEWVER;
            case UpdateState::eUpdateStateDownloadFile:
                return UPDATE_STATE_NAME_DOWNLOADFILE;
            case UpdateState::eUpdateStateDecompress:
                return UPDATE_STATE_NAME_DECOMPRESS;
            case UpdateState::eUpdateStateParseDiffFile:
                return UPDATE_STATE_NAME_PARSEDIFFFILE;
            case UpdateState::eUpdateStateCheckCilent:
                return UPDATE_STATE_NAME_CHECKCLIENT;
            case UpdateState::eUpdateStateClearCache:
                return UPDATE_STATE_NAME_CLEARCACHE;
            case UpdateState::eUpdateNeedUpdate:
                return UPDATE_STATE_NAME_NEEDUPDATE;
            case UpdateState::eUpdateUserReject:
                return UPDATE_USERREJECT;
            case UpdateState::eUpdateFinish:
                return UPDATE_STATE_NAME_CHECKCLIENT;
        }

        return "";
    }
}

class UpdateMessage
{
public:
    explicit UpdateMessage(UpdateState s, const std::string& what, float b, float e, bool bStop)
        :eUpdateState(s), strWhat(what), fBegin(b), fEnd(e), bStopProcess(bStop)
    {}
    virtual ~UpdateMessage(){}

    virtual std::string getMsg(){ return strWhat; }
    virtual void setMsg(const std::string& strMsg){ strWhat = strMsg; }
    virtual float getRate(){ return  fEnd; }
    UpdateState getState(){ return eUpdateState; }
    bool getStopProcess(){ return bStopProcess; }
protected:
    UpdateState	eUpdateState;
    std::string strWhat;
    float		fBegin, fEnd;
    bool		bStopProcess; //Í£Ö¹½øÐÐ
};


class UpdateBeginMessage : public UpdateMessage
{
public:
    explicit UpdateBeginMessage()
        :UpdateMessage(UpdateState::eUpdateStateBegin, UPDATE_START_UPDATE, 0.f, 1.f, false){}
};

class UpdateCheckClientVersionMessage : public UpdateMessage
{
public:
    explicit UpdateCheckClientVersionMessage()
        :UpdateMessage(UpdateState::eUpdateStateCheckNewVersion, UPDATE_GETSERVERVER, 1.f, 3.f, false){}
};

class UpdateDownloadFileMessage : public UpdateMessage
{
public:
    explicit UpdateDownloadFileMessage(int _nCurrentSize, int _nTotalSize, int _fSpeed)
        :UpdateMessage(UpdateState::eUpdateStateDownloadFile, UPDATE_DOWNLOAD_PACKAGE, 3.f, 65.f, false),
        nCurrentSize(_nCurrentSize), nTotalSize(_nTotalSize), fSpeed(_fSpeed)
    {}

    virtual std::string getMsg()
    {
        char buf[256] = { 0 };

        float _fRate = 0.f;
        if (nTotalSize > 0)
        {
            _fRate = (float)nCurrentSize * 100 / nTotalSize;
            if (_fRate < 0.01f)
            {
                _fRate = 0.01f;
            }
            else if (_fRate > 100.f)
            {
                _fRate = 100.f;
            }
        }

        sprintf(buf, strWhat.c_str(), (int)_fRate, fSpeed);
        return buf;
    }

    virtual float getRate()
    {
        if (nTotalSize == 0)
        {
            return fBegin;
        }

        float fRate1 = float(nCurrentSize) / nTotalSize;
        if (fRate1 < 0.1f)
        {
            fRate1 = 0.1f;
        }
        else if (fRate1 > 1.f)
        {
            fRate1 = 1.f;
        }
        return fBegin + (fEnd - fBegin)*fRate1;
    }

    int nCurrentSize;
    int nTotalSize;
    int fSpeed;
};

class UpdateDecompressMessage : public UpdateMessage
{
public:
    explicit UpdateDecompressMessage(float _fRate)
        :UpdateMessage(UpdateState::eUpdateStateDecompress, UPDATE_DECOMPRESS_FILE, 65.f, 70.f, false),
        fRate(_fRate)
    {}

    virtual std::string getMsg()
    {
        char buf[256] = { 0 };
        sprintf(buf, strWhat.c_str(), (int)(fRate*100.f));
        return buf;
    }

    virtual float getRate()
    {
        return fBegin + (fEnd - fBegin)*fRate;
    }

    float fRate;
};

class UpdateParseDiffFileMessage : public UpdateMessage
{
public:
    explicit UpdateParseDiffFileMessage(float _fRate)
        :UpdateMessage(UpdateState::eUpdateStateParseDiffFile, UPDATE_PARSE_DIFF_FILE, 70.f, 90.f, false),
        fRate(_fRate)
    {}

    virtual std::string getMsg()
    {
        char buf[256] = { 0 };
        sprintf(buf, strWhat.c_str(), (int)(fRate*100.f));
        return buf;
    }

    virtual float getRate()
    {
        return fBegin + (fEnd - fBegin)*fRate;
    }

    float fRate;
};

class UpdateCheckClientMessage : public UpdateMessage
{
public:
    explicit UpdateCheckClientMessage()
        :UpdateMessage(UpdateState::eUpdateStateCheckCilent, UPDATE_CHECKCLIENT_FILE, 90.f, 95.f, false){}
};

class UpdateCleanCacheMessage : public UpdateMessage
{
public:
    explicit UpdateCleanCacheMessage()
        :UpdateMessage(UpdateState::eUpdateStateClearCache, UPDATE_CHECKCLIENT_FILE, 95.f, 100.f, false){}
};

class UpdateFinishMessage : public UpdateMessage
{
public:
    explicit UpdateFinishMessage()
        :UpdateMessage(UpdateState::eUpdateFinish, UPDATE_CHECKCLIENT_FILE, 0.f, 0.f, true){}
};

class UpdateFailedMessage : public UpdateMessage
{
public:
    explicit UpdateFailedMessage(CURLcode errCode, UpdateState s)
        :UpdateMessage(UpdateState::eUpdateStateFailed, UPDATE_UPDATE_ERROR, 0.f, 0.f, true),
        httpErrorCode(errCode), lastState(s)
    {}

    virtual std::string getMsg()
    {
        char buf[256] = { 0 };
        sprintf(
            buf, strWhat.c_str(),
            getUpdateStateName(lastState).c_str(),
            getHttpProxyErrCodeName(httpErrorCode).c_str()
            );
        return buf;
    }

    CURLcode	httpErrorCode;
    UpdateState	lastState;
};

class UserRejectMessage : public UpdateMessage
{
public:
    explicit UserRejectMessage(UpdateState s)
        :UpdateMessage(UpdateState::eUpdateUserReject, UPDATE_USERREJECT, 0.f, 0.f, true)
        , lastState(s)
    {}

    virtual std::string getMsg()
    {
        char buf[256] = { 0 };
        sprintf(buf, strWhat.c_str(), getUpdateStateName(lastState).c_str());
        return buf;
    }

    UpdateState	lastState;
};

class UpdateNeedUpdateMessage : public UpdateMessage
{
public:
    explicit UpdateNeedUpdateMessage() 
        :UpdateMessage(UpdateState::eUpdateNeedUpdate, "", 0.f, 0.f, true){}
};

class UpdateShowGameMessage : public UpdateMessage
{
public:
    explicit UpdateShowGameMessage()
        :UpdateMessage(UpdateState::eUpdateStateShowSmallGame, "", 0.f, 0.f, false){}
};

class UpdateHideGameMessage : public UpdateMessage
{
public:
    explicit UpdateHideGameMessage()
        :UpdateMessage(UpdateState::eUpdateStateHideSmallGame, "", 0.f, 0.f, false){}
};

class UpdateDelegate
{
public:
    virtual void SendMessage(UpdateMessage* pMsg) = 0;
    virtual void PlayAni(const std::string& strName) = 0;
};

#endif
