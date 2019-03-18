#include <iostream>
#include "wsseapi.h"
#include "soapH.h"
using namespace std;

#define ONVIF_USER "admin"
#define ONVIF_PASSWORD "dilu1234"
//#define ONVIF_PASSWORD "abcd1234"

int probe()
{
    struct soap *soap = soap_new();
    soap_set_namespaces(soap, namespaces);    

    struct wsdd__ProbeType req;
    struct __wsdd__ProbeMatches resp;
    struct wsdd__ScopesType sScope;
    struct SOAP_ENV__Header header;
    int count = 0;
    int result = 0; 

    char guid_string[100];
    soap->recv_timeout = 5;     //超过5秒钟没有数据就退出
    soap_default_SOAP_ENV__Header(soap, &header);
 
    header.wsa__MessageID = guid_string;
    header.wsa__To= "urn:schemas-xmlsoap-org:ws:2005:04:discovery";
    header.wsa__Action= "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
    soap->header = &header;

    soap_default_wsdd__ScopesType(soap, &sScope);
    sScope.__item = "";
    soap_default_wsdd__ProbeType(soap, &req);
    req.Scopes = &sScope;
    req.Types = "";


    result = soap_send___wsdd__Probe(soap, "soap.udp://239.255.255.250:3702", NULL, &req);

    do{
         result = soap_recv___wsdd__ProbeMatches(soap, &resp); 
         if (soap->error) 
         { 
             cout<<"soap error:"<<soap->error<<soap_faultcode(soap)<<"---"<<soap_faultstring(soap)<<endl; 
             result = soap->error; 
             break;
         } 
         else
         {
            cout<<"========================================="<<endl;
 
            cout<<"Match size:"<<resp.wsdd__ProbeMatches->__sizeProbeMatch<<endl;
            cout<<"xsd-unsignedInt:"<<resp.wsdd__ProbeMatches->ProbeMatch->MetadataVersion<<endl;
//            cout<<"scopes item:"<<resp.wsdd__ProbeMatches->ProbeMatch->Scopes->__item<<endl;
            //cout<<"scopes matchby:"<<resp.wsdd__ProbeMatches->ProbeMatch->Scopes->MatchBy<<endl;
            cout<<"QName:"<<resp.wsdd__ProbeMatches->ProbeMatch->Types<<endl;
            cout<<"xsd:string:"<<resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.Address<<endl;
            cout<<"xsd:QName:"<<resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.PortType<<endl;
            cout<<"wsa:ServiceNameType:"<<resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.ServiceName<<endl;
            cout<<"sequence of elements:"<<resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.__size<<endl;
            cout<<"xsd:anyType:"<<resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.__anyAttribute<<endl;
            cout<<"endpoint any:"<<resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.__any<<endl;
            cout<<"wsdd:UriListType:"<<resp.wsdd__ProbeMatches->ProbeMatch->XAddrs<<endl;
         }
     }while(1);

    soap_end(soap);
    soap_done(soap);
    soap_free(soap);

    return 0;
}

