//
//  GFileUtility.h
//  Cocos2dXTest
//
//  Created by ldr123 on 12-5-7.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#ifndef Cocos2dXTest_GFileUtility_
#define Cocos2dXTest_GFileUtility_

#ifdef WIN32
#   define CREATE_DIRECTORY(P)				(::CreateDirectoryA(P, 0)!= 0)
#   define REMOVE_DIRECTORY(P)				(::RemoveDirectoryA(P)!= 0)
#   define DELETE_FILE(P)					(::DeleteFileA(P)!= 0)
#else
#include <unistd.h>

#   define CREATE_DIRECTORY(P)				(::mkdir(P, S_IRWXU|S_IRWXG|S_IRWXO)== 0)
#   define REMOVE_DIRECTORY(P)				(::rmdir(P)== 0)
#   define DELETE_FILE(P)					(::unlink(P)== 0)
# endif

#include "cocos2d.h"

class GFileUtility
{
public:
    static bool IsFileExist(const std::string& fileFullPath);
    static bool IsFileExistInZip(const std::string& fileFullPath);
    static bool RenameFileName(const std::string& fileOldFullPath, const std::string& fileNewFullPath);
    static bool DeleteFile(const std::string& fileFullPath);
	static bool DeletePath(const std::string& strFullPath);
    static bool CopyFile(const std::string& fileOldFullPath, const std::string& fileNewFullPath);
    static bool CopyFileFromZip(const std::string& fileOldFullPath, const std::string& fileNewFullPath);
    static bool GetFileNameFromFilePath(const std::string& filePath,std::string& fileName);
    /**
     @ Function:    CreateDirectory
     @ in:          fileRelativePath
     @ out return:  true(create directory successful) false(create directory failed)
     @ Note:        if directory is exist, return true
     */
    static bool CreateDirectory(const std::string& fileRelativePath);
	static bool CreateDirectoryFromFilePath(const std::string& file);
	static bool CopyDirectory(const std::string &from, const std::string &to);
private:
	static bool _CopyDirectory(const std::string &_fromStart, const std::string &from, const std::string &to);

	static bool FixDirectorySuffix(std::string &path);
};

class GCCFileData
{
public:
    GCCFileData(const char* pszFileName, const char* pszMode)
    : m_pBuffer(0)
    , m_uSize(0)
    {
        m_pBuffer = cocos2d::CCFileUtils::sharedFileUtils()->getFileData(pszFileName, pszMode, &m_uSize);
    }
    ~GCCFileData()
    {
        CC_SAFE_DELETE_ARRAY(m_pBuffer);
    }
    
    bool reset(const char* pszFileName, const char* pszMode)
    {
        CC_SAFE_DELETE_ARRAY(m_pBuffer);
        m_uSize = 0;
        m_pBuffer = cocos2d::CCFileUtils::sharedFileUtils()->getFileData(pszFileName, pszMode, &m_uSize);
        return (m_pBuffer) ? true : false;
    }
    CC_SYNTHESIZE_READONLY(unsigned char *, m_pBuffer, Buffer);
    CC_SYNTHESIZE_READONLY(unsigned long ,  m_uSize,   Size);
};
#endif
