//
//  GUpdateImpl.cpp
//  dz3gz
//
//  Created by ldr123 on 11/04/13.
//

#include "GUpdateImpl.h"
#include "GResourceHelper.h"
#include "GFileUtility.h"

#include "support/zip_support/unzip.h"
#include "md5.h"
#include "UserDataStorage.h"

#include "GUpdateConfig.h"
#include "patch.h"
#include "GCommonAlertView.h"
#include "Utility.h"

//Roderick Zheng 2014.8.18
#include "RdSampleGameImpl.h"

USING_NS_CC;
using namespace std;

namespace
{
    void readFile(vector<TByte>& data, const string& fileName)
    {
        data.clear();

        FILE* file = fopen(fileName.c_str(), "rb");
        if (!file)
        {
            return;
        }

        fseek(file, 0, SEEK_END);
        int file_length = (int)ftell(file);
        fseek(file, 0, SEEK_SET);

        data.resize(file_length);
        if (file_length > 0)
        {
            fread(&data[0], 1, file_length, file);
        }

        fclose(file);
    }

    void writeFile(const TByte* data, int nLenth, const string& strFileName)
    {
        GFileUtility::CreateDirectoryFromFilePath(strFileName);

        FILE* file = fopen(strFileName.c_str(), "wb");
        if (!file)
        {
            return;
        }

        if (nLenth > 0)
        {
            fwrite(data, 1, nLenth, file);
        }

        fclose(file);
    }

    void writeFile(const vector<TByte>& data, const string& strFileName)
    {
        writeFile(&data[0], data.size(), strFileName);
    }

    long getFileSize(const string& strFileName)
    {
        long nLen = 0;
        FILE *fread = fopen(strFileName.c_str(), "rb");
        if (!fread)
        {
            return nLen;
        }

        fseek(fread, 0, SEEK_END);
        nLen = ftell(fread);
        fseek(fread, 0, SEEK_SET);
        fclose(fread);

        return nLen;
    }

    string getFileMd5(const string& strFileName)
    {
        unsigned char digest[16];
        memset(digest, 0, 16);

        vector<TByte> fileData;
        readFile(fileData, strFileName);
        if (fileData.size() > 0)
        {
            md5_state_s ctx;
            md5_init(&ctx);
            md5_append(&ctx, (unsigned char*)fileData.data(), fileData.size());
            md5_finish(&ctx, digest);
        }

        string strResult = "";
        char buf[4] = { 0 };
        for (auto &x : digest)
        {
            memset(buf, 0, 4);
            sprintf(buf, "%.2X", x);
            strResult += buf;
        }

        return strResult;
    }

    bool compareMd5(const string& strA, const string& strB)
    {
        int nSizeA = (int)strA.size();
        int nSizeB = (int)strB.size();

        if (nSizeA != nSizeB)
        {
            return false;
        }

        for (int i = 0; i < nSizeA; ++i)
        {
            char a = strA[i];
            char b = strB[i];

            if (a != b && tolower(a) != tolower(b))
            {
                return false;
            }
        }

        return true;
    }

    string unPackString(const TByte** pBegin, const TByte* pEnd)
    {
        string strRet = "";

        int nSize = unpack32Bit(pBegin, pEnd);
        for (int i = 0; i < nSize; ++i)
        {
            char c = (char)unpack32Bit(pBegin, pEnd);
            strRet.push_back(c);
        }

        return strRet;
    }
}

UpdateImpl::UpdateImpl() : m_pUpdateDegate(nullptr)
{
    initData();

    curl_global_init(CURL_GLOBAL_ALL);
}

UpdateImpl::~UpdateImpl()
{
    curl_global_cleanup();
}

void UpdateImpl::initData()
{
    m_trdUpdate = nullptr;
    m_lastMsg = nullptr;
    m_bThreadContinue = true;
    m_pUpdateDegate = nullptr;
    m_strDownloadingUrl = "";
    m_strDownloadMd5 = "";
    m_nClientResVersion = 0;
    m_strServerVersionText = "";
    m_nServerResVersion = 0;
    m_nDownloadType = 0;
    m_ulDownloadTime = 0;
    m_ulDownloadSize = 0;
    m_ulDownloadStartSize = 0;
    m_ulDownloadTotalSize = 0;
    m_eUpdateConfirm = UpdateImplConfirm::UPDATE_UNKNOW;
}

