//
//  GFileUtility.cpp
//  Cocos2dXTest
//
//  Created by ldr123 on 12-5-7.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#include "GFileUtility.h"
#include "cocos2d.h"
#include "GUtility.h"
#include <algorithm>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

using namespace std;
using namespace cocos2d;

bool GFileUtility::IsFileExist(const std::string &fileFullPath)
{
    string strFile = CCFileUtils::sharedFileUtils()->fullPathForFilename(fileFullPath.c_str());
    if (CCFileUtils::sharedFileUtils()->isFileExist(strFile.c_str()))
    {
        return true;
    }
    else 
    {
//         CCLog("[GFileUtility::IsFileExist] file(%s) don't exist!",fileFullPath.c_str());
        return false;
    }
}

bool GFileUtility::FixDirectorySuffix(std::string &path)
{
	int nSize = path.size();
	if (nSize == 0)
	{
		return false;
	}

	std::replace(path.begin(), path.end(), '\\', '/');

	char c = path[nSize - 1];
	if (c != '/' && c != '\\')
	{
		path += "/";
	}

	return true;
}

bool GFileUtility::GetFileNameFromFilePath(const std::string& filePath,std::string& fileName)
{
    if (filePath.empty())
    {
        return false;
    }
    size_t pathSeperatorPos = filePath.rfind('/');
    fileName = filePath.substr(pathSeperatorPos+1);
    return true;
}

bool GFileUtility::IsFileExistInZip(const std::string& fileFullPath)
{
    GCCFileData data(fileFullPath.c_str(), "rt");
	unsigned long size = data.getSize();
    if (size > 0) 
    {
        return true;
    }
    return false;
}

bool GFileUtility::RenameFileName(const std::string &fileOldFullPath, const std::string &fileNewFullPath)
{
    //int rename(char *oldname, char *newname);
    if ( rename(fileOldFullPath.c_str(), fileNewFullPath.c_str()) == 0 )
    {
        return true;
    }
    else 
    {
        return false;
    }
}

bool GFileUtility::DeletePath(const std::string& _strFullPath)
{
	string strFullPath = _strFullPath;
	FixDirectorySuffix(strFullPath);
#ifdef _WIN32
	_finddata_t  fileInfo;
	string strFind = strFullPath + "*";
	long nHandle = _findfirst(strFind.c_str(), &fileInfo);
	if (nHandle != -1)
	{
		do
		{
			if (fileInfo.attrib & _A_SUBDIR)
			{
				if ((strcmp(fileInfo.name, ".") != 0) && (strcmp(fileInfo.name, "..") != 0))
				{
					DeletePath(strFullPath + fileInfo.name);
				}
			}
			else
			{
				string filename = (strFullPath + fileInfo.name);
				DELETE_FILE(filename.c_str());
			}
		} while (_findnext(nHandle, &fileInfo) == 0);

		_findclose(nHandle);
	}
#else
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
    if ((dp = opendir(strFullPath.c_str())) == nullptr)
	{
        return false;
	}

	chdir(strFullPath.c_str());
    while ((entry = readdir(dp)) != nullptr)
	{
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode))
		{
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				continue;

			DeletePath(entry->d_name);
		}
		else
		{
			string filename = entry->d_name;
			DELETE_FILE(filename.c_str());
		}
	}

	chdir("..");
	closedir(dp);
#endif 

	REMOVE_DIRECTORY(strFullPath.c_str());

	return true;
}

bool GFileUtility::DeleteFile(const std::string &fileFullPath)
{		
	return DELETE_FILE(fileFullPath.c_str());
}

bool GFileUtility::CopyFile(const std::string& fileOldFullPath, const std::string& fileNewFullPath)
{
    if( !GFileUtility::IsFileExist(fileOldFullPath) )
    {
        CCLog("[GFileUtility] Error:(CopyFile) file(%s) don't exist!",fileOldFullPath.c_str());
        return false;
    }

    FILE* source_file = fopen(fileOldFullPath.c_str(), "rb");
    if (source_file == nullptr)
    {
        CCLog("[GFileUtility] Error:(CopyFile) open file(%s) failed!",fileOldFullPath.c_str());
        return false;
    }
    
    if (!GFileUtility::CreateDirectoryFromFilePath(fileNewFullPath)) 
    {
        CCLog("[GFileUtility] Error:(CopyFile) create directory for file(%s) failed!",fileNewFullPath.c_str());
        return false;
    }

	fseek(source_file, 0, SEEK_END);
	int file_length = (int)ftell(source_file);
	fseek(source_file, 0, SEEK_SET);

	unsigned char *buf = new unsigned char[file_length];
	fread(buf, 1, file_length, source_file);
	fclose(source_file);

    FILE* destination_file = fopen(fileNewFullPath.c_str(), "wb");
	if (destination_file)
	{
		fwrite(buf, 1, file_length, destination_file);
		fclose(destination_file);
	}
	else
    {
		delete []buf;
        CCLog("[GFileUtility] Error:(CopyFile) open file(%s) failed!",fileNewFullPath.c_str());
        return false;
    }
    
	delete []buf;

    return true;
}

