//
//  GUtility.cpp
//
//  Created by ldr123 on 13-3-28.
//  Copyright (c) 2013年 __MyCompanyName__. All rights reserved.
//

#include "GUtility.h"
#include "cocos-ext.h"
using namespace cocos2d;
using namespace cocos2d::extension;
using namespace std;
using namespace cocos2d;

bool IsNodeShowing(cocos2d::CCNode* node)
{
    return IsNodeNeedShow(node);
}

bool IsNodeNeedShow(cocos2d::CCNode* node)
{
    if (node)
    {
        if (node->isVisible())
        {
            return IsNodeNeedShow(node->getParent());
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool IsUserOperationIsClick(cocos2d::CCPoint& beginPos,cocos2d::CCPoint& endPoint)
{
#define kMaxDeltaDistanceBetweenClickPoint 20.0f
    float deltaX = beginPos.x - endPoint.x;
    float deltaY = beginPos.y - endPoint.y;
    
    if (fabsf(deltaX) < kMaxDeltaDistanceBetweenClickPoint &&
        fabsf(deltaY) < kMaxDeltaDistanceBetweenClickPoint)
    {
        return true;
    }
    else
    {
        GUILOG("[GAbstractTouchLayer::IsUserOperationIsClick] Info: \n deltaX:%f \n deltaY:%f", deltaX, deltaY);
        return false;
    }
}


void GetNodeScaleValueHelper(cocos2d::CCNode* node,cocos2d::CCSize& size)
{
    if (node)
    {
        size.width  *= node->getScaleX();
        size.height *= node->getScaleY();
        CCNode* parent = node->getParent();
        if (parent)
        {
            GetNodeScaleValueHelper(parent, size);
        }
    }
}

cocos2d::CCSize GetNodeScaleValue(cocos2d::CCNode* node)
{
    cocos2d::CCSize size;
    size.width  = 1.0f;
    size.height = 1.0f;
    GetNodeScaleValueHelper(node, size);
    return size;
}

GENormalAnchorPosType ConvertARToARType(const cocos2d::CCPoint& anchor)
{
    GENormalAnchorPosType anchorType = kGAnchorType_Invalid;
    if (anchor.equals(kGAnchor_LeftTop))
    {
        anchorType = kGAnchorType_LeftTop;
    }
    else if (anchor.equals(kGAnchor_LeftMid))
    {
        anchorType = kGAnchorType_LeftMid;
    }
    else if (anchor.equals(kGAnchor_LeftBottom))
    {
        anchorType = kGAnchorType_LeftBottom;
    }
    else if (anchor.equals(kGAnchor_RightTop))
    {
        anchorType = kGAnchorType_RightTop;
    }
    else if (anchor.equals(kGAnchor_RightMid))
    {
        anchorType = kGAnchorType_RightMid;
    }
    else if (anchor.equals(kGAnchor_RightBottom))
    {
        anchorType = kGAnchorType_RightBottom;
    }
    else if (anchor.equals(kGAnchor_Mid))
    {
        anchorType = kGAnchorType_Mid;
    }
    else if (anchor.equals(kGAnchor_TopMid))
    {
        anchorType = kGAnchorType_TopMid;
    }
    else if (anchor.equals(kGAnchor_BottomMid))
    {
        anchorType = kGAnchorType_BottomMid;
    }
    else
    {
        CCAssert(false, "Error: unsupport anchor!");
    }
    return anchorType;
}

cocos2d::CCPoint GetAchorWith(GENormalAnchorPosType achorPosType)
{
    CCPoint tempAnchor = CCPointZero;
    switch (achorPosType) {
        case kGAnchorType_LeftBottom:
            tempAnchor = CCPointMake(0.0, 0.0);
            break;
        case kGAnchorType_LeftMid:
            tempAnchor = CCPointMake(0.0, 0.5);
            break;
        case kGAnchorType_LeftTop:
            tempAnchor = CCPointMake(0.0, 1.0);
            break;
        case kGAnchorType_RightBottom:
            tempAnchor = CCPointMake(1.0, 0.0);
            break;
        case kGAnchorType_RightMid:
            tempAnchor = CCPointMake(1.0, 0.5);
            break;
        case kGAnchorType_RightTop:
            tempAnchor = CCPointMake(1.0, 1.0);
            break;
        case kGAnchorType_BottomMid:
            tempAnchor = CCPointMake(0.5, 0.0);
            break;
        case kGAnchorType_Mid:
            tempAnchor = CCPointMake(0.5, 0.5);
            break;
        case kGAnchorType_TopMid:
            tempAnchor = CCPointMake(0.5, 1.0);
            break;
        default:
            CCAssert(false, "Error: Unsupport anchor type!");
            break;
    }
    return tempAnchor;
}

cocos2d::CCPoint ConvertNodeGivenARPosToRealARPos(cocos2d::CCNode* node,
                                                  const cocos2d::CCPoint& givenARPos,
                                                  GENormalAnchorPosType givenARPosType,
                                                  bool isUseBoundingBox)
{
    GENormalAnchorPosType givenARPosBType = ConvertARToARType(node->getAnchorPoint());
    return ConvertNodeGivenARPosAToB(node, givenARPos, givenARPosType, givenARPosBType, isUseBoundingBox);
}

cocos2d::CCPoint ConvertNodeGivenARPosAToB(cocos2d::CCNode* node,
                                           const cocos2d::CCPoint& givenARPosA,
                                           GENormalAnchorPosType givenARPosAType,
                                           GENormalAnchorPosType givenARPosBType,
                                           bool isUseBoundingBox)
{
    CCPoint tempAnchorA = GetAchorWith(givenARPosAType);
    CCPoint tempAnchorB = GetAchorWith(givenARPosBType);
    CCSize contentSize = isUseBoundingBox ? node->boundingBox().size : node->getContentSize();
    return ConvertGivenARPosAToB(contentSize, givenARPosA, tempAnchorA, tempAnchorB);
}

cocos2d::CCPoint ConvertNodeRealARPosToGivenARPos(cocos2d::CCNode* node,GENormalAnchorPosType givenARPosType,
                                                  bool isUseBoundingBox)
{
    const cocos2d::CCPoint& nodePos = node->getPosition();
    GENormalAnchorPosType nodeARType = ConvertARToARType(node->getAnchorPoint());
    return ConvertNodeGivenARPosAToB(node, nodePos, nodeARType, givenARPosType, isUseBoundingBox);
}

cocos2d::CCPoint ConvertGivenARPosAToB(const cocos2d::CCSize& size,
                                       const cocos2d::CCPoint& givenARPosA,
                                       const cocos2d::CCPoint& givenARA,
                                       const cocos2d::CCPoint& givenARB)
{
    cocos2d::CCPoint temp;
    temp.x = (givenARB.x - givenARA.x) * size.width + givenARPosA.x;
    temp.y = (givenARB.y - givenARA.y) * size.height + givenARPosA.y;
    return temp;
}

int GUtility::StringToInt(const char *str)
{
    return atoi(str);
}

float GUtility::StringToFloat(const char *str)
{  
    return atof(str);
}

bool GUtility::StringToBool(const string& str)
{
    if (str == "YES" || str == "true" || str == "1")
    {
        return true;
    }
    else if(str == "NO" || str == "false" || str == "0")
    {
        return false;
    }
    else
    {
        CCLOG("[GUtility::StringToBool] Error:%s",str.c_str());
        CCAssert(false,"Invalid bool string");
    }
    return false;
}

string GUtility::IntToString(int value)
{
    char temp[64];
    memset(temp, 0, 64);
    sprintf(temp, "%d", value);
    return std::string(temp);
}

string GUtility::LongLongToString(long long value)
{
    char temp[65];
    memset(temp, 0, 65);
    sprintf(temp, "%lld",value);
    return std::string(temp);
}

string GUtility::ListIntToString(const std::list<int>& listIntegers,char splite_mark)
{
    string temp;
    string mark(1,splite_mark);
    std::list<int>::const_iterator begin = listIntegers.begin();
    for (; begin !=listIntegers.end(); begin++)
    {
        temp += IntToString(*begin);
        temp += mark;
    }
    if (temp.length() > 0)
    {
        temp.erase(temp.end()-1);
    }
    return temp;
}

bool GUtility::StringToListInt(const std::string& str, char splite_mark, std::list<int>&intergerList)
{
    vector<string> stringVector;
    if( !GUtility::SplitString(str, splite_mark, stringVector) )
    {
        return false;
    }
    else
    {
        for (int i=0; i<stringVector.size(); i++)
        {
            int temp = StringToInt(stringVector.at(i).c_str());
            intergerList.push_back(temp);
        }
    }
    return true;
}

string GUtility::FloatToString(float value)
{
    char temp[64];
    memset(temp, 0, 64);
    sprintf(temp, "%f", value);
    return std::string(temp);
}

CCTextAlignment GUtility::StringToTextAlignment(const char* str)
{
    return GUtility::StringToTextAlignment(std::string(str));
}

CCTextAlignment GUtility::StringToTextAlignment(const std::string& str)
{
    if (str == "Center")
    {
        return cocos2d::kCCTextAlignmentCenter;
    }
    else if (str == "Right")
    {
        return cocos2d::kCCTextAlignmentRight;
    }
    else if (str == "Left")
    {
        return cocos2d::kCCTextAlignmentLeft;
    }
    else
    {
        CCLOG("[GUtility::StringToBool] Error:%s",str.c_str());
        CCAssert(false, "Invalid CCTextAlignment string");
    }
    return cocos2d::kCCTextAlignmentCenter;
}

std::string GUtility::CharVectorToString(const std::vector<char>* charVector)
{
    string temp;
    if (charVector)
    {
        if (charVector->size() > 0)
        {
            //Fix: char* 以\0为结尾，所以需要设置最后一个字符为\0
            char* tempBuff = new char[charVector->size()+1];
            memset(tempBuff, 0, charVector->size()+1);
            for (unsigned int i = 0; i < charVector->size(); i++)
            {
                tempBuff[i] = (*charVector)[i];
            }
            temp = tempBuff;
            delete[] tempBuff;
        }
    }
    return temp;
}

bool GUtility::IsEqual(const char *str1, const char *str2)
{
    if ( 0 == strcmp(str1,str2) )
    {
        return true;
    }
    return false;
}

void GUtility::TrimmedString(std::string & src)
{
    string::iterator iter = src.begin();
    while (*iter == '\t' || *iter == ' ' || *iter == '\n')
    {
        src.erase(iter);
        iter = src.begin();
    }
    iter = --src.end();
    while (*iter == '\t' || *iter == ' ' || *iter == '\n')
    {
        src.erase(iter);
        iter = --src.end();
    }
}

std::string GUtility::StringVectorToString(const std::vector<std::string>& input, const std::string& spliteStr)
{
    std::string temp;
    std::vector<std::string>::const_iterator iter = input.begin();
    while (iter!=input.end())
    {
        temp += *iter;
        
        iter++;
        
        if (iter == input.end())
        {
            break;
        }
        
        temp += spliteStr;
    }
    return temp;
}

char* GUtility::Long2IP(unsigned long longIp)
{
    unsigned char *p;
    p = (unsigned char*)(&longIp);
    static char output[16];
    snprintf(output, 16,"%d.%d.%d.%d",p[0],p[1],p[2],p[3]);
    return output;
}

unsigned long GUtility::IP2Long(char *ip)
{
    unsigned char addr[16];
    sscanf(ip,"%d.%d.%d.%d",(int*)(addr),(int*)(addr+1),(int*)(addr+2),(int*)(addr+3));
    unsigned long *vl = (unsigned long*)(&addr);
    return *vl;
}

//  e.g:
//  source_str      = "/TEST/test//TEST"
//  splite_mark     = '/'
//  splited_string  = {"TEST","test","","TEST"}
bool GUtility::SplitString(const std::string& source_str, char splite_mark, std::vector<std::string>& splited_strings)
{
    if (source_str.length() == 0)
    {
        return true;
    }
    
    int  str_start_pos = 0;
    char temp_str[128];
    int  temp_str_char_index = 0;
    int  temp_str_size = sizeof(temp_str);
    memset(temp_str, 0, temp_str_size);
    for (int i=0; i<source_str.size(); i++)
    {
        if (source_str[i] == splite_mark)
        {
            int  temp_str_length  = i - str_start_pos;
            if ( temp_str_size < temp_str_length+1 )
            {
                CCLOG("[GUtility] Error:(SplitString)temp_str size(%d) is less than temp_str length(%d+1)",
                      temp_str_size,
                      temp_str_length);
                return false;
            }
            splited_strings.push_back(temp_str);
            memset(temp_str, 0, temp_str_size);
            temp_str_char_index = 0;
            str_start_pos = i+1;
        }
        else
        {
            temp_str[temp_str_char_index] = source_str[i];
            temp_str_char_index++;
        }
    }
    splited_strings.push_back(temp_str);
    
    return true;
}

bool GUtility::Utf8StrToStrVector(const std::string & src , std::vector<std::string>& des)
{
    string temp;
    uint8_t tempChar = 0;
    for (int i=0; i<src.size(); i++)
    {
        temp.clear();
        tempChar = src.at(i);
        //tempChar &= 0xff;
        if (tempChar<=0x7F) // 1 byte
        {
            temp.push_back(src.at(i));
        }
        else if (tempChar<=0xDF) // 2 byte
        {
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
        }
        else if (tempChar<=0xEF) // 3 byte
        {
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
        }
        else if (tempChar<=0xF7) // 4 byte
        {
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
        }
        else if (tempChar<=0xFB) // 5 byte
        {
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
        }
        else if (tempChar<=0xFD) // 6 byte
        {
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
            i++;
            temp.push_back(src.at(i));
        }
        else
        {
            return false;
        }
        des.push_back(temp);
    }
    return true;
}

bool GUtility::ChangeFileSuffix(std::string& fileName,const std::string& newSuffix)
{
    size_t dotPos = fileName.find_last_of(".");
    if (dotPos != std::string::npos)
    {
        dotPos++;
        fileName.assign(fileName.begin(), fileName.begin()+dotPos);
        fileName += newSuffix;
        return true;
    }
    return false;
}

void  GUtility::PrintVectorString(const std::vector<std::string>& string_vector)
{
#ifdef DEBUG
    typedef std::vector<std::string>::const_iterator ConstStringVectorIter;
    ConstStringVectorIter begin = string_vector.begin();
    for (; begin!=string_vector.end(); begin++)
    {
        CCLOG("%s\n",(*begin).c_str());
    }
#endif
}

//转为竖向字体
std::string GUtility::toVertical(const std::string horizontal)
{
    string verticalstr = "";
    verticalstr.clear();
    
    vector<string> tempGeneralNameVect;
    GUtility::Utf8StrToStrVector(horizontal, tempGeneralNameVect);
    int nameCharCount = tempGeneralNameVect.size();
    if (nameCharCount>1)
    {
        for (int i=0; i<nameCharCount; i++)
        {
            verticalstr += tempGeneralNameVect.at(i);
            verticalstr += "\n";
        }
        verticalstr.pop_back();
    }
    return verticalstr;
}







