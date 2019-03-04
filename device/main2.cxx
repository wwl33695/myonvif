
#include "wsdd.h"
#include <stdio.h>
#include <ctype.h>
#include <openssl/sha.h>
 
#include "cameraonvif.h"
 
/* 设备信息 */
typedef struct USER_INFO
{
    char username[16];
    char password[32];
    char IpAddr[16];
} UserInfo_S;
 
/* 设备能力地址 */
typedef struct CAPABILITIES_ADDR
{
    char *Device;
    char *Imaging;
    char *Media;
    char *PTZ;
} CapabilitiesAddr_S;
 
/* 设备参数标识 */
typedef struct PROFILE_TOKEN
{
    char *Media;
    char *PTZConfiguration;
} ProfileToken_S;
 
/* 设备Uri信息 */
typedef struct URI
{
    char *StreamUri;
    char *SnapshotUri;
} Uri_S;
 
/** 初始化信息集合 **/
typedef struct ONVIF_COMPLETE_INFO
{
    UserInfo_S *UserInfo;
    CapabilitiesAddr_S *CapabilitiesAddr;
    ProfileToken_S *ProfileToken;
    Uri_S *Uri;
} OnvifCompleteInfo_S;
OnvifCompleteInfo_S stOnvifCompleteInfo;
 
static const char base64digits[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 
#define BAD     -1
static const signed char base64val[] = {
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD, 62, BAD,BAD,BAD, 63,
    52, 53, 54, 55,  56, 57, 58, 59,  60, 61,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14,
    15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25,BAD, BAD,BAD,BAD,BAD,
    BAD, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40,
    41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51,BAD, BAD,BAD,BAD,BAD
};
#define DECODE64(c)  (isascii(c) ? base64val[c] : BAD)
 
/**
 * @brief base64加密函数
 * @param out
 * @param in
 * @param inlen
 */
void base64_bits_to_64(unsigned char *out, const unsigned char *in, int inlen)
{
    for (; inlen >= 3; inlen -= 3)
    {
        *out++ = base64digits[in[0] >> 2];
        *out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];
        *out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
        *out++ = base64digits[in[2] & 0x3f];
        in += 3;
    }
 
    if (inlen > 0)
    {
        unsigned char fragment;
 
        *out++ = base64digits[in[0] >> 2];
        fragment = (in[0] << 4) & 0x30;
 
        if (inlen > 1)
            fragment |= in[1] >> 4;
 
        *out++ = base64digits[fragment];
        *out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
        *out++ = '=';
    }
 
    *out = '\0';
}
 
/**
 * @brief base64解密函数
 * @param out
 * @param in
 * @return
 */
int base64_64_to_bits(char *out, const char *in)
{
    int len = 0;
    register unsigned char digit1, digit2, digit3, digit4;
 
    if (in[0] == '+' && in[1] == ' ')
        in += 2;
    if (*in == '\r')
        return(0);
 
    do {
        digit1 = in[0];
        if (DECODE64(digit1) == BAD)
            return(-1);
        digit2 = in[1];
        if (DECODE64(digit2) == BAD)
            return(-1);
        digit3 = in[2];
        if (digit3 != '=' && DECODE64(digit3) == BAD)
            return(-1);
        digit4 = in[3];
        if (digit4 != '=' && DECODE64(digit4) == BAD)
            return(-1);
        in += 4;
        *out++ = (DECODE64(digit1) << 2) | (DECODE64(digit2) >> 4);
        ++len;
        if (digit3 != '=')
        {
            *out++ = ((DECODE64(digit2) << 4) & 0xf0) | (DECODE64(digit3) >> 2);
            ++len;
            if (digit4 != '=')
            {
                *out++ = ((DECODE64(digit3) << 6) & 0xc0) | DECODE64(digit4);
                ++len;
            }
        }
    } while (*in && *in != '\r' && digit4 != '=');
 
    return (len);
}
 
/**
 * @brief ONVIF鉴权生成
 * @param pwddigest_out
 * @param pwd
 * @param nonc
 * @param time
 */
