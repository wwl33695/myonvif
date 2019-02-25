#include <iostream>
#include "plugin/wsseapi.h"
#include "soapH.h"
using namespace std;

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
            cout<<"scopes item:"<<resp.wsdd__ProbeMatches->ProbeMatch->Scopes->__item<<endl;
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
    struct _trt__GetProfilesResponse getProfilesResponse;
    struct _trt__GetStreamUriResponse getStreamUriResponse;
    capa_req.Category = (enum tt__CapabilityCategory *)soap_malloc(soap, sizeof(int));
    capa_req.__sizeCategory = 1;
    *(capa_req.Category) = (enum tt__CapabilityCategory)(tt__CapabilityCategory__Media);
    const char *soap_action = "http://www.onvif.org/ver10/device/wsdl/GetCapabilities";
    struct _tds__GetCapabilitiesResponse capa_resp;

    int result = soap_call___tds__GetCapabilities(soap, url, NULL, &capa_req, &capa_resp);
//int result = soap_call___tds__GetCapabilities(soap, "http://192.168.1.51/onvif/device_service", NULL, &capa_req, capa_resp);
    if (soap->error)
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

#define ONVIF_USER "aaaa"
#define ONVIF_PASSWORD "test"
int getprofiles(char *url)
{
    struct soap *soap = soap_new();
    soap_set_namespaces(soap, namespaces);    

    struct SOAP_ENV__Header header;
    soap_default_SOAP_ENV__Header(soap, &header);

    struct _trt__GetProfiles getProfiles;
    struct _trt__GetProfilesResponse getProfilesResponse;
    struct _trt__GetStreamUriResponse getStreamUriResponse;

    soap_wsse_add_UsernameTokenDigest(soap,NULL, ONVIF_USER, ONVIF_PASSWORD); 
    int result = soap_call___trt__GetProfiles(soap, url, NULL, &getProfiles, &getProfilesResponse);
    if (result==-1){
        printf("soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
        result = soap->error;
        return -1;
    }else{
        if(getProfilesResponse.Profiles!=NULL){
            if(getProfilesResponse.Profiles->Name!=NULL){
                printf("Profiles Name:%s  \n",getProfilesResponse.Profiles->Name);
            }
            if(getProfilesResponse.Profiles->token!=NULL){
                printf("Profiles Taken:%s\n",getProfilesResponse.Profiles->token);
            }
        //            return(UserGetUri(&getStreamUriResponse,getProfilesResponse,capa_resp));
        }else{
            printf("Profiles Get inner Error\n");
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
    //soap_wsse_add_UsernameTokenDigest(soap,"user", ONVIF_USER, ONVIF_PASSWORD); 
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

//    probe();

    char *url = "http://10.0.1.242/onvif/device_service";
//    getcapblities(url);

    char *profileurl = "http://10.0.1.242/onvif/media_service";
    getprofiles(profileurl);

    return 0;
}   
