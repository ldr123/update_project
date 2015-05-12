//
//  GUtility.h
//
//  Created by ldr123 on 13-3-28.
//  Copyright (c) 2013年 __MyCompanyName__. All rights reserved.
//

#ifndef LuaToCPlusTest_GUtility_cpp
#define LuaToCPlusTest_GUtility_cpp

#include "cocos2d.h"
#include <algorithm>
#include <unordered_map>

/**
 @brief:    GSingletonMacro
 */
// #pragma mark -
// #pragma mark GSingletonMacro
#define SINGLETON(__TYPE__) \
private: \
    __TYPE__(){} \
    static __TYPE__** GetInstance() \
    { \
        static __TYPE__* instance_ = nullptr; \
        if(instance_ == nullptr) \
        { \
        instance_ = new __TYPE__(); \
        } \
        return (&instance_);\
    } \
public: \
    static __TYPE__* ShareInstance() \
    { \
        return (*__TYPE__::GetInstance()); \
    } \
    static void ReleaseInstance() \
    { \
        __TYPE__** temp = GetInstance(); \
        if(*temp) \
        { \
            delete (*temp); \
            *temp = nullptr; \
        } \
    } \
public: \
;

#define SHARE_INSTANCE(__TYPE__) \
static __TYPE__* ShareInstance() \
{ \
    static __TYPE__* instance = nullptr; \
    if(instance == nullptr) \
    { \
        instance = new __TYPE__(); \
    } \
    return instance; \
} \
; \

#define PROPERTY_ASSIGN(_TYPE_,_VAR_,_FUNCNAME_,_DEFAULTVALUE_) \
public: \
    _TYPE_ Get##_FUNCNAME_()const { return _VAR_; } \
    void Set##_FUNCNAME_(_TYPE_ value) { _VAR_ = value; } \
protected: \
    _TYPE_ _VAR_ = _DEFAULTVALUE_; \
public: \
;

/**
 @brief:    Debug Log Macro
 */
// #pragma mark -
// #pragma mark Debug Log Macro
#define DEBUG_GAMELOGIC
#ifdef DEBUG_GAMELOGIC
#define GLOG(format, ...)      cocos2d::CCLog(format, ##__VA_ARGS__)
#define GCONDITIONLOG(condition, format, ...)    if(condition){cocos2d::CCLog(format, ##__VA_ARGS__);}
#endif

#define DEBUG_GUI
#ifdef DEBUG_GUI
#define GUILOG(format, ...)      cocos2d::CCLog(format, ##__VA_ARGS__)
#define GUICONDITIONLOG(condition, format, ...)    if(condition){cocos2d::CCLog(format, ##__VA_ARGS__);}
#endif

#define DEBUG_NETWORK
#ifdef DEBUG_NETWORK
#define GNETWORKLOG(format, ...)      cocos2d::CCLog(format, ##__VA_ARGS__)
#define GNETWORKCONDITIONLOG(condition, format, ...)    if(condition){cocos2d::CCLog(format, ##__VA_ARGS__);}
#endif

#define GDEBUG
#ifdef GDEBUG
#define GASSERT(myassert, format, ...)    if(!myassert){cocos2d::CCLog(format, ##__VA_ARGS__);assert(false);}
#endif

struct GStdStringCompare : public std::binary_function<std::string, std::string, bool>
{
public:
    bool operator() (const std::string& a, const std::string& b) const
    {
        return strcmp(a.c_str(), b.c_str()) < 0;
    }
};

/**
 @brief:    判断node是否显示 和 IsNodeNeedShow是同样的功能
 */
extern bool IsNodeShowing(cocos2d::CCNode* node);

/**
 @brief: recursive check node parent visible property,if parent is disvisible then return false,else return true
 */
extern bool IsNodeNeedShow(cocos2d::CCNode* node);

/**
 @brief: 判断用户的操作是否为点击操作
 @param endPoint: 用户操作结束时的坐标点
 @comment: 用户操作起始时的坐标点为 m_beginTouchPoint
 */
extern bool IsUserOperationIsClick(cocos2d::CCPoint& beginPoint, cocos2d::CCPoint& endPoint);

/**
 @brief: 获得node的scale值
 */
extern cocos2d::CCSize GetNodeScaleValue(cocos2d::CCNode* node);

/**
 * Node AnchorPos Position Convert
 */
const cocos2d::CCPoint kGAnchor_LeftTop(0.0, 1.0);
const cocos2d::CCPoint kGAnchor_LeftMid(0.0, 0.5);
const cocos2d::CCPoint kGAnchor_LeftBottom(0.0, 0.0);
const cocos2d::CCPoint kGAnchor_RightTop(1.0, 1.0);
const cocos2d::CCPoint kGAnchor_RightMid(1.0, 0.5);
const cocos2d::CCPoint kGAnchor_RightBottom(1.0, 0.0);
const cocos2d::CCPoint kGAnchor_TopMid(0.5, 1.0);
const cocos2d::CCPoint kGAnchor_Mid(0.5, 0.5);
const cocos2d::CCPoint kGAnchor_BottomMid(0.5, 0.0);

enum GENormalAnchorPosType
{
    kGAnchorType_Invalid = -1,
    kGAnchorType_LeftTop,
    kGAnchorType_LeftMid,
    kGAnchorType_LeftBottom,
    kGAnchorType_RightTop,
    kGAnchorType_RightMid,
    kGAnchorType_RightBottom,
    kGAnchorType_TopMid,
    kGAnchorType_Mid,
    kGAnchorType_BottomMid,
};
extern GENormalAnchorPosType ConvertARToARType(const cocos2d::CCPoint& anchor);
extern cocos2d::CCPoint GetAchorWith(GENormalAnchorPosType achorPosType);
extern cocos2d::CCPoint ConvertNodeRealARPosToGivenARPos(cocos2d::CCNode* node,
                                                         GENormalAnchorPosType givenARPosType,
                                                         bool isUseBoundingBox=true);
