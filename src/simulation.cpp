/*
Student Name: Ece Dilara Aslan
Compiling Status: Compiling
Program Status: Working
*/

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace std;

ofstream output; //output file

pthread_mutex_t outFile; //mutex for writing into output file

sem_t arrivalA, arrivalB, arrivalC; //semaphore for synchronization of writing first lines of the tellers (Teller A/B/C has arrived.)
sem_t availableTeller; //semaphore for checking whether there is any available teller
pthread_mutex_t findTeller; //mutex for finding which teller is available through a global array
sem_t semA, semB, semC; //semaphore for tellers to wait until a client arrives into teller A, B, C, respectively
pthread_mutex_t findSeat; //mutex for checking available seats

bool available[3] = {true, true, true}; //stores whether teller A, B, C is available (0-->A, 1-->B, 2-->C)
string bufferA[3], bufferB[3], bufferC[3]; //stores the information about the client which goes to teller A, B, C, respectively
bool seats[201]; //stores whether a seat is available or not
int capacity; //stores the capacity of the theater hall
int smallestSeat = 1; //stores the smallest available seat number
int finishedClient = 0; //stores the number of clients which are finished their execution

void* client(void* param){ //client thread
    string clientInfo = *(string*) param;
    string name, serviceTime, seatNum;
    int arrivalTime;

    stringstream token(clientInfo); //tokenizes the information about the client
    getline(token, name, ',');
    string temp;
    getline(token, temp, ',');
    arrivalTime = stoi(temp);
    getline(token, serviceTime, ',');
    getline(token, seatNum, ',');

    this_thread::sleep_for(chrono::milliseconds(arrivalTime)); //waits for its arrival time

    sem_wait(&availableTeller); //waits for an available teller

    pthread_mutex_lock(&findTeller); //checks which teller is available
    int teller;
    for(teller = 0; teller<3; teller++){
        if(available[teller]){
            available[teller] = false;
            break;
        }
    }
    pthread_mutex_unlock(&findTeller);

    if(teller == 0){ //if teller A is available
        //puts the information about itself to a buffer of teller A
        bufferA[0] = name; 
        bufferA[1] = serviceTime;
        bufferA[2] = seatNum;

        sem_post(&semA); //informs teller A that it is chosen

    }
    else if(teller == 1){ //if teller B is available
        //puts the information about itself to a buffer of teller B
        bufferB[0] = name;
        bufferB[1] = serviceTime;
        bufferB[2] = seatNum;

        sem_post(&semB); //informs teller B that it is chosen

    }
    else if(teller == 2){ //if teller C is available
        //puts the information about itself to a buffer of teller C
        bufferC[0] = name;
        bufferC[1] = serviceTime;
        bufferC[2] = seatNum;

        sem_post(&semC); //informs teller C that it is chosen

    }

    return NULL;
}

void* tellerA(void* param){ //teller A thread
    pthread_mutex_lock(&outFile); //writes into the output file
    output << "Teller A has arrived." << endl;
    pthread_mutex_unlock(&outFile);
    sem_post(&arrivalA); //informs teller B that it has written

    string line;
    string name;
    int serviceTime, seatNum;

    while(true){

        sem_wait(&semA); //waits until a client chooses teller A
        
        name = bufferA[0];
        serviceTime = stoi(bufferA[1]);
        seatNum = stoi(bufferA[2]);

        pthread_mutex_lock(&findSeat); //finds an available seat for the associated client
        if(seatNum <= capacity && seats[seatNum]){ //if the wanted seat is valid and available
            line = name + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(seatNum) + ". Signed by Teller A.";
            seats[seatNum] = false;
            if(seatNum == smallestSeat){
                for(; smallestSeat<=capacity; smallestSeat++){
                    if(seats[smallestSeat]){
                        break;
                    }
                }
            }
        }
        else if(smallestSeat <= capacity){ //if there are still available seats but the wanted seat is not valid or not available
            line = name + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(smallestSeat) + ". Signed by Teller A.";
            seats[smallestSeat] = false;        
            for(; smallestSeat<=capacity; smallestSeat++){
                if(seats[smallestSeat]){
                    break;
                }
            }
        }
        else{ //if there is no available seat
            line = name + " requests seat " + to_string(seatNum) + ", reserves None. Signed by Teller A.";
        }
        pthread_mutex_unlock(&findSeat);

        this_thread::sleep_for(chrono::milliseconds(serviceTime)); //waits for the service time of the client

        pthread_mutex_lock(&outFile); //writes into the output file about the information of chosen seat
        output << line << endl;
        pthread_mutex_unlock(&outFile);

        pthread_mutex_lock(&findTeller); //makes teller A available again and increases the number of finished clients
        available[0] = true;
        finishedClient++;
        pthread_mutex_unlock(&findTeller);

        sem_post(&availableTeller); //makes one of the tellers available

    }

    return NULL;
}