int getcapblities(char *url)
{
    struct soap *soap = soap_new();
    soap_set_namespaces(soap, namespaces);    

    struct SOAP_ENV__Header header;
    soap_default_SOAP_ENV__Header(soap, &header);

    struct _tds__GetCapabilities capa_req;
    struct _tds__GetCapabilitiesResponse capa_resp;

    capa_req.Category = (enum tt__CapabilityCategory *)soap_malloc(soap, sizeof(int));
    capa_req.__sizeCategory = 1;
    *(capa_req.Category) = (enum tt__CapabilityCategory)(tt__CapabilityCategory__Media);

    int result = soap_call___tds__GetCapabilities(soap, url, NULL, &capa_req, &capa_resp);
    if (SOAP_OK != soap->error)
    {
        printf("[%d]--->>> soap error: %d, %s, %s\n", __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
        int retval = soap->error;
        return -1;
    }else{
        if(capa_resp.Capabilities==NULL)
        {
            printf(" GetCapabilities  failed!  result=%d \n",result);
        }
        else
        {
            cout<<"wsdd:UriListType:"<< capa_resp.Capabilities->Media->XAddr <<endl;

            //            return(UserGetProfiles(&getProfilesResponse,capa_resp));
        }
    }

    soap_destroy(soap);
    soap_end(soap);
    soap_free(soap);

    return 0;
}

int getprofiles(char *url, int timeout)
{
    struct soap *soap = soap_new();
    soap_set_namespaces(soap, namespaces);    

    struct SOAP_ENV__Header header;
    soap_default_SOAP_ENV__Header(soap, &header);

    soap_wsse_add_UsernameTokenDigest(soap,NULL, ONVIF_USER, ONVIF_PASSWORD); 

    struct _trt__GetProfiles getProfiles;
    struct _trt__GetProfilesResponse getProfilesResponse;

    int result = soap_call___trt__GetProfiles(soap, url, NULL, &getProfiles, &getProfilesResponse);
    if ( SOAP_OK != result || SOAP_OK != soap->error)
    {
        printf("soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
        result = soap->error;
        return -1;
    }else{
        if (getProfilesResponse.__sizeProfiles > 0) 
        {           
            for( int i = 0; i < getProfilesResponse.__sizeProfiles; i++) {                                   // 提取所有配置文件信息（我们所关心的）
                struct tt__Profile *ttProfile = &getProfilesResponse.Profiles[i];
                if(ttProfile->Name!=NULL){
                    printf("Profiles Name:%s  \n",ttProfile->Name);
                }
                if(ttProfile->token!=NULL){
                    printf("Profiles Taken:%s\n",ttProfile->token);
                }
            }
        }
    }


    soap_destroy(soap);
    soap_end(soap);
    soap_free(soap);

    return 0;
}

int getstreamuri(char *url, char* token)
{
    struct soap *soap = soap_new();
    soap_set_namespaces(soap, namespaces);    

    struct SOAP_ENV__Header header;
    soap_default_SOAP_ENV__Header(soap, &header);

    struct _trt__GetStreamUriResponse getStreamUriResponse;
    struct _trt__GetStreamUri getStreamUri;

    soap->socket = 3;
    getStreamUri.StreamSetup = (struct tt__StreamSetup*)soap_malloc(soap, sizeof(struct tt__StreamSetup));//初始化，分配空间 
    getStreamUri.StreamSetup->Stream = tt__StreamType__RTP_Unicast;//0;//stream type 
    getStreamUri.StreamSetup->Transport = (struct tt__Transport *)soap_malloc(soap, sizeof(struct tt__Transport));//初始化，分配空间 
    getStreamUri.StreamSetup->Transport->Protocol = tt__TransportProtocol__UDP;//0; 
    getStreamUri.StreamSetup->Transport->Tunnel = NULL;
    getStreamUri.StreamSetup->__size = 1;
    getStreamUri.StreamSetup->__any = NULL;
    getStreamUri.StreamSetup->__anyAttribute =NULL;
    getStreamUri.ProfileToken = (char*)soap_malloc(soap,1024);
    strcpy(getStreamUri.ProfileToken, token);//trt__GetProfilesResponse->Profiles->token);

    // getStreamUri.ProfileToken = trt__GetProfilesResponse->Profiles->token ; 
    soap_wsse_add_UsernameTokenDigest(soap,"user", ONVIF_USER, ONVIF_PASSWORD);
    soap_call___trt__GetStreamUri(soap, url, NULL, &getStreamUri, &getStreamUriResponse);
    if (soap->error) {
        printf("soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
        return soap->error;
    }
    else
    {
       printf("==== [ Media Stream Uri Response ] ====\n > MediaUri :\n\t%s\n", getStreamUriResponse.MediaUri->Uri);        
    }

    soap_destroy(soap);
    soap_end(soap);
    soap_free(soap);

    return 0;
}

int make_uri_withauth(char *src_uri, char *username, char *password, char *dest_uri, unsigned int size_dest_uri)
{
    int result = 0;
    unsigned int needBufSize = 0;

    memset(dest_uri, 0x00, size_dest_uri);

    needBufSize = strlen(src_uri) + strlen(username) + strlen(password) + 3;    // 检查缓存是否足够，包括‘:’和‘@’和字符串结束符
    if (size_dest_uri < needBufSize) {
        printf("dest uri buf size is not enough.\n");
        result = -1;
        goto EXIT;
    }

    if (0 == strlen(username) && 0 == strlen(password)) {                       // 生成新的uri地址
        strcpy(dest_uri, src_uri);
    } else {
        char *p = strstr(src_uri, "//");
        if (NULL == p) {
            printf("can't found '//', src uri is: %s.\n", src_uri);
            result = -1;
            goto EXIT;
        }
        p += 2;

        memcpy(dest_uri, src_uri, p - src_uri);
        sprintf(dest_uri + strlen(dest_uri), "%s:%s@", username, password);
        strcat(dest_uri, p);
    }

EXIT:

    return result;
}

int main(int argc, char **argv)    
{    
/*
    struct soap add_soap;    

    soap_init(&add_soap);
    soap_set_namespaces(&add_soap, namespaces);    
    char * server_addr = "http://0.0.0.0:80";

    wsdd__HelloType type;
    int ret = soap_send___wsdd__Hello(&add_soap, server_addr, "", &type);
    if ( ret == SOAP_ERR)
    {
        printf("Error while calling the soap_send___wsdd__Hello");
    }
    else
    {
        printf("Calling the soap_send___wsdd__Hello success。\n");
    }
    soap_end(&add_soap);
    soap_done(&add_soap);
*/

//    printf("probe========================== \n");
//    probe();

    printf("getcapblities========================== \n");
//    char *url = "http://192.168.99.1:10002/onvif/device_service";
    char *url = "http://10.0.1.251/onvif/device_service";
    getcapblities(url);

    printf("getprofiles=============================== \n");
//    char *profileurl = "http://192.168.99.1:10002/onvif/media_service";
    char *profileurl = "http://10.0.1.251/onvif/media_service";
    getprofiles(profileurl, 3);

    printf("getstreamuri=============================== \n");
//    char *streamuri = "http://192.168.99.1:10002/onvif/media_service";
    char *streamuri = "http://10.0.1.251/onvif/media_service";
    getstreamuri(streamuri, "Profile_1");

    return 0;
}   