static void ONVIF_GenrateDigest(unsigned char *pwddigest_out, unsigned char *pwd, char *nonc, char *time)
{
    const unsigned char *tdist;
    unsigned char dist[1024] = {0};
    char tmp[1024] = {0};
    unsigned char bout[1024] = {0};
    strcpy(tmp,nonc);
    base64_64_to_bits((char*)bout, tmp);
    sprintf(tmp,"%s%s%s",bout,time,pwd);
    SHA1((const unsigned char*)tmp,strlen((const char*)tmp),dist);
    tdist = dist;
    memset(bout,0x0,1024);
    base64_bits_to_64(bout,tdist,(int)strlen((const char*)tdist));
    strcpy((char *)pwddigest_out,(const char*)bout);
}
 
/**
 * @brief ONVIF初始化一个soap对象
 * @param header            SOAP_ENV__HEADER
 * @param was_To            *wsa_To
 * @param was_Action        *wsa_Action
 * @param timeout           超时设置
 * @return soap结构体地址
 */
static struct soap* ONVIF_Initsoap(struct SOAP_ENV__Header *header, const char *was_To, const char *was_Action, int timeout, int toAuthenticate)
{
    //printf("> %s\n> %s\n> %s\n", pUserInfo->username, pUserInfo->password, pUserInfo->IpAddr);
    struct soap *soap = NULL;
    unsigned char macaddr[6];
    char _HwId[1024];
    unsigned int Flagrand;
    soap = soap_new();
    if(soap == NULL)
    {
        printf("[%d]soap = NULL\n", __LINE__);
        return NULL;
    }
     soap_set_namespaces( soap, namespaces);
    //超过5秒钟没有数据就退出
    if (timeout > 0)
    {
        soap->recv_timeout = timeout;
        soap->send_timeout = timeout;
        soap->connect_timeout = timeout;
    }
    else
    {
        //如果外部接口没有设备默认超时时间的话，我这里给了一个默认值10s
        soap->recv_timeout    = 10;
        soap->send_timeout    = 10;
        soap->connect_timeout = 10;
    }
    soap_default_SOAP_ENV__Header(soap, header);
 
