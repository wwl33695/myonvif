int main(int argc, char **argv)    
{    
    int m, s;    
    struct soap add_soap;    
    int server_udp;  
  
	server_udp = create_server_socket_udp();  
	//bind_server_udp1(server_udp);  
	pthread_t thrHello;  
	pthread_t thrProbe;  
	//pthread_create(&thrHello,NULL,main_Hello,server_udp);  
	//sleep(2);  
	pthread_create(&thrProbe,NULL,main_Probe,server_udp);  

	soap_init(&add_soap);    
	soap_set_namespaces(&add_soap, namespaces);    
   
   
     if (argc < 0) {    
         printf("usage: %s <server_port> \n", argv[0]);    
         exit(1);    
     } else {    
         m = soap_bind(&add_soap, NULL, 80, 100);    
         if (m < 0) {    
             soap_print_fault(&add_soap, stderr);    
             exit(-1);    
         }    
         fprintf(stderr, "Socket connection successful: master socket = %d\n", m);    
         for (;;) {    
             s = soap_accept(&add_soap);    
             if (s < 0) {    
                 soap_print_fault(&add_soap, stderr);    
                 exit(-1);    
             }    
             fprintf(stderr, "Socket connection successful: slave socket = %d\n", s);    
             soap_serve(&add_soap);    
             soap_end(&add_soap);    
         }    
     }    
     return 0;    
}   