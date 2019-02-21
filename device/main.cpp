#include "stdsoap2.h"
#include "soapStub.h"

int main(int argc, char **argv)    
{    
    struct soap add_soap;    

    //soap_init(&add_soap);
    //soap_set_namespaces(&add_soap, namespaces);    
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

    return 0;
}   
