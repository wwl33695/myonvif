#include "stdsoap2.h"

int main(int argc, char **argv)    
{    
    struct soap add_soap;    

    //soap_init(&add_soap);
    //soap_set_namespaces(&add_soap, namespaces);    
    char * server_addr = "http://10.64.57.10:10000";
/*
    int iRet = soap_call_ns__add(&add_soap, server_addr, "" , num1,num2,&result);
    if ( iRet == SOAP_ERR)
    {
        printf("Error while calling the soap_call_ns__add");
    }
    else
    {
        printf("Calling the soap_call_ns__add success。\n");
        printf("%d + %d = %d\n",num1,num2,result);
    }
*/
    soap_end(&add_soap);
    soap_done(&add_soap);

    return 0;
}   