void* tellerB(void* param){ //teller B thread
    sem_wait(&arrivalA); //waits until teller A writes its first line
    pthread_mutex_lock(&outFile); //writes into the output file
    output << "Teller B has arrived." << endl;
    pthread_mutex_unlock(&outFile);
    sem_post(&arrivalB); //informs teller C that it has written

    string line;
    string name;
    int serviceTime, seatNum;

    while(true){

        sem_wait(&semB); //waits until a client chooses teller B
        
        name = bufferB[0];
        serviceTime = stoi(bufferB[1]);
        seatNum = stoi(bufferB[2]);

        pthread_mutex_lock(&findSeat); //finds an available seat for the associated client
        if(seatNum <= capacity && seats[seatNum]){ //if the wanted seat is valid and available
            line = name + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(seatNum) + ". Signed by Teller B.";
            seats[seatNum] = false;
            if(seatNum == smallestSeat){
                for(; smallestSeat<=capacity; smallestSeat++){
                    if(seats[smallestSeat]){
                        break;
                    }
                }
            }
        }
        else if(smallestSeat <= capacity){ //if there are still available seats but the wanted seat is not valid or not available
            line = name + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(smallestSeat) + ". Signed by Teller B.";
            seats[smallestSeat] = false;        
            for(; smallestSeat<=capacity; smallestSeat++){
                if(seats[smallestSeat]){
                    break;
                }
            }
        }
        else{ //if there is no available seat
            line = name + " requests seat " + to_string(seatNum) + ", reserves None. Signed by Teller B.";
        }
        pthread_mutex_unlock(&findSeat);

        this_thread::sleep_for(chrono::milliseconds(serviceTime)); //waits for the service time of the client

        pthread_mutex_lock(&outFile); //writes into the output file about the information of chosen seat
        output << line << endl;
        pthread_mutex_unlock(&outFile);

        pthread_mutex_lock(&findTeller); //makes teller B available again and increases the number of finished clients
        available[1] = true;
        finishedClient++;
        pthread_mutex_unlock(&findTeller);

        sem_post(&availableTeller); //makes one of the tellers available

    }

    return NULL;
}

void* tellerC(void* param){ //teller C thread
    sem_wait(&arrivalB); //waits until teller B writes its first line
    pthread_mutex_lock(&outFile); //writes into the output file
    output << "Teller C has arrived." << endl;
    pthread_mutex_unlock(&outFile);
    sem_post(&arrivalC); //informs main thread that it has written

    string line;
    string name;
    int serviceTime, seatNum;

    while(true){

        sem_wait(&semC); //waits until a client chooses teller C

        name = bufferC[0];
        serviceTime = stoi(bufferC[1]);
        seatNum = stoi(bufferC[2]);

        pthread_mutex_lock(&findSeat); //finds an available seat for the associated clients
        if(seatNum <= capacity && seats[seatNum]){ //if the wanted seat is valid and available
            line = name + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(seatNum) + ". Signed by Teller C.";
            seats[seatNum] = false;
            if(seatNum == smallestSeat){
                for(; smallestSeat<=capacity; smallestSeat++){
                    if(seats[smallestSeat]){
                        break;
                    }
                }
            }
        }
        else if(smallestSeat <= capacity){ //if there are still available seats but the wanted seat is not valid or not available
            line = name + " requests seat " + to_string(seatNum) + ", reserves seat " + to_string(smallestSeat) + ". Signed by Teller C.";
            seats[smallestSeat] = false;        
            for(; smallestSeat<=capacity; smallestSeat++){
                if(seats[smallestSeat]){
                    break;
                }
            }
        }
        else{ //if there is no available seat
            line = name + " requests seat " + to_string(seatNum) + ", reserves None. Signed by Teller C.";
        }
        pthread_mutex_unlock(&findSeat);

        this_thread::sleep_for(chrono::milliseconds(serviceTime)); //waits for the service time of the client

        pthread_mutex_lock(&outFile); //writes into the output file about the information of chosen seat
        output << line << endl;
        pthread_mutex_unlock(&outFile);

        pthread_mutex_lock(&findTeller); //makes teller C available again and increases the number of finished clients
        available[2] = true;
        finishedClient++;
        pthread_mutex_unlock(&findTeller);

        sem_post(&availableTeller); //makes one of the tellers available

    }

    return NULL;
}

int main(int argc, char* argv[]){
    //initialization of mutexes and semaphores
    sem_init(&arrivalA, 0, 0);
    sem_init(&arrivalB, 0, 0);
    sem_init(&arrivalC, 0, 0);
    pthread_mutex_init(&outFile, NULL);
    sem_init(&availableTeller, 0, 3);
    pthread_mutex_init(&findTeller, NULL);
    sem_init(&semA, 0, 0);
    sem_init(&semB, 0, 0);
    sem_init(&semC, 0, 0);
    pthread_mutex_init(&findSeat, NULL);
    
    output.open(argv[2], ios::out);
    output << "Welcome to the Sync-Ticket!" << endl;

    pthread_t tidA, tidB, tidC;
    pthread_create(&tidA, NULL, tellerA, NULL);
    pthread_create(&tidB, NULL, tellerB, NULL);
    pthread_create(&tidC, NULL, tellerC, NULL);

    sem_wait(&arrivalC); //waits until each teller has written its first line

    ifstream input;
    input.open(argv[1], ios::in);
    
    string theaterHall; //name of the theater hall
    input >> theaterHall;
    //stores the proper capacity and initializes seats as available
    if(theaterHall == "OdaTiyatrosu"){
        capacity = 60;
    }
    else if(theaterHall == "UskudarStudyoSahne"){
        capacity = 80;
    }
    else if(theaterHall == "KucukSahne"){
        capacity = 200;
    }
    fill(seats, seats+201, true);

    int clientNum; //number of clients
    input >> clientNum;
    
    string clientInfo[clientNum]; //temporary array for parameters of client threads

    for(int i=0; i<clientNum; i++){
        input >> clientInfo[i];
        pthread_t tidClient;
        pthread_create(&tidClient, NULL, client, &clientInfo[i]);
    }

    while(finishedClient < clientNum); //waits until all clients finishes their executions

    output <<  "All clients received service." << endl;
    output.close();
    return 0;
}