extern cocos2d::CCPoint ConvertNodeGivenARPosToRealARPos(cocos2d::CCNode* node,
                                                         const cocos2d::CCPoint& givenARPos,
                                                         GENormalAnchorPosType givenARPosType,
                                                         bool isUseBoundingBox=true);
extern cocos2d::CCPoint ConvertNodeGivenARPosAToB(cocos2d::CCNode* node,
                                                  const cocos2d::CCPoint& givenARPosA,
                                                  GENormalAnchorPosType givenARPosAType,
                                                  GENormalAnchorPosType givenARPosBType,
                                                  bool isUseBoundingBox=true);
extern cocos2d::CCPoint ConvertGivenARPosAToB(const cocos2d::CCSize& size,
                                              const cocos2d::CCPoint& givenARPosA,
                                              const cocos2d::CCPoint& givenARA,
                                              const cocos2d::CCPoint& givenARB);

/**
 @brief:    GUtility
 */
// #pragma mark -
// #pragma mark GUtility

class GUtility
{
public:
    //convert function
    static int   StringToInt(const char* str);
    static float StringToFloat(const char *str);
    static bool  StringToBool(const std::string& str);
    static std::string IntToString(int value);
    static std::string FloatToString(float value);
    static std::string LongLongToString(long long value);
    static std::string ListIntToString(const std::list<int>& listIntegers,char splite_mark);
    static bool  StringToListInt(const std::string& str, char splite_mark, std::list<int>&intergerList);
    static cocos2d::CCTextAlignment StringToTextAlignment(const std::string& str);
    static cocos2d::CCTextAlignment StringToTextAlignment(const char* str);
    static std::string CharVectorToString(const std::vector<char>* charVector);
    static std::string StringVectorToString(const std::vector<std::string>& input, const std::string& spliteStr);
    
    static char* Long2IP(unsigned long longIp);
    static unsigned long IP2Long(char *ip);
    
    static bool SplitString(const std::string& sourceStr, char splite_mark, std::vector<std::string>& splitedString);
    static bool Utf8StrToStrVector(const std::string & src , std::vector<std::string>& des);
    static bool ChangeFileSuffix(std::string& fileName,const std::string& newSuffix);
    //compare function
    static bool IsEqual(const char* str1,const char* str2);

    //将字符串的前后尾空格去掉
    static void TrimmedString(std::string & src);
    
    //debug function
    static void  PrintVectorString(const std::vector<std::string>& string_vector);

    static float GetFloatValueDecimalPart(float fValue)
    {
        return (fValue-(int)fValue);
    }
    //转为竖向字体
    static std::string toVertical(const std::string horizontal);
};

template <typename ElemType>
bool EraseFirstAppearValue(std::vector<ElemType>& values,ElemType value)
{
    typename std::vector<ElemType>::iterator beg = values.begin();
    for (; beg!=values.end(); beg++)
    {
        if (value == *beg)
        {
            values.erase(beg);
            return true;
        }
    }
    return false;
}

template <typename ElemType>
bool EraseValue(std::vector<ElemType>& values,ElemType value)
{
    typename std::vector<ElemType>::iterator tempIter = std::remove(values.begin(), values.end(), value);
    if (tempIter != values.end())
    {
        values.erase(tempIter,values.end());
        return true;
    }
    return false;
}

template <typename PointerType>
bool ReleasePointersInVector(std::vector<PointerType>& pointers)
{
    typename std::vector<PointerType>::iterator beg = pointers.begin();
    for (; beg != pointers.end(); beg++)
    {
        CC_SAFE_RELEASE((*beg));
    }
    pointers.clear();
    return true;
}

template <typename PointerType>
bool DeletePointersInVector(std::vector<PointerType>& pointers)
{
    typename std::vector<PointerType>::iterator beg = pointers.begin();
    for (; beg != pointers.end(); beg++)
    {
        delete *beg;
    }
    pointers.clear();
    return true;
}

template <typename KeyType,typename PointerValueType>
bool ReleasePointersInMap(std::unordered_map<KeyType, PointerValueType>& pointers)
{
    for (auto &beg : pointers)
	{
        CC_SAFE_RELEASE((beg.second));
	}
    pointers.clear();
    return true;
}

template <typename KeyType,typename PointerValueType>
bool DeletePointersInMap (std::unordered_map<KeyType, PointerValueType>& pointers)
{
	for (auto &beg : pointers)
	{
		delete beg.second;
	}

    pointers.clear();
    return true;
}

template <typename KeyType,typename PointerValueType>
bool DeletePointersInMultiMap (std::multimap<KeyType, PointerValueType>& pointers)
{
    typename std::multimap<KeyType, PointerValueType>::iterator beg = pointers.begin();
    for (; beg != pointers.end(); beg++)
    {
        delete beg->second;
    }
    pointers.clear();
    return true;
}

template<typename ParentType>
ParentType GetParent(cocos2d::CCNode* node)
{
    if (node)
    {
        cocos2d::CCNode* tempParent = node->getParent();
        ParentType parent = dynamic_cast<ParentType>(tempParent);
        if (parent)
        {
            return parent;
        }
        else
        {
            return GetParent<ParentType>(tempParent);
        }
        
    }
    return nullptr;
}

//notification
#define NOTIFY_APP_EXIT "notify_app_exit"
#endif
