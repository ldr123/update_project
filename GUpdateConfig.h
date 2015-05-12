//
//  GUpdateImpl.h
//  dz3gz
//
//  Created by ldr123 on 11/04/13.
//
//

#ifndef __dz3gz__GUpdateConfig____
#define __dz3gz__GUpdateConfig____

#define LOCAL_CLIENT_RES_VERSION "local_client_res_version"

//客户端包的版本号
#define UPDATE_CLIENT_PACKAGE_VERSION_TXT "1.0.0"
#define UPDATE_CLIENT_PACKAGE_VERSION 1

/*
1.跳转到app页面
找到应用程序的描述链接，比如：http://itunes.apple.com/gb/app/yin/id391945719?mt=8
然后将 http:// 替换为 itms:// 或者 itms-apps://：
itms://itunes.apple.com/gb/app/yi/id391945719?mt=8
itms-apps:// itunes.apple.com/gb/app/yi/id391945719?mt=8
然后打开这个链接地址：
[
    [UIApplication sharedApplication]
    openURL:[NSURL URLWithString:@"itms://itunes.apple.com/gb/app/yi/id391945719?mt=8"]
];

[
[UIApplication sharedApplication]
    openURL:[NSURL URLWithString:@"itms-apps://itunes.apple.com/gb/app/yi/id391945719?mt=8"]
];
*/

/*
返回值描述
客户端程序版本小于服务器程序版本，需要去appstore更新
类型 1

资源版本为0，第一次进入游戏时的版本
类型 2 SERVER_MAJOR_VERSION SERVER_MINOR_VERSION SERVER_RESOURCE_VERSION SERVER_APP_MD5 SERVER_DOWNLOAD_URL

资源版本大于0,但是小于服务器资源版本
类型 3 SERVER_RESOURCE_VERSION MD5(PACKFILE) PACKFILE_URL

资源版本大于0,并且跟服务器资源版本匹配
类型 4

错误 404
*/

#endif