bool GFileUtility::CopyFileFromZip(const std::string& fileOldFullPath, const std::string& fileNewFullPath)
{
    GCCFileData data(fileOldFullPath.c_str(), "rt");
	unsigned long size = data.getSize();
	char *pBuffer = (char*) data.getBuffer();
    
    FILE* new_file = fopen(fileNewFullPath.c_str(), "w");
    if (new_file == nullptr)
    {
        return false;
    }
    
    if(size != fwrite(pBuffer, 1, size, new_file) )
    {
        fclose(new_file);
        return false;
    }
    fclose(new_file);
    return true;
}

//  e.g:
//  input:  fileFullPath = "Document/TempDirLevelOne/TempDirLevelTwo/test"
//  output: makedir(Document/TempDirLevelOne) makedir(Document/TempDirLevelOne/TempDirLevelTwo)
bool GFileUtility::CreateDirectory(const std::string &_fileFullPath)
{
	string fileFullPath = _fileFullPath;
	if (!FixDirectorySuffix(fileFullPath))
	{
		return false;
	}

	string strTmp = "";
	for (int i = 0; i < fileFullPath.size(); ++i)
	{
		strTmp += fileFullPath[i];
		if (fileFullPath[i] == '/')
		{
			if (!IsFileExist(strTmp))
			{
				CREATE_DIRECTORY(strTmp.c_str());
			}
		}
	}

	return true;
}

bool GFileUtility::CreateDirectoryFromFilePath(const std::string& file)
{
	int nSize = file.size();
	if (nSize == 0)
	{
		return false;
	}

	string fileFullPath = file;
	std::replace(fileFullPath.begin(), fileFullPath.end(), '\\', '/');
	size_t nPos = fileFullPath.rfind('/');
	if (nPos == string::npos)
	{
		return false;
	}

	fileFullPath = file.substr(0, nPos);
	return CreateDirectory(fileFullPath);
}

bool GFileUtility::CopyDirectory(const std::string &from, const std::string &to)
{
	return _CopyDirectory(from, from, to);
}

bool GFileUtility::_CopyDirectory(const string &_fromStart, const string &_from, const string &_to)
{
	string strFromStart = _fromStart;
	FixDirectorySuffix(strFromStart);

	string strFrom = _from;
	FixDirectorySuffix(strFrom);

	string strTo = _to;
	FixDirectorySuffix(strTo);

#ifdef _WIN32
	_finddata_t  fileInfo;
	string strFind = strFrom + "*";
	long nHandle = _findfirst(strFind.c_str(), &fileInfo);
	if (nHandle != -1)
	{
		do
		{
			if (fileInfo.attrib & _A_SUBDIR)
			{
				if ((strcmp(fileInfo.name, ".") != 0) && (strcmp(fileInfo.name, "..") != 0))
				{
					return _CopyDirectory(_fromStart, strFrom + fileInfo.name, strTo);
				}
			}
			else
			{
				string filename = (strFrom + fileInfo.name);
				string strRelative = filename;

				strRelative = strRelative.substr(_fromStart.size());
				CopyFile(filename, strTo + strRelative);
			}
		} while (_findnext(nHandle, &fileInfo) == 0);

		_findclose(nHandle);
	}
#else
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
    if ((dp = opendir(_from.c_str())) == nullptr)
	{
		return false;
	}

	chdir(_from.c_str());
    while ((entry = readdir(dp)) != nullptr)
	{
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode))
		{
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				continue;

			return _CopyDirectory(_fromStart, entry->d_name, strTo);
		}
		else
		{
			string filename = entry->d_name;
			string strRelative = filename;

			strRelative = strRelative.substr(_fromStart.size());
			CopyFile(filename, strTo + strRelative);
		}
	}

	chdir("..");
	closedir(dp);
#endif 

	return true;
}
