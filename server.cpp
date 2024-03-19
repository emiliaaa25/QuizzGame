#include<sys/socket.h>
#include<iostream>
#include<cstring>
#include<arpa/inet.h>
#include<thread>
#include<unistd.h>
#include<vector>
#include<map>
#include<mutex>
#include<shared_mutex>
#include<algorithm>
#include<fstream>
#include<chrono>
#include <condition_variable>
#include <SFML/Graphics.hpp>
#include<ctime>
#include<fcntl.h>

//Pentru compilare folositi:
//g++ server.cpp -o server -lsfml-graphics -lsfml-window -lsfml-system
//dupa ce ati instalat sfml 


using namespace std;
using namespace chrono;

#define PORT 1117
int timpLimitaSecunde=15;
sf::Font font;
sf::RenderWindow window(sf::VideoMode(3000, 700), "QuizzGame");
sf::Texture backgroundTexture;
sf::Sprite backgroundSprite;
sf::Sprite PresenterSprite;
sf::Sprite PisicaSprite;

class TextBox {
private:
    sf::RectangleShape box;
    bool isSelected;

public:
sf::Text text;
TextBox(const sf::Font& font,float width, float height,unsigned int charSize = 10) {
    box.setSize(sf::Vector2f(width, height));
    box.setFillColor(sf::Color::Black);
    box.setOutlineColor(sf::Color::White);
    box.setOutlineThickness(2);

    text.setFont(font);
    text.setCharacterSize(charSize);
    text.setFillColor(sf::Color::White);
    text.setPosition(box.getPosition().x + 5, box.getPosition().y + 5);
}

    void setText(const std::string& str) {
        text.setString(str);
    }
    

    void setFont(){
         if (!font.loadFromFile("arial.ttf")) 
            return;
    }

    void setPosition(float x, float y) {
        box.setPosition(x, y);
        text.setPosition(x + 5, y + 5);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(box);
        window.draw(text);
    }

};

struct  Question {
    string text;
    vector<string> options;
    string correct;
    int index=0;
};


struct thData{
    int idThread;
    int cl;
};

class Client{
    public:
    Client():id(1),score(0),active(true),nume("da"){}
    Client(int id, string nume,bool active):id(id),score(0),active(true),nume(nume){}
    int getId() const {return id;}
    void updateScore(){score=score+10;}
    void updateIntrebari(){intrebari++ ;}
    int getIntrebari()const{return intrebari;}
    int getScore()const {return score;}
    int getSocket()const {return socket;}
    int getActive()const {return active;}
    void print(){
        cout<<id<<" "<<score<<" "<<" "<<active<<" "<<nume;
    }
    string getName()const{return nume;}    
    void setSocket(int socketDescriptor) {
        this->socket = socketDescriptor;
    }
    void setInactive(){active=false;}
    private:
    int id;
    int socket;
    int score;
    string nume;
    bool active;
    int intrebari=0;
};

class Quizz{
public:
Quizz(int port):port(PORT){}
void start();
static map<int,Client> clienti;
shared_mutex intrebarimutex;
mutex printMutex;
condition_variable condVar;
vector<string> intrebari;
vector<string> raspunsuri;
static bool gamestate;
static bool game;
void  handleCommand(thData *data);
void sendQestion(thData *data);
void reincepe();
void receiveAndProcessAnswer(thData* data, string correct);
void actualizarePunctaj(thData* data);
void trimite_clasament(thData* data);
void incarcaIntrebari();
void verificaFinal();
void bubbleSort(vector<Client>& vec);
void startGame();
void loginUsers(char comanda[256],thData* client);
void loadQuestions(vector <Question>& questions);
void waitForClients();
void seteaza_grafica();
void deseneaza_Clienti();
void seteazaTextPrezentator(string s);
void seteazaTextClient(string s);
static vector <Question> clientQuestions;

private:
int socketServer;
    vector<thread> threads;
    sockaddr_in server,from;
    int port;
};


map<int,Client> Quizz::clienti;
vector <Question> Quizz::clientQuestions;
bool Quizz::gamestate=true;
bool Quizz::game=true;
int id1=-1;
int index1=0;
int id2=0;
string clasament;
int x=0;
int trimiteri=0;
int i=0;
int loc=0;

