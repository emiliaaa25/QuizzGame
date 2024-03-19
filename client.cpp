#include<stdio.h>
#include<cstring>
#include<arpa/inet.h>
#include<iostream>
#include<unistd.h>
#include <SFML/Graphics.hpp>
using namespace std;
#define PORT 1117
bool quizInProgress = true; 


void getQuestion(int socketServer){

    char mesaj[1024];
    memset(mesaj, 0, sizeof(mesaj));
    
    try {
        if(recv(socketServer, mesaj, sizeof(mesaj),0) <= 0) 
            throw(22);
        }
    catch(...)
        {cout<<"Eroare la read de la server\n";  exit(1);}

    mesaj[sizeof(mesaj)-1]='\0';
    cout<<mesaj;


    if(strstr(mesaj,"Clientul cu id-ul")!=nullptr)
        quizInProgress=false;
    if(quizInProgress==false)
        return;
    char buf[256];
    memset(buf, 0, sizeof(buf));
    cin.getline(buf,sizeof(buf));
    buf[sizeof(buf)-1]='\0';

    if(strstr(buf,"logout")!=nullptr){
        char mesaj2[256];
        strcpy(mesaj2,"eu am parasit jocul");
        try {
            if(send(socketServer,mesaj2,sizeof(mesaj2),0) <= 0)
                throw(22);
            }
        catch(...)
            {cout<<"Eroare la write spre server\n";  exit(1);}
            
            char raspuns[256];
    memset(raspuns, 0, sizeof(raspuns));
    try{
        if(recv(socketServer, raspuns, sizeof(raspuns),0) <= 0) 
            throw(22);
        }
    catch(...)
        {cout<<"Eroare la read de la server\n";  exit(1);}
    
    raspuns[sizeof(raspuns)-1]='\0';

    cout<<raspuns;
    exit(1);
            }
    else {
        try {
            if(send(socketServer,buf,sizeof(buf),0) <= 0)
                throw(22);
            }
        catch(...)
            {cout<<"Eroare la write spre server\n";  exit(1);}
            char raspuns[256];
    memset(raspuns, 0, sizeof(raspuns));
    try{
        if(recv(socketServer, raspuns, sizeof(raspuns),0) <= 0) 
            throw(22);
        }
    catch(...)
        {cout<<"Eroare la read de la server\n";  exit(1);}
    
    raspuns[sizeof(raspuns)-1]='\0';

    cout<<raspuns;
        }


}


int main()
{
    int socketServer;
    sockaddr_in server;
    try {
        if((socketServer=socket(AF_INET,SOCK_STREAM,0))==-1)
            throw(22);
        }
    catch(...) {cout<<"Eroare la socket()"; exit(1);}

    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(PORT);

    try {
        if(connect(socketServer,reinterpret_cast<sockaddr*>( &server),sizeof(server))==-1)
            throw(22);
        }
    catch(...)
        {cout<<"Eroare la connect"; exit(1);}


    while(true){

    char buf[256];
    memset(buf, 0, sizeof(buf));
    cout<<"Client: Introduceti o comanda: ";
    fflush(stdout);

    cin.getline(buf,sizeof(buf));
    buf[sizeof(buf)-1]='\0';

    if(strstr(buf,"login")==nullptr){
        try{
            if(send(socketServer,buf,sizeof(buf),0) <= 0)
                throw(22);
            }
        catch(...)
            {cout<<"Eroare la write spre server\n";  exit(1);}
    }
    else
        if(strstr(buf, "logout")!=nullptr)
            break;
        else
            {
            try {
                if(send(socketServer,buf,sizeof(buf),0) <= 0)
                    throw(22);
                }
            catch(...)
                {cout<<"Eroare la write spre server\n";  exit(1);}   
            }

    char mesaj[256];
    memset(mesaj, 0, sizeof(mesaj));
    try {
        if(recv(socketServer, mesaj, sizeof(mesaj),0) <= 0) 
            throw(22);
        }
    catch(...)
        {cout<<"Eroare la read de la serveraa\n";  exit(1);}
    
    mesaj[sizeof(mesaj)]='\0';
    cout<<mesaj;
    fflush(stdout);


    while(quizInProgress==true)
        getQuestion(socketServer);
    if(quizInProgress==false)
        break;
    }

    close (socketServer);
    return 0;

}