    //为了保证每次搜索的时候MessageID都是不相同的！因为简单，直接取了随机值
    srand((int)time(0));
    Flagrand = rand()%9000 + 1000; //保证四位整数
    macaddr[0] = 0x1; macaddr[1] = 0x2; macaddr[2] = 0x3; macaddr[3] = 0x4; macaddr[4] = 0x5; macaddr[5] = 0x6;
    sprintf(_HwId,"urn:uuid:%ud68a-1dd2-11b2-a105-%02X%02X%02X%02X%02X%02X",
            Flagrand, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    header->wsa__MessageID =(char *)malloc( 100);
    memset(header->wsa__MessageID, 0, 100);
    strncpy(header->wsa__MessageID, _HwId, strlen(_HwId));
 
    //这里开始作鉴权处理了，如果有用户信息的话，就会处理鉴权问题
    //如果设备端不需要鉴权的话，在外层调用此接口的时候把 toAuthenticate 置0
    if( toAuthenticate )
    {
        header->wsse__Security = (struct _wsse__Security *)malloc(sizeof(struct _wsse__Security));
        memset(header->wsse__Security, 0 , sizeof(struct _wsse__Security));
 
        header->wsse__Security->UsernameToken = (struct _wsse__UsernameToken *)calloc(1,sizeof(struct _wsse__UsernameToken));
        header->wsse__Security->UsernameToken->Username = (char *)malloc(64);
        memset(header->wsse__Security->UsernameToken->Username, '\0', 64);
 
        header->wsse__Security->UsernameToken->Nonce = (char*)malloc(64);
        memset(header->wsse__Security->UsernameToken->Nonce, '\0', 64);
        strcpy(header->wsse__Security->UsernameToken->Nonce,"LKqI6G/AikKCQrN0zqZFlg=="); //注意这里
 
        header->wsse__Security->UsernameToken->wsu__Created = (char*)malloc(64);
        memset(header->wsse__Security->UsernameToken->wsu__Created, '\0', 64);
        strcpy(header->wsse__Security->UsernameToken->wsu__Created,"2010-09-16T07:50:45Z");
 
        strcpy(header->wsse__Security->UsernameToken->Username, stOnvifCompleteInfo.UserInfo->username /*pUserInfo->username*/);
        header->wsse__Security->UsernameToken->Password = (struct _wsse__Password *)malloc(sizeof(struct _wsse__Password));
        header->wsse__Security->UsernameToken->Password->Type = (char*)malloc(128);
        memset(header->wsse__Security->UsernameToken->Password->Type, '\0', 128);
        strcpy(header->wsse__Security->UsernameToken->Password->Type,\
                "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest");
        header->wsse__Security->UsernameToken->Password->__item = (char*)malloc(128);
        ONVIF_GenrateDigest((unsigned char*)header->wsse__Security->UsernameToken->Password->__item, \
            (unsigned char*)stOnvifCompleteInfo.UserInfo->password/*pUserInfo->password*/, header->wsse__Security->UsernameToken->Nonce, header->wsse__Security->UsernameToken->wsu__Created);
 
    }
    if (was_Action != NULL)
    {
        header->wsa__Action =(char *)malloc(1024);
        memset(header->wsa__Action, '\0', 1024);
        strncpy(header->wsa__Action, was_Action, 1024);//"http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
    }
    if (was_To != NULL)
    {
        header->wsa__To =(char *)malloc(1024);
        memset(header->wsa__To, '\0', 1024);
        strncpy(header->wsa__To,  was_To, 1024);//"urn:schemas-xmlsoap-org:ws:2005:04:discovery";
    }
    soap->header = header;
    return soap;
}
 
/**
 * @brief ONVIF获取视频流地址
 * @return 成功 0 / 失败 soap_error错误代码
 */
int ONVIF_GetRTSPStream()
{
    int retval = 0;
    struct soap *soap = NULL;
    char *soap_endpoint = (char *)malloc(256);
    const char *soap_action = NULL;
 
    struct SOAP_ENV__Header header;
 
    struct _trt__GetProfiles media_GetProfiles;
    struct _trt__GetProfilesResponse media_GetProfilesResponse;
    struct _trt__GetStreamUri media_GetStreamUri;
    struct _trt__GetStreamUriResponse media_GetStreamUriResponse;
 
    /* 1 GetProfiles */
    soap = ONVIF_Initsoap(&header, NULL, NULL, 5, 1);
    memset(soap_endpoint, '\0', 256);
    sprintf(soap_endpoint, "http://%s/onvif/Media", stOnvifCompleteInfo.UserInfo->IpAddr);
 
    do
    {
        soap_call___trt__GetProfiles(soap, soap_endpoint, soap_action, &media_GetProfiles, &media_GetProfilesResponse);
        if (soap->error)
        {
            printf("[%s][%d]--->>> soap error: %d, %s, %s\n", __func__, __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
            retval = soap->error;
            break;
        }
        else
        {
            printf("==== [ Media Profiles Response ] ====\n"
                   "> Name  :  %s\n"
                   "> token :  %s\n\n", \
                   media_GetProfilesResponse.Profiles->Name, \
                   media_GetProfilesResponse.Profiles->token );
        }
    }while(0);
 
    /* 2 GetStreamUri */
    soap = ONVIF_Initsoap(&header, NULL, NULL, 5, 1);
    memset(soap_endpoint, '\0', 256);
    sprintf(soap_endpoint, "http://%s/onvif/Media", stOnvifCompleteInfo.UserInfo->IpAddr);
 
    media_GetStreamUri.StreamSetup = (struct tt__StreamSetup *)soap_malloc(soap, sizeof(struct tt__StreamSetup));
    media_GetStreamUri.StreamSetup->Transport = (struct tt__Transport *)soap_malloc(soap, sizeof(struct tt__Transport));
 
    media_GetStreamUri.StreamSetup->Stream = 0;
    media_GetStreamUri.StreamSetup->Transport->Protocol = 0;
    media_GetStreamUri.StreamSetup->Transport->Tunnel = 0;
    media_GetStreamUri.StreamSetup->__size = 1;
    media_GetStreamUri.StreamSetup->__any = NULL;
    media_GetStreamUri.StreamSetup->__anyAttribute = NULL;
    media_GetStreamUri.ProfileToken = media_GetProfilesResponse.Profiles->token;
 
    do
    {
        soap_call___trt__GetStreamUri(soap, soap_endpoint, soap_action, &media_GetStreamUri, &media_GetStreamUriResponse);
        if (soap->error)
        {
            printf("[%s][%d]--->>> soap error: %d, %s, %s\n", __func__, __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
            retval = soap->error;
            break;
        }
        else
        {
            printf("==== [ Media Stream Uri Response ] ====\n"
                   "> MediaUri :\n\t%s\n", \
                   media_GetStreamUriResponse.MediaUri->Uri);
            printf("[\033[1;32m success\033[0m ] Get Stream Uri!\n");
#if 0   // 此处可使用ffplay播放视频流, 请将stUserInfo结构体内容替换成相应的信息
            char *rtspStreamUri = (char*)malloc(sizeof(char));
            strncpy(rtspStreamUri, media_GetStreamUriResponse.MediaUri->Uri, 7);
            strcat(rtspStreamUri, stUserInfo->username);
            strcat(rtspStreamUri, ":");
            strcat(rtspStreamUri, stUserInfo->password);
            strcat(rtspStreamUri, "@");
            strcat(rtspStreamUri, media_GetStreamUriResponse.MediaUri->Uri + 7);
            printf("> Completing Uri :\n\t%s\n\n", rtspStreamUri);
 
            char buf[1024];
            sprintf(buf, "ffplay %s", rtspStreamUri);
            system(buf);
#endif
        }
    }while(0);
 
    return retval;
}
 
/**
 * @brief 主函数
 * @return
 */
int main()
{
    // 这个结构体是我为了方便使用而定义的.
    // 一些无关的内容就不赋值了.
    /* 结构体初始化 */
    stOnvifCompleteInfo.UserInfo = (UserInfo_S *)malloc(sizeof(UserInfo_S));
    stOnvifCompleteInfo.CapabilitiesAddr = (CapabilitiesAddr_S *)malloc(sizeof(CapabilitiesAddr_S));
    stOnvifCompleteInfo.CapabilitiesAddr->Device = (char *)malloc(sizeof(char));
    stOnvifCompleteInfo.CapabilitiesAddr->Imaging = (char *)malloc(sizeof(char));
    stOnvifCompleteInfo.CapabilitiesAddr->Media = (char *)malloc(sizeof(char));
    stOnvifCompleteInfo.CapabilitiesAddr->PTZ = (char *)malloc(sizeof(char));
    stOnvifCompleteInfo.ProfileToken = (ProfileToken_S *)malloc(sizeof(ProfileToken_S));
    stOnvifCompleteInfo.ProfileToken->Media = (char *)malloc(sizeof(char));
    stOnvifCompleteInfo.ProfileToken->PTZConfiguration = (char *)malloc(sizeof(char));
    stOnvifCompleteInfo.Uri = (Uri_S *)malloc(sizeof(Uri_S));
    stOnvifCompleteInfo.Uri->StreamUri = (char *)malloc(sizeof(char));
    stOnvifCompleteInfo.Uri->SnapshotUri = (char *)malloc(sizeof(char));
 
    stOnvifCompleteInfo.UserInfo->username = "admin";
    stOnvifCompleteInfo.UserInfo->password = "admin";
    stOnvifCompleteInfo.UserInfo->IpAddr = "192.168.1.123";
    stOnvifCompleteInfo.CapabilitiesAddr->Device = "";
    stOnvifCompleteInfo.CapabilitiesAddr->Imaging = "";
    stOnvifCompleteInfo.CapabilitiesAddr->Media = "";
    stOnvifCompleteInfo.CapabilitiesAddr->PTZ = "";
    stOnvifCompleteInfo.ProfileToken->Media = "";
    stOnvifCompleteInfo.ProfileToken->PTZConfiguration = "";
    stOnvifCompleteInfo.Uri->StreamUri = "";
    stOnvifCompleteInfo.Uri->SnapshotUri = "";
 
    if (ONVIF_GetRTSPStream() != 0)
    {
        printf("[%s][%d]--->>> get RTSP stream failed!\n", __func__, __LINE__);
        return -1;
    }
    return 0;
}
