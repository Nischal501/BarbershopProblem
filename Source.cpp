#include <iostream>
#include <cstdlib>
#include <sstream>
#include <stdio.h>
#include <semaphore.h>
#include <mutex>
#include <pthread.h>

using namespace std;

int waitingRoom[25];
int WaitInLine = 0;
int RemoveFromLine = 0;

sem_t emptyChairs;  //Semaphore for waiting room chairs
sem_t fullChairs;  //Semaphore for full chairs

pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER; //Mutex lock for the waiting room
pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;  //For cout function

struct OurThreads 
{
    int ID;
    int HowmanyCustomers;
};

void joinQueue(int id) 
{
    waitingRoom[WaitInLine] = id;
    WaitInLine++;  //Fill the line
}

int Customers() 
{
    int id = waitingRoom[RemoveFromLine];  //Haircut for this customer
    RemoveFromLine++;  //Leaves after the haircut
    return id;  //Return the user id
}

void* MakeBarbWork(void* CustomerInfo)
{
    struct OurThreads* p = (struct OurThreads*) CustomerInfo;
    pthread_mutex_lock(&printMutex);
    cout << "The barber is sleeping. \n";
    pthread_mutex_unlock(&printMutex);

    for (int i = 0; i < p->HowmanyCustomers; i++) 
    {
        sem_wait(&fullChairs);
        pthread_mutex_lock(&waitMutex);
        int customerID = Customers();

        pthread_mutex_lock(&printMutex);
        cout << "The barber is cutting customers hair. \n";
        pthread_mutex_unlock(&printMutex);

        pthread_mutex_unlock(&waitMutex);
        sem_post(&emptyChairs);

        //Customer leaves
        pthread_mutex_lock(&printMutex);
        cout << "Customer " << customerID << " is leaving the barber shop. \n";
        pthread_mutex_unlock(&printMutex);

        //if there is a full chair
        int numFullChairs;
        sem_getvalue(&fullChairs, &numFullChairs);
        if (numFullChairs == 0) {
            //the barber is sleeping
            pthread_mutex_lock(&printMutex);
            cout << "The barber is sleeping. \n";
            pthread_mutex_unlock(&printMutex);
        }
    }
    return NULL;
}


void* Barber(void* CustomerInfo) 
{
    struct OurThreads* p = (struct OurThreads*) CustomerInfo;
    pthread_mutex_lock(&printMutex);
    cout << "Customer " << p->ID << " is in the barber shop.\n";
    pthread_mutex_unlock(&printMutex);
   
    sem_wait(&emptyChairs); //Customer takes an empty seat; empty seat reduced by one

    pthread_mutex_lock(&waitMutex); //Adding the customer to the waiting queue
    joinQueue(p->ID);

    pthread_mutex_lock(&printMutex);
    cout << "Customer " << p->ID << " took an empty seat.\n";
    pthread_mutex_unlock(&printMutex);

    pthread_mutex_unlock(&waitMutex);
    sem_post(&fullChairs);

    int numFullChairs;
    sem_getvalue(&fullChairs, &numFullChairs);

    //Make barber wake up when somebody in the waiting queue
    if (numFullChairs == 1) {
        pthread_mutex_lock(&printMutex);
        cout << "Customer " << p->ID << " wakes the barber up. \n";
        pthread_mutex_unlock(&printMutex);
    }
    return NULL;
}

int main()
{
    cout << "Welcome to the Evilll Barber Shop!!!\n";
    int customerCount = 26;

    while (customerCount > 25 || customerCount <= 0)
    {
        cout << "How many customers are in the shop?(Enter between 0 and 25.)\n";
        cin >> customerCount;

        //if the customercount is out of range
        if (customerCount > 25 || customerCount < 0) {
            cout << "Input Error!!\n";
        }
    }

    int NoOfChairrs;
    cout << "How many empty chairs are there in the waiting room? \n";
    cin >> NoOfChairrs;

    sem_init(&emptyChairs, 0, NoOfChairrs);

    sem_init(&fullChairs, 0, 0);

    //creating thread ids
    pthread_t* threads = new pthread_t[customerCount];
    pthread_t barberThread;

    //Checking if the thread creation was sucessfull
    int errorCheck;

    //create thread properties
    pthread_attr_t attribute;

    //status of the thread
    void* status;

    //initialize and set thread joinable
    pthread_attr_init(&attribute);
    pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_JOINABLE);

    //creating an array of structs to pass to threads
    OurThreads* allThreads = new OurThreads[customerCount];

    //creating struct to pass to barber thread
    OurThreads* barberInfo;

    OurThreads tempStruct;
    tempStruct.HowmanyCustomers = customerCount;
    tempStruct.ID = 753; //a random id to use this struct

    //setting up a pointer to point to it
    barberInfo = &tempStruct;

    //creating barber thread
    pthread_create(&barberThread, &attribute, &MakeBarbWork, (void*)barberInfo);

    //creating customer threads
    for (int i = 0; i < customerCount; i++)
    {
        //creating struct to pass info to Barber
        allThreads[i].HowmanyCustomers = customerCount;
        allThreads[i].ID = i;

        //creating a pointer to be sent to the thread
        OurThreads* tempThread;

        //assigning pointer to the correct struct in allThreads
        tempThread = &allThreads[i];

        pthread_mutex_lock(&printMutex);
        pthread_mutex_unlock(&printMutex);

        errorCheck = pthread_create(&threads[i], &attribute, &Barber, (void*)tempThread);

        //Checking to see if threads were sucessfully created
        if (errorCheck) {
            cout << "Unable to create thread!! " << errorCheck << endl;
            exit(-1);
        }
    }

    //free the attribute and wait for other threads
    pthread_attr_destroy(&attribute);

    int results;

    //joining the threads
    for (int i = 0; i < customerCount; i++) {
        //this waits for all the results to be completed
        results = pthread_join(threads[i], &status);

        if (results) {
            cout << "Error:unable to join," << results << endl;
            exit(-1);
        }

    }

    cout << "The barber is off for the day!!\n " << endl;
    cout.flush();
    pthread_exit(NULL);

}