void UpdateImpl::threadContinue(bool bContinue)
{
    std::lock_guard<std::mutex> autoLock(m_mtxContinue);
    m_bThreadContinue = bContinue;

    if (m_trdUpdate)
    {
        return;
    }

    startUpdate();
}

int UpdateImpl::getLocalClientResVersion()
{
    int nResVersion = GAME_CONFIG.resourceVer();
    if (nResVersion > 0)
    {
        return nResVersion;
    }

    return UserDataStorage::Instance().getint(LOCAL_CLIENT_RES_VERSION);
}

void UpdateImpl::updateLocalClientResVersion(int nVer)
{
    //使用配置版本号
    if (GAME_CONFIG.resourceVer() > 0)
    {
        return;
    }

    UserDataStorage::Instance().setint(LOCAL_CLIENT_RES_VERSION, nVer);
}

CURLcode UpdateImpl::getRemoteFileSize(const string& strUrl, int &nLen)
{
    CURL *pCurl = curl_easy_init();
    if (!pCurl)
    {
        return CURL_LAST;
    }

    nLen = -1;
    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(pCurl, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(pCurl, CURLOPT_HEADER, 1);
    curl_easy_setopt(pCurl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(pCurl, CURLOPT_URL, strUrl.c_str());

    CURLcode err = curl_easy_perform(pCurl);
    if (err == CURLE_OK)
    {
        double sz = 0;
        curl_easy_getinfo(pCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &sz);
        nLen = (int)sz;
    }

    curl_easy_cleanup(pCurl);

    return err;
}

static size_t funcGetHttpText(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t retSize = size*nmemb;
    string* pContent = (string*)userdata;
    if (pContent)
    {
        pContent->append((char*)ptr, retSize);
    }

    return retSize;
}

CURLcode UpdateImpl::getHttpText(const string& strUrl, string& strHttpText)
{
    CURL* pCurl = curl_easy_init();
    if (!pCurl)
    {
        return CURL_LAST;
    }

    strHttpText.clear();

    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(pCurl, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(pCurl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, funcGetHttpText);
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &strHttpText);

    CURLcode err = curl_easy_perform(pCurl);
    if (err == CURLE_OK)
    {
        int sz = 0;
        curl_easy_getinfo(pCurl, CURLINFO_HTTP_CODE, &sz);
        if ((sz != 200) && (sz != 206))
        {
            //ok or not modify
            err = CURL_LAST;
        }
    }
    curl_easy_cleanup(pCurl);

    return err;
}

static size_t downLoadPackage(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    FILE *fp = (FILE*)userdata;
    size_t written = fwrite(ptr, size, nmemb, fp);

    UpdateImpl *pUpdate = &(UpdateImpl::Instance());
    uint64_t _now = client::Utility::getMillisecond();
    uint64_t _old = pUpdate->getDownloadTime();
    int nUseSecond = (_now - _old) / 1000;
    if (nUseSecond <= 0)
    {
        return written;
    }

    unsigned long sz = pUpdate->getDownloadSize() + written;
    pUpdate->setDownloadSize(sz);
    unsigned long lDownloadStartSize = pUpdate->getDownloadStartSize();
    int nSpeed = (int)((float)(sz - lDownloadStartSize) / nUseSecond / 1024);//KB

    unsigned long total = pUpdate->getDownloadTotalSize();
    if (sz > total)
    {
        sz = total;
    }

    if (nSpeed > 0 && total > 0 && sz > 0)
    {
        pUpdate->postMessage(new UpdateDownloadFileMessage(sz, total, nSpeed));
    }

    return written;
}

CURLcode UpdateImpl::download(const std::string& strFileUrl, const std::string& strSaveFile, bool bAllow3G)
{
    int nLocalFileSize = getFileSize(strSaveFile);
    if (nLocalFileSize > m_ulDownloadTotalSize)
    {
        GFileUtility::DeleteFile(strSaveFile);
        nLocalFileSize = 0;
    }

    if (!bAllow3G && client::Utility::getNetConnectionType() != client::Utility::WIFI)
    {
        m_eUpdateConfirm = UpdateImplConfirm::UPDATE_UNKNOW;

        {
            std::unique_lock<std::mutex> locker(m_mtxConfirm);
            m_bWaitForContinue = true;
            m_nLocalSize = nLocalFileSize;
        }

        while (1)
        {
            this_thread::sleep_for(chrono::milliseconds(50));
            std::unique_lock<std::mutex> locker(m_mtxConfirm);
            if (m_eUpdateConfirm == UpdateImplConfirm::UPDATE_UNKNOW)
            {
                this_thread::yield();
            }
            else
            {
                break;
            }
        }

        if (m_eUpdateConfirm == UpdateImplConfirm::UPDATE_REJECT)
        {
            //拒绝更新
            postMessage(new UserRejectMessage(UpdateState::eUpdateStateDownloadFile));
            m_eUpdateConfirm = UpdateImplConfirm::UPDATE_UNKNOW;
            return CURL_LAST;
        }
    }

    postMessage(new UpdateShowGameMessage());
    CURLcode err;
    while (true)
    {
        err = _download(strFileUrl, strSaveFile, getFileSize(strSaveFile));
        if (err == CURL_LAST || err == CURLE_OK)
        {
            postMessage(new UpdateHideGameMessage());

            return err;
        }

        if (err == CURLE_OPERATION_TIMEDOUT)
        {
            continue;
        }
    }

    return CURL_LAST;
}

CURLcode UpdateImpl::_download(const string& strFileUrl, const string& strSaveFile, int nRangeFrom)
{
    if (!m_bThreadContinue)
    {
        return CURL_LAST;
    }

    FILE *fp = nullptr;
    if (nRangeFrom > 0)
    {
        fp = fopen(strSaveFile.c_str(), "ab+");
    }
    else
    {
        fp = fopen(strSaveFile.c_str(), "wb+");
    }

    if (!fp)
    {
        return CURL_LAST;
    }

    CURL* pCurl = curl_easy_init();
    if (!pCurl)
    {
        return CURL_LAST;
    }

    char rangBuf[32] = { 0 };
    sprintf(rangBuf, "%d-", nRangeFrom);

    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(pCurl, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(pCurl, CURLOPT_URL, strFileUrl.c_str());
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, downLoadPackage);
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(pCurl, CURLOPT_RANGE, rangBuf);
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 10);

    m_ulDownloadTime = client::Utility::getMillisecond();
    m_ulDownloadStartSize = m_ulDownloadSize = nRangeFrom;

    CURLcode err = curl_easy_perform(pCurl);
    if (err == CURLE_OK)
    {
        int sz = 0;
        curl_easy_getinfo(pCurl, CURLINFO_HTTP_CODE, &sz);
        if ((sz != 200) && (sz != 206))
        {
            //ok or not modify
            err = CURL_LAST;
        }
    }

    curl_easy_cleanup(pCurl);

    fclose(fp);

    return err;
}

void UpdateImpl::startUpdate()
{
    if (!m_pUpdateDegate)
    {
        return;
    }

    if (m_trdUpdate)
    {
        //已经开始了
        return;
    }

    m_lastMsg = nullptr;

    //android多线程情况下无法用jni的一些调用》？
    GResourceHelper tmp; //调用构造函数
    CCScheduler* scheduler = CCDirector::sharedDirector()->getScheduler();
    scheduler->scheduleSelector(schedule_selector(UpdateImpl::mainUpdate), this, 0.0f, false);

    m_trdUpdate = new std::thread(&UpdateImpl::doUpdate, this);
    m_trdUpdate->detach();
}

void UpdateImpl::finish()
{
    CCScheduler* scheduler = CCDirector::sharedDirector()->getScheduler();
    scheduler->unscheduleSelector(schedule_selector(UpdateImpl::mainUpdate), this);

    initData();
}

CURLcode UpdateImpl::getServerVersion()
{
    m_nClientResVersion = getLocalClientResVersion();

    char buf[1024] = { 0 };

    m_strCheckBaseUrl = "";

    int nClientPackageVersion = GAME_CONFIG.clientVer();
    if (nClientPackageVersion <= 0)
    {
        nClientPackageVersion = UPDATE_CLIENT_PACKAGE_VERSION;
    }

    sprintf(buf, GAME_CONFIG.updateVersionUrl().c_str(), nClientPackageVersion, m_nClientResVersion);

    string strVersionCode;
    CURLcode err = getHttpText(buf, strVersionCode);
    if (err != CURLE_OK)
    {
        return err;
    }

    if (strVersionCode.empty())
    {
        return CURL_LAST;
    }

    m_strServerVersionText = "";
    bool bParseOk = false;
    sprintf(buf, "%s", strVersionCode.c_str());
    char *sigStr = strtok((char*)buf, ";");
    if (sigStr)
    {
        m_nDownloadType = atoi(sigStr);
        switch (m_nDownloadType)
        {
            case 1:
            {
                sigStr = strtok(nullptr, ";");
                if (sigStr)
                {
                    m_strServerVersionText = sigStr;
                }

                bParseOk = !m_strServerVersionText.empty();

                break;
            }
            case 2:
            {
                sigStr = strtok(nullptr, ";");
                if (sigStr)
                {
                    m_nServerResVersion = atoi(sigStr);

                    sigStr = strtok(nullptr, ";");

                    if (m_nServerResVersion > 0 && sigStr)
                    {
                        m_strDownloadMd5 = sigStr;

                        sigStr = strtok(nullptr, ";");

                        if (!m_strDownloadMd5.empty() && sigStr)
                        {
                            m_strDownloadingUrl = sigStr;

                            sigStr = strtok(nullptr, ";");
                            if (!m_strDownloadingUrl.empty() && sigStr)
                            {
                                m_strCheckBaseUrl = sigStr;
                                bParseOk = true;
                            }
                        }
                    }
                }

                break;
            }
            case 3:
            {
                sigStr = strtok(nullptr, ";");
                if (sigStr)
                {
                    m_nServerResVersion = atoi(sigStr);
                    sigStr = strtok(nullptr, ";");

                    if (m_nServerResVersion > 0 && sigStr)
                    {
                        m_strDownloadMd5 = sigStr;

                        sigStr = strtok(nullptr, ";");

                        if (!m_strDownloadMd5.empty() && sigStr)
                        {
                            m_strDownloadingUrl = sigStr;
                            sigStr = strtok(nullptr, ";");
                            if (!m_strDownloadingUrl.empty() && sigStr)
                            {
                                m_strCheckBaseUrl = sigStr;
                                bParseOk = true;
                            }
                        }
                    }
                }

                break;
            }
            case 4:
            {
                bParseOk = true;

                break;
            }
            default:
            {
                break;
            }
        }
    }

    return bParseOk ? CURLE_OK : CURL_LAST;
}

bool UpdateImpl::parseDecompressData()
{
    string strDocumentPath = GResourceHelper::GetDocumentPath();
    string strIndexFile = strDocumentPath + "index.dat";
    vector<TByte> fileData;
    readFile(fileData, strIndexFile);
    if (fileData.empty())
    {
        return false;
    }

    size_t szFileData = fileData.size();
    for (int i = 0; i < szFileData; ++i)
    {
        fileData[i] ^= (i % 63 + 1);
    }

    postMessage(new UpdateParseDiffFileMessage(0.1f));

    TByte *pData = (TByte*)&fileData[0];
    TByte *pDataEnd = pData + fileData.size();
    const TByte** pDataBegin = (const TByte**)&pData;

    int nResVersion = unpack32Bit(pDataBegin, pDataEnd);
    if (m_nServerResVersion != nResVersion)
    {
        //神马情况，出错了？
        return false;
    }

    postMessage(new UpdateParseDiffFileMessage(0.15f));

    list<string> deleteFiles;
    m_mapFileWithMD5.clear();

    int nDeleteUnUsedFiles = unpack32Bit(pDataBegin, pDataEnd);
    for (int i = 0; i < nDeleteUnUsedFiles; ++i)
    {
        deleteFiles.push_back(unPackString(pDataBegin, pDataEnd));
    }

    int nFilesNum = unpack32Bit(pDataBegin, pDataEnd);
    for (int i = 0; i < nFilesNum; ++i)
    {
        string strFile = unPackString(pDataBegin, pDataEnd);
        string strMD5 = unPackString(pDataBegin, pDataEnd);

        m_mapFileWithMD5[strFile] = strMD5;
    }

    int nPos = 0;
    for (auto &x : deleteFiles)
    {
        float fTmpRate = (float)nPos++ / nDeleteUnUsedFiles * 0.7f;
        postMessage(new UpdateParseDiffFileMessage(0.3f + fTmpRate));
        GFileUtility::DeleteFile(strDocumentPath + x);
    }

    return true;
}

void UpdateImpl::checkClient()
{
    if (m_strCheckBaseUrl.empty())
    {
        return;
    }

    list<string> lstNeedDownload;
    string strDocumentPath = GResourceHelper::GetDocumentPath();
    string strFileName = "";

    for (auto &x : m_mapFileWithMD5)
    {
        strFileName = strDocumentPath + x.first;
        if (!compareMd5(getFileMd5(strFileName), x.second))
        {
            lstNeedDownload.push_back(x.first);
        }
    }

    char downloadUrl[1024] = { 0 };
    string destFile;
    for (auto &z : lstNeedDownload)
    {
        sprintf(downloadUrl, "%s/%s", m_strCheckBaseUrl.c_str(), z.c_str());
        destFile = strDocumentPath + z;
        download(downloadUrl, destFile, false);
    }
}

void UpdateImpl::cleanUp()
{
    string strDocumentPath = GResourceHelper::GetDocumentPath();
    string strIndexFile = strDocumentPath + "index.dat";

    GFileUtility::DeleteFile(strIndexFile);
    GFileUtility::DeletePath(GResourceHelper::GetDownloadPath());
}

bool UpdateImpl::downloadAndDecompress(const string& downLoadFile)
{
    CURLcode err = CURLE_OK;
    GFileUtility::CreateDirectoryFromFilePath(downLoadFile);

    if (!compareMd5(m_strDownloadMd5, getFileMd5(downLoadFile)))
    {
        //md5 一致
        int nRemoteSize = 0;
        err = getRemoteFileSize(m_strDownloadingUrl, nRemoteSize);
        if (err != CURLE_OK || nRemoteSize <= 0)
        {
            //获取失败
            postMessage(new UpdateFailedMessage(err, UpdateState::eUpdateStateDownloadFile));
            return false;
        }
        m_ulDownloadTotalSize = nRemoteSize;

        postMessage(new UpdateDownloadFileMessage(0, 0, 0.f));
        err = download(m_strDownloadingUrl, downLoadFile, false);
        if (err != CURLE_OK)
        {
            //下载出问题了？
            postMessage(new UpdateFailedMessage(err, UpdateState::eUpdateStateDownloadFile));
            return false;
        }
    }

    if (!compareMd5(m_strDownloadMd5, getFileMd5(downLoadFile)))
    {
        //还是不一致，出问题了吧
        GFileUtility::DeleteFile(downLoadFile);
        postMessage(new UpdateFailedMessage(err, UpdateState::eUpdateStateDownloadFile));
        return false;
    }

    if (!decompress(downLoadFile, GResourceHelper::m_strWritableDir))
    {
        //解压失败，fk
        postMessage(new UpdateFailedMessage(CURL_LAST, UpdateState::eUpdateStateDecompress));
        return false;
    }

    if (!parseDecompressData())
    {
        //解析失败，fk
        postMessage(new UpdateFailedMessage(CURL_LAST, UpdateState::eUpdateStateParseDiffFile));
        return false;
    }

    //check_client
    postMessage(new UpdateCheckClientMessage());
    checkClient();

    //cleanup
    postMessage(new UpdateCleanCacheMessage());
    cleanUp();

    updateLocalClientResVersion(m_nServerResVersion);

    postMessage(new UpdateFinishMessage());

    return true;
}

void UpdateImpl::fullresUpdate()
{
    char strDownloadFile[1024] = { 0 };
    sprintf(strDownloadFile, "%s%d.zip", GResourceHelper::GetDownloadPath().c_str(), m_nServerResVersion);
    downloadAndDecompress(strDownloadFile);
}

void UpdateImpl::diffUpdate()
{
    char strLocalDiffFile[1024] = { 0 };
    sprintf(strLocalDiffFile, "%s%d-%d.zip", GResourceHelper::GetDownloadPath().c_str(),
        m_nServerResVersion, getLocalClientResVersion());
    downloadAndDecompress(strLocalDiffFile);
}

void UpdateImpl::doUpdate()
{
    CURLcode err = CURLE_OK;
    postMessage(new UpdateBeginMessage());

    this_thread::sleep_for(chrono::milliseconds(500));
    err = getServerVersion();
    if (err != CURLE_OK)
    {
        //获取版本号失败
        postMessage(new UpdateFailedMessage(err, UpdateState::eUpdateStateBegin));
        return;
    }

    postMessage(new UpdateCheckClientVersionMessage());
    this_thread::sleep_for(chrono::milliseconds(500));
    switch (m_nDownloadType)
    {
        case 1:
        {
            //大版本不一致，去平台更新吧
            UpdateNeedUpdateMessage *pMsg = new UpdateNeedUpdateMessage;
            pMsg->setMsg(m_strServerVersionText);
            postMessage(pMsg);

            break;
        }
        case 2:
        {
            //完整下载
            fullresUpdate();

            break;
        }
        case 3:
        {
            //差异包更新
            diffUpdate();

            break;
        }
        case 4:
        {
            //版本成功
            postMessage(new UpdateFinishMessage());

            break;
        }
        default:
        {
            //获取失败
            postMessage(new UpdateFailedMessage(CURL_LAST, UpdateState::eUpdateStateCheckNewVersion));

            break;
        }
    }
}

#define BUFFER_SIZE 8192
bool UpdateImpl::decompress(const string& strZipFile, const string& destPath)
{
    unzFile zipfile = unzOpen(strZipFile.c_str());
    if (!zipfile)
    {
        CCLOG("can not open downloaded zip file %s", strZipFile.c_str());
        return false;
    }

    unz_global_info global_info;
    if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
    {
        CCLOG("can not read file global info of %s", strZipFile.c_str());
        unzClose(zipfile);
        return false;
    }

    CCLOG("start uncompressing");

    char readBuffer[BUFFER_SIZE];
    for (uLong i = 0; i < global_info.number_entry; ++i)
    {
        postMessage(new UpdateDecompressMessage((float)i / global_info.number_entry));

        unz_file_info fileInfo;
        char fileName[512] = { 0 };
        if (unzGetCurrentFileInfo(zipfile, &fileInfo, fileName, 512, nullptr, 0, nullptr, 0) != UNZ_OK)
        {
            CCLOG("can not read file info");
            unzClose(zipfile);
            return false;
        }

        string fullPath = destPath + fileName;
        const size_t filenameLength = strlen(fileName);
        if (fileName[filenameLength - 1] == '/')
        {
            GFileUtility::CreateDirectory(fullPath);
        }
        else
        {
            if (unzOpenCurrentFilePassword(zipfile, GAME_CONFIG.resourcePasswd().c_str()) != UNZ_OK)
            {
                CCLOG("can not open file %s", fileName);
                unzClose(zipfile);
                return false;
            }

            FILE *out = fopen(fullPath.c_str(), "wb");
            if (!out)
            {
                CCLOG("can not open destination file %s", fullPath.c_str());
                unzCloseCurrentFile(zipfile);
                unzClose(zipfile);
                return false;
            }

            int error = UNZ_OK;
            do
            {
                error = unzReadCurrentFile(zipfile, readBuffer, BUFFER_SIZE);
                if (error < 0)
                {
                    CCLOG("can not read zip file %s, error code is %d", fileName, error);
                    unzCloseCurrentFile(zipfile);
                    unzClose(zipfile);
                    return false;
                }

                if (error > 0)
                {
                    fwrite(readBuffer, error, 1, out);
                }
            }
            while (error > 0);
            fclose(out);
        }

        unzCloseCurrentFile(zipfile);
        if ((i + 1) < global_info.number_entry)
        {
            if (unzGoToNextFile(zipfile) != UNZ_OK)
            {
                CCLOG("can not read next file");
                unzClose(zipfile);
                return false;
            }
        }
    }

    unzClose(zipfile);

    CCLOG("end uncompressing");

    return true;
}

void UpdateImpl::ConfirmContinue()
{
    std::lock_guard<std::mutex> autoLock(m_mtxConfirm);
    m_eUpdateConfirm = UpdateImplConfirm::UPDATE_CONTINUE;
}

void UpdateImpl::ConfirmReject()
{
    std::lock_guard<std::mutex> autoLock(m_mtxConfirm);
    m_eUpdateConfirm = UpdateImplConfirm::UPDATE_REJECT;
}

void UpdateImpl::postMessage(UpdateMessage* pMsg)
{
    if (!pMsg)
    {
        return;
    }

    std::lock_guard<std::mutex> autoRelease(m_mtxState);
    if (m_lastMsg)
    {
        delete m_lastMsg;
        m_lastMsg = nullptr;
    }

    UpdateState state = pMsg->getState();
    if (state == UpdateState::eUpdateStateShowSmallGame || state == UpdateState::eUpdateStateHideSmallGame)
    {
        m_lstSmallGameMsg.push_back(pMsg);

        return;
    }

    m_lastMsg = pMsg;

    if (m_lastMsg->getStopProcess())
    {
        delete m_trdUpdate;
        m_trdUpdate = nullptr;
    }

}

void UpdateImpl::mainUpdate(float dt)
{
    {
        std::unique_lock<std::mutex> locker(m_mtxConfirm);
        if (m_bWaitForContinue)
        {
            GCommonAlertViewData alertViewData;
            char buf[256] = { 0 };
            int nSize = (m_ulDownloadTotalSize - m_nLocalSize);
            float fMB = (float)nSize / 1024 / 1024;
            sprintf(buf, sLanguageManager->getLanguageByKey("RESOURCE_VERSION_SIZE_MAX").c_str(), fMB);
            alertViewData.message = buf;
            alertViewData.okButtonText = sLanguageManager->getLanguageByKey("Sure");
            alertViewData.cancelButtonText = sLanguageManager->getLanguageByKey("Cancel");

            GCommonAlertView* pAlertView = GAlertViewManager::CreateAlertView<GCommonAlertView>(&alertViewData);
            GAlertViewManager::ShareInstance()->ShowAlertView();
            pAlertView->SetOkCallBack(this, callfunc_selector(UpdateImpl::ConfirmContinue), true);
            pAlertView->SetCancelCallBack(this, callfunc_selector(UpdateImpl::ConfirmReject), true);

            m_bWaitForContinue = false;
        }
    }

    if (!m_lstSmallGameMsg.empty())
    {
        UpdateMessage* pMsg = m_lstSmallGameMsg.front();
        UpdateState state = pMsg->getState();
        if (state == UpdateState::eUpdateStateShowSmallGame)
        {
            RdSampleGameImpl::Instance().changeSampleGameState(RdSampleGameImpl::SAMPLE_GAME_STATE_SHOW);
            m_pUpdateDegate->PlayAni("sample_game_show");
        }
        else
        {
            RdSampleGameImpl::Instance().changeSampleGameState(RdSampleGameImpl::SAMPLE_GAME_STATE_UNSTART);
            m_pUpdateDegate->PlayAni("sample_game_hide");
        }

        delete pMsg;
        m_lstSmallGameMsg.pop_front();
    }

    std::lock_guard<std::mutex> autoRelease(m_mtxState);
    if (!m_lastMsg)
    {
        return;
    }

    bool bStop = m_lastMsg->getStopProcess();
    if (m_pUpdateDegate)
    {
        m_pUpdateDegate->SendMessage(m_lastMsg);
    }

    if (m_lastMsg->getState() == UpdateState::eUpdateFinish)
    {
        m_pUpdateDegate = nullptr;
    }

    delete m_lastMsg;
    m_lastMsg = nullptr;

    if (bStop)
    {
        finish();
    }
}