void Quizz::start(){
    try{
        if((socketServer=socket(AF_INET,SOCK_STREAM,0))==-1)
            {
                throw(22);
            }
    }
    catch(...)
        {cout<<"Eroare la socket()";exit(1);}
    
    int on=1;
    setsockopt(socketServer,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(PORT);

    memset(&from,0,sizeof(from));
    try {
        if(bind(socketServer,reinterpret_cast<sockaddr*>( &server),sizeof(server))==-1)
            throw(23);
        }
    catch(...)
    {cout<<"Eroare la bind"; exit(1);}
 
    try{
    if(listen(socketServer,2)==-1)
        throw(24);
    }
    catch(...)
    {cout<<"Eroare la listen"; exit(1);}

    cout<<"Astept la Potul "<<PORT<<"\n";
    fflush(stdout);
    seteaza_grafica();
}


void Quizz::waitForClients()
{   
    thData * data;

    while(gamestate==true){
         int client;
         socklen_t length;
            length=sizeof(from);

        try{
            if((client=accept(socketServer,reinterpret_cast<sockaddr*>( &from),&length))==-1)
                throw(25);
        }
        catch(...)
        {cout<<"Eroare la accept"; break;}
        
	    data = new thData();
        data->idThread = i++;
        data->cl=client;
        
        threads.push_back(thread(&Quizz::handleCommand, this, data));
    }
}

void Quizz::trimite_clasament(thData* data){
    vector<Client> vec;
    
     map<int,Client>::iterator it;
     for (it = clienti.begin(); it != clienti.end(); ++it) {
        int cheie = it->first;
        Client& client = it->second;
        vec.push_back(client);
    }

    bubbleSort(vec);
    trimiteri++;
    clasament+="CLASAMENTUL ACESTUI JOC: \n\n";
    
    for (const auto& client1 : vec) {
        loc++;
        clasament+="Clientul cu id-ul: ";
        clasament+=to_string(client1.getId());
        clasament+=", numele de utilizator: ";
        clasament+=client1.getName();
        clasament+=" a obtinut scorul ";
        clasament+=to_string(client1.getScore());
        clasament+=" si s-a clasat pe locul: ";
        clasament+=to_string(i);
        clasament+=". ";
        if(client1.getActive()==1)
            clasament+="Clientul este inca activ.";
        else
            clasament+="Clientul nu mai este activ.";
        clasament+="\n";
        clasament+="\n";
    }

    try{
        if (send(data->cl, clasament.c_str(), clasament.length(), 0)<=0) 
            throw(22);
        }
        catch(...)                      
        {cout<<"Eroare la write spre clientttt\n"; exit(1);}
        clasament=""; 
}


void Quizz::handleCommand(thData *data){
char comanda[256];
memset(comanda, 0, sizeof(comanda));
try{
    if(recv(data->cl, comanda, sizeof(comanda),0) <= 0) 
        throw(22);
    }
    catch(...)
    {cout<<"[Thread " << data->idThread << "]\n"<<"Eroare la read de la client\n"; exit(1);}
    comanda[strlen(comanda)]='\0';
    if(strstr(comanda,"login")!=nullptr)
        {id2++;loginUsers(comanda,data);}
    else
        if(strstr(comanda,"logout"))
        {
        char mesaj[256];
        strcpy(mesaj,"V-ati delogat cu succes ");
        strcat(mesaj,"\n");
        try{
            if(send(data->cl,mesaj,sizeof(mesaj),0) <= 0)
                throw(22);
            }
        catch(...)
        {cout<<"Eroare la write spre client\n"; exit(1);}
        mesaj[0]='\0';   
        }
        else {
            char mesaj[256];
            strcpy(mesaj,"Nu ati introdus o comanda valida ");
            strcat(mesaj,"\n");
            try{
                if(send(data->cl,mesaj,sizeof(mesaj),0) <= 0)
                    throw(22);
              }
            catch(...)
            {cout<<"Eroare la write spre client\n"; exit(1);}
            }
comanda[0]='\0';
}


void Quizz::bubbleSort(vector<Client>& vec){
int i, j;
for(i=0;i<vec.size()-1;i++)
    for(j=0;j<vec.size();j++)
        if(vec[i].getScore()<vec[j].getScore())
            swap(vec[i],vec[j]);
}

void Quizz::loginUsers(char comanda[256],thData* data){
    char utilizator[256];
    int k=0;
    int i;

    for(i=0;i<strlen(comanda);i++)
        if(comanda[i]=='l'&&comanda[i+1]=='o'&&comanda[i+2]=='g'&&comanda[i+3]=='i'&&comanda[i+4]=='n')
            break;
    i=i+5;
    while(isspace(comanda[i])||(!isalpha(comanda[i])&&!isdigit(comanda[i])))
        i++;
    while(isalpha(comanda[i])||isdigit(comanda[i]))
        {utilizator[k]=comanda[i];k++;i++;}
    utilizator[k]='\0';
    id1++;
    if (utilizator!=nullptr){
        clienti[id1]=Client (id1,utilizator,true);
        clienti[id1].setSocket(data->cl);      
        char mesaj[256];
        strcpy(mesaj,"V-ati logat cu succes ");
        strcat(mesaj,utilizator);
        strcat(mesaj,"\n");
        try{
        if(send(data->cl,mesaj,sizeof(mesaj),0) <= 0)
            throw(22);}
        catch(...)
        {cout<<"Eroare la write spre client\n";
        exit(1);}
        mesaj[0]='\0';
        seteazaTextPrezentator(intrebari[0]);
        sendQestion(data);
    }
}



void Quizz::incarcaIntrebari() {
    loadQuestions(clientQuestions);
    string message;
    for (const auto& currentQuestion:clientQuestions ){
        message=currentQuestion.text+"\n";
        for (const auto& option : currentQuestion.options) 
            message += option + "\n";
     intrebari.push_back(message);
     raspunsuri.push_back(currentQuestion.correct);
   }


}

void Quizz::sendQestion(thData* data) {
    for (size_t j = 0; j < intrebari.size(); j++) {
        sleep(1);
        shared_lock<shared_mutex> lock(intrebarimutex);
        seteazaTextPrezentator(intrebari[j]);
        try {
            if (send(data->cl, intrebari[j].c_str(), intrebari[j].length(), 0)<=0) 
                throw(22);
            else
                {x++;
                clienti[data->idThread].updateIntrebari();
                }
            }
        catch(...)                      
            {cout<<"Eroare la write spre clientttt\n";  exit(1);}   
        char buffer[256];
        memset(buffer,0,sizeof(buffer));
        int ok=0;
        auto start = high_resolution_clock::now();
        auto end_time = start + seconds(timpLimitaSecunde);
     
        while (high_resolution_clock::now() < end_time) {
            struct timeval tv;
            tv.tv_sec = 1; 
            tv.tv_usec = 0;

            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(data->cl, &read_fds);

            if (select(data->cl + 1, &read_fds, nullptr, nullptr, &tv) > 0) {
                if (recv(data->cl, buffer, sizeof(buffer), 0)>0)
                    {ok=1;break;}
            }
                if (std::chrono::high_resolution_clock::now() >= end_time) {
                    break;
                }    
        } 
            
        buffer[sizeof(buffer)-1]='\0';
        if(ok==0)
        {
            if (recv(data->cl, buffer, sizeof(buffer), 0)<0)
                memset(buffer,0,sizeof(buffer));
        }
   
                
        if(ok==1){
            if(strstr(buffer,"eu am parasit jocul")!=nullptr){
                
                clienti[data->idThread].setInactive();
                char mesaj[256];
                memset(mesaj,0, sizeof(mesaj));
                strcpy(mesaj, "Te-ai delogat cu succes\n");
                try {
                    if (send(data->cl, mesaj, sizeof(mesaj), 0)<=0) 
                        throw(22);
                    }
                catch(...)                      
                    {cout<<"Eroare la write spre clientttt\n";  exit(1);}
                x=x-clienti[data->idThread].getIntrebari();
                id2--;
                break;
            }
            else {
                if(strcmp(buffer,raspunsuri[j].c_str())==0){
                    char mesaj[256];
                    strcpy(mesaj, "BRAVO AI RASPUNS CORECT\n");
                    try{
                        if (send(data->cl, mesaj, sizeof(mesaj), 0)<=0) 
                            throw(22);}
                    catch(...)                      
                        {cout<<"Eroare la write spre clientttt\n"; exit(1);}
                    actualizarePunctaj(data);
                }
                else {
                    char mesaj[256];
                    strcpy(mesaj, "NU AI RASPUNS CORECT\n");
                    try{
                        if (send(data->cl, mesaj, sizeof(mesaj), 0)<=0) 
                            throw(22);}
                    catch(...)                      
                        {cout<<"Eroare la write spre clientttt\n"; exit(1);}
                    }

                }
        }
        else {
            char mesaj[256];
            strcpy(mesaj, "NU AI RASPUNS LA TIMP\n");
            try{
                if (send(data->cl, mesaj, sizeof(mesaj), 0)<=0) 
                    throw(22);}
            catch(...)                      
                {cout<<"Eroare la write spre clientttt\n"; exit(1);}

            }
    seteazaTextPrezentator(intrebari[j+1]);
    if(x==index1*id2)
        condVar.notify_all();
    }
    unique_lock<mutex> lock(printMutex);
    condVar.wait(lock, [] { return x == index1*id2; });
    trimite_clasament(data);
       lock.unlock();
    if(trimiteri==id2)
        reincepe(); 
}


void Quizz::reincepe(){

    id1=-1;
    id2=0;
    trimiteri=0;
    x=0;
    i=0;
    clasament="";
    loc=0;
    clienti.clear();
    for (auto& thread : threads) {
        thread.detach();
    }
    threads.clear();
    waitForClients();
}

void Quizz::actualizarePunctaj(thData* data) {
    Quizz::clienti[data->idThread].updateScore();       
}

string manualTrim(const string& str) {
    size_t start = 0;
    size_t end = str.length() - 1;

    while (start <= end && (str[start] == ' ' || str[start] == '\t' || str[start] == '\n' || str[start] == '\r' || str[start] == '\f' || str[start] == '\v')) {
        start++;
    }

    while (end >= start && (str[end] == ' ' || str[end] == '\t' || str[end] == '\n' || str[end] == '\r' || str[end] == '\f' || str[end] == '\v')) {
        end--;
    }

    if (start > end) {
        return ""; 
    }

    return str.substr(start, end - start + 1);
}

void Quizz::loadQuestions(vector <Question>& clientQuestions){
ifstream fin("questions.xml");
string line;
while(getline(fin, line)){
    fflush(stdout);
    if(strstr(line.c_str(), "<question>")!=nullptr)
    {
        Question current={};
        getline(fin,line);
        line=manualTrim(line);
        current.text = line.substr(6, line.find("</text>") - 6);
        index1++;
        current.index=index1;
        getline(fin, line);
        while(getline(fin, line)&&strstr(line.c_str(),"/options")==nullptr)
            {line=manualTrim(line);
            string option = line.substr(8, line.find("</option>") - 8);
            current.options.push_back(option);     }
        getline(fin, line);
        line=manualTrim(line);
        current.correct = line.substr(9, line.find("</correct>") - 9);
        getline(fin, line);
        clientQuestions.push_back(current);
    }   
}
}


void Quizz::deseneaza_Clienti()
{
    int margine1=50;
    int margine2=450;
    std::vector<sf::Texture> texturi;
    for (int i = 1; i <= id2; i++) {
        sf::Texture textura;
        std::string numeFisier = "cat" + std::to_string(i) + ".png"; 
        if (!textura.loadFromFile(numeFisier)) {
        }
        texturi.push_back(textura);
    }

     for (int i = 0; i < id2; i++) {
        sf::Sprite sprite;
        sprite.setTexture(texturi[i]);
        sprite.setPosition(margine1, margine2);
        window.draw(sprite);
        margine1 = margine1 + 100;
    }
    window.display();
}


void Quizz::seteazaTextPrezentator(string s){


window.draw(backgroundSprite);
window.draw(PresenterSprite);
TextBox textBox(font,350, 100);
textBox.setFont();
textBox.setText(s);
textBox.setPosition(1430, 40);
textBox.draw(window);
deseneaza_Clienti();
window.display();
}

void Quizz::seteaza_grafica()
{
    if (!backgroundTexture.loadFromFile("room.jpg"))
        return;
    backgroundSprite.setTexture(backgroundTexture);
    
    float scaleX = 3000.0f / backgroundTexture.getSize().x;
    float scaleY = 700.0f / backgroundTexture.getSize().y;
    backgroundSprite.setScale(scaleX, scaleY);

    sf::Texture Presenter;
    if (!Presenter.loadFromFile("dog.png"))
        return;
    PresenterSprite.setTexture(Presenter);
    PresenterSprite.setPosition(1400,150);
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
           
        }

        window.clear();
        window.draw(backgroundSprite);
        window.draw(PresenterSprite);
        window.display();
        std::thread clientThread(&Quizz::waitForClients, this);
        clientThread.join(); 

    }
}


void Quizz::startGame()
{
while(game)
 {  
    incarcaIntrebari();
    start();
 }

}



int main(){

    
   
    Quizz joc(PORT);
    joc.startGame();

    return 0; 
}    
