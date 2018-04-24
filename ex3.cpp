////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                                      /*  Name: Mahdi Asali, Elon Avisror  ID: 206331795, 305370801 Assignement: 3*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include<sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <chrono>
#include <sys/wait.h>
#include <sys/sem.h>
#include <string.h>
#include <random>
//////////////////////////////////
#define SEGSIZE 1024
#define MAX_SIZE 7
#define SEMMSL 128
#define ORDER_AMOUNT_MAX 4//new
#define SEMAPHORES_COUNT 6

#define PRINT_ID_SEMAPHORE 0
#define WRITE_TO_ORDER_BOARD_ID_SEMAPHORE 1
#define READ_FROM_ORDER_BOARD_ID_SEMAPHORE 2
#define READ_FROM_MENU_ID_SEMAPHORE 3
#define READ_FROM_MENU_BOARD_ID_SEMAPHORE 4
#define WRITE_TO_MENU_BOARD_ID_SEMAPHORE 5

using namespace std;



//Functions Declerations
void printMenu(struct item _item[],int);
void initMenu(struct item _item[],int);
void checkArgs(int argc,char *argv[]);
void initArgs(float &Simt,int &menItemC,int &Custc,int &Waitc,char * argv[]);
void printSimulationArgs(float Simt,int menItemC,int Custc,int Waitc);
void startTimer();
float getTimeDiff();
void initSharedMemory();
//semaphore down/up functions
void downPrint(  );
void upPrint(  );
void downReadWriteOrder(); //new
void upReadWriteOrder(); //new
void printOrdersBoard();
void WaiterWriteOrder(int WaiterId);
//int initSemaphores(key_t key); //new
void initSemaphores();
void ManageSemaphores(); //new
void createCustomers(int);
void createWaiters(int);
float getTotalAmount();//new
void pout(string);//new
void pspawn();//new
int GetRandom(int max, int min); //new
bool PrevIsDone(pid_t pid,const int processIndex); //new
bool isAllDone();
void waitChildesEnd() ;
void addOrder(int customerId, int itemId, int amount) ;
void generateNewOrder(int customerId, int itemId, int amount, bool didOrdered) ;
void readMenuBoard(int customerId, int itemId, int amount, bool didOrdered) ;
string getItemNameById(int itemId);
float getAmount();//new
void printTotal();//new
void initOrderStruct();
void freeSemaphores(); //new
void freeSharedMemory();//new
void downWriteOrderBoard();
void upWriteOrderBoard();
void downMenuBoardRead();
void upMenuBoardRead();
int TotalOrders();
///////////////////////////STRUCTURES////////////////////////////
struct item{
    int id;
    char Name[16];
    double Price;
    int Orders;
};

struct order{
    int CustomerId;
    int ItemId;
    int Amount;
    bool Done;
};

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
} semProporties;


//Additional Functions (including structs)


string nameLists[MAX_SIZE]=
{"Pizza",
"Salad",
"Hamburger",
"Spaghetti",
"Pie   ",
"Milkshake",
"Cheeps"};

double PriceArr[MAX_SIZE]=
{10.00,
8.00,
12.00,
9.00,
9.50,
6.00,
8.00};

/////////////////////////////////////////////////////////////////


//Global Timer
chrono::steady_clock::time_point simulationTimer;

item * itemMenuTablePtr;
order * orderBoardPtr;
int menItemC,Custc,Waitc;
u_short semvals[SEMMSL];
float simulationTime;
int semaphoreSetId;



int main (int argc, char *argv[])
{
    srand (time(NULL));
    checkArgs(argc,argv);
    initArgs(simulationTime,menItemC,Custc,Waitc,argv);
    printSimulationArgs(simulationTime,menItemC,Custc,Waitc);
    initSharedMemory();
    initOrderStruct();
    initMenu(itemMenuTablePtr,menItemC);
    printOrdersBoard();
    initSemaphores();
    printMenu(itemMenuTablePtr,menItemC);
    startTimer();
    printf("%.3f  Main process start creating sub-processes\n", getTimeDiff());
    pspawn();//spawn the waiters and customers.
    waitChildesEnd();
    //printOrdersBoard();
    //printMenu(itemMenuTablePtr,menItemC);
    printTotal();
    freeSemaphores();
    freeSharedMemory();
}

void freeSemaphores(){
    semctl(semaphoreSetId,SEMAPHORES_COUNT , IPC_RMID);
}


void freeSharedMemory(){
    shmdt(itemMenuTablePtr); //release
    shmdt(orderBoardPtr); //release
}

//printing Total orders
void printTotal(){
    cout<<"Total orders "+to_string(TotalOrders())<<", for an amount "+to_string(getTotalAmount())+" NIL"<<endl;
    cout<<to_string(getTimeDiff())<<" Main ID "+ to_string(getpid()) + " end work"<<endl;
    cout<<to_string(getTimeDiff())<<" End of simulation"<<endl;
}

float getTotalAmount(){
    float sum=0.00;
    for(int i=0;i<menItemC;i++)
        sum+=itemMenuTablePtr[i].Orders * PriceArr[i];
    return sum;
}

void printOrdersBoard(){
    cout<<"========Order Board=========="<<endl;
    cout<<"Cust ID\titem id\tAmount\tDone"<<endl;
    for(int i=0;i<Custc;i++)
    {
        cout<<orderBoardPtr[i].CustomerId<<"\t"<<orderBoardPtr[i].ItemId<<"\t"<<orderBoardPtr[i].Amount<<"\t"<<orderBoardPtr[i].Done<<endl;
    }
    cout<<"============================"<<endl;

}
void initOrderStruct(){
    for(int i=0;i<Custc;i++) {
        orderBoardPtr[i].CustomerId = i;
        orderBoardPtr[i].Done = true;
        orderBoardPtr[i].ItemId = -1;
        orderBoardPtr[i].Amount = -1;
    }
}

void waitChildesEnd() {
    pid_t status;
    while ((wait(&status)) > 0);
}



void pspawn(){
    //processes = new process[Custc+Waitc];  //init processes informations (pid,type,etc..)
    createWaiters(Waitc); //creating waiters(workers).
    createCustomers(Custc); //creating customers.
}



int GetRandom(int max, int min)
{
    return min + (rand()%(max-min+1));
}


//calculate the total orders.
int TotalOrders(){
    int total=0;
    for(int i=0;i<menItemC;i++)
        total += itemMenuTablePtr[i].Orders ;
    return total;
}


//function creates waiters and spawn them .
void createWaiters(int Waitc){
    pid_t pid=-2;
    int myId;
    string myType;
    int mypid;
    for (int i=0; i<Waitc; ++i)
    {
        if (pid != 0)
            pid = fork();
        if(pid!=0){
            //cout<<"Waiter "<<i<<": created PID "<<cpid<<" PPID "<<getpid()<<endl;
            printf("%.3f  Waiter %d: created PID %d PPID %d\n", getTimeDiff(),i,pid,getpid());
        }
        if(pid==0)
        {
            myType="Waiter";
            myId=i;
            mypid=getpid();
            break;
        }

    } // done creating waiters //

    if(pid==0) {
        //waiter code (permission : childs)

        if (myType=="Waiter") {
            //cout << "waiter pid entered" << getpid() << endl;
            while (simulationTime > getTimeDiff()) {
                //read order from order board
                //if all done
                if (!isAllDone()) {
                    srand(getpid());
                    int sleeptime = GetRandom(2, 1);
                    sleep(sleeptime); /* this is step (b) between 3, 6 seconds */
                    //read an order
                    downReadWriteOrder();
                    WaiterWriteOrder(myId);
                    upReadWriteOrder();
                }
            }
        }

        pout(myType+" ID "+to_string(myId)+" PID"+to_string(mypid)+" end work PPID "+to_string(getppid()));
        exit(1);

    }

}

//creating customers and spawn them .
void createCustomers(int Custc){
    pid_t pid=-2;
    int myId; //customer index
    string myType; //customer type
    int mypid; //customer pid
    for (int i=0; i<Custc; ++i)
    {
        // make sure all the waiter pid will not mess around here
        if (pid != 0)
            pid = fork();
        if(pid!=0){
            //cout<<"Waiter "<<i<<": created PID "<<cpid<<" PPID "<<getpid()<<endl;
            printf("%.3f  Customer %d: created PID %d PPID %d\n", getTimeDiff(),i,pid,getpid());
        }
        if(pid==0)
        {
            srand(time(NULL)+getpid()*getppid());
            myType="Customer";
            myId=i;
            mypid=getpid();
            break;
        }
    } // done creating customers //
    //customers code (permission : childs)
    if(pid==0){
    if (myType=="Customer") {
        while (simulationTime > getTimeDiff()) {
            int sleeptime=GetRandom(6,3);
            sleep(sleeptime); /* this is step (b) between 3, 6 seconds */
            if(PrevIsDone(getpid(),myId)==true){  /* this is step (d) */                
                int didOrdered = GetRandom(1,0) >= 0.5;
                int itemId = GetRandom((menItemC-1), 0);
                int amount = GetRandom(ORDER_AMOUNT_MAX, 1);               
                generateNewOrder(myId, itemId, amount , didOrdered);
                itemId = 0;
            }
            //reloop
        }
        pout(myType+" ID "+to_string(myId)+" PID"+to_string(mypid)+" end work PPID "+to_string(getppid()));
        exit(1);
    }
    }
}


//function returns true in case all orders done.
bool isAllDone(){
    bool flag=true;
    for(int i=0;i<Custc;i++){
        if(orderBoardPtr[i].Done==false) {
            flag = false;
            break;
        }
    }
    return flag;
}




void readMenuBoard(int customerId, int itemId, int amount, bool didOrdered) {
    string orderString = didOrdered ? "ordered, amount " + to_string(amount): "doesn't want to order";
    pout("Customer ID " + to_string(customerId) + ": reads a menu about " + getItemNameById(itemId) + " (" + orderString + ")");
}



string getItemNameById(int itemId) {
    string itemName;
    downMenuBoardRead();
    itemName = itemMenuTablePtr[itemId].Name;
    upMenuBoardRead();

    itemName.erase(std::find_if(itemName.rbegin(), itemName.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), itemName.end());
    return itemName;
}
void generateNewOrder(int customerId, int itemId, int amount, bool didOrdered) {
        readMenuBoard(customerId, itemId, amount, didOrdered);
        if (didOrdered) addOrder(customerId, itemId, amount);
        sleep(1);

}

void addOrder(int customerId, int itemId, int amount) {
    downWriteOrderBoard();
    //cout<<"Customer locked add order pid:"<<getpid()<<endl;
    orderBoardPtr[customerId].ItemId = itemId;
    orderBoardPtr[customerId].Amount = amount;
    orderBoardPtr[customerId].Done  = false;
    //itemMenuTablePtr[itemId].Orders+=1;
    //cout<<"Customer unlocked add order pid:"<<getpid()<<endl;
    upWriteOrderBoard();
}

void WaiterWriteOrder(int WaiterId){
    //cout<<"waiter process pid:"<<getpid()<<" locked the board"<<endl;
    for(int i=0;i<Custc;i++)
    {
        if(orderBoardPtr[i].Done==false)
        {
            pout(
            " Waiter ID "+ 
            to_string(WaiterId) +
            ": performs the order of customer ID " + 
            to_string(orderBoardPtr[i].CustomerId) +
            " ( "+to_string(orderBoardPtr[i].Amount)+" "+
            getItemNameById(orderBoardPtr[i].ItemId)+" ) "
            );
            orderBoardPtr[i].Done=true;
            itemMenuTablePtr[orderBoardPtr[i].ItemId].Orders+=orderBoardPtr[i].Amount; //add to the total orders
            break;
        }
    }
    //cout<<"waiter process pid:"<<getpid()<<" unlocked the board"<<endl;
}

void downReadWriteOrder(){

    //algorithm:
    //1. Wait to menu writers to finish.
    //2. Wait to order writers to finish.
    //3. add a new writer to orderboard.
    auto * sops = new sembuf[3];
    sops[0].sem_op = 0;
    sops[0].sem_num = WRITE_TO_MENU_BOARD_ID_SEMAPHORE;
    sops[0].sem_flg = SEM_UNDO;
 
    sops[1].sem_op =0;
    sops[1].sem_num = WRITE_TO_ORDER_BOARD_ID_SEMAPHORE;
    sops[1].sem_flg = SEM_UNDO;

    sops[2].sem_op = 1;
    sops[2].sem_num = WRITE_TO_ORDER_BOARD_ID_SEMAPHORE;
    sops[2].sem_flg = SEM_UNDO;
    if ((semop(semaphoreSetId, sops,3)) == -1){
        perror("semop");
        exit(1);
    }

}

void upReadWriteOrder(){
    //algorithm:
    //1. Relase writer 
    auto *sops = new sembuf;

    sops->sem_op = -1;
    sops->sem_num = WRITE_TO_ORDER_BOARD_ID_SEMAPHORE;

    if ((semop(semaphoreSetId, sops,1)) == -1){
        perror("semop");
        exit(1);
    }

}


void downMenuBoardRead(){
    //algorithm:
    //1. makes sure that no writers.
    //2. up the number of readers.

    auto * sops = new sembuf[2];
    /* wait writer to finish */
    sops[0].sem_op = 0;
    sops[0].sem_num = WRITE_TO_MENU_BOARD_ID_SEMAPHORE;
    sops[0].sem_flg = SEM_UNDO;
    /* add a reader */
    sops[1].sem_op = 1;
    sops[1].sem_num = READ_FROM_MENU_BOARD_ID_SEMAPHORE;
    sops[1].sem_flg = SEM_UNDO;


    if ((semop(semaphoreSetId, sops, 2)) == -1){
        perror("semop");
        exit(1);
    }

}
void upMenuBoardRead(){
    //algorithm:
    //1. Release Reading

    auto * sops = new sembuf;
    sops->sem_op = -1; // release reader
    sops->sem_num = READ_FROM_MENU_BOARD_ID_SEMAPHORE;
    sops->sem_flg = SEM_UNDO;


    if ((semop(semaphoreSetId, sops, 1)) == -1){
        perror("semop");
        exit(1);
    }

}


void downWriteOrderBoard( ){
    //algorithm:
    //1. makes sure that no one writing currently
    //2. makes sure that no readers in Order Board
    //3. adding a new writer.

    auto * sops = new sembuf[3];
    /* wait for reades to finish */
    sops[0].sem_op = 0; //wait - to - zero, means you should wait till other readers finishing and decrement it's values.
    sops[0].sem_num = READ_FROM_ORDER_BOARD_ID_SEMAPHORE;
    sops[0].sem_flg = SEM_UNDO;

    /*wait for other writers*/
    sops[1].sem_op = 0; //wait - to - zero, means you should wait till other readers finishing and decrement it's values.
    sops[1].sem_num = WRITE_TO_ORDER_BOARD_ID_SEMAPHORE;
    sops[1].sem_flg = SEM_UNDO;

    /*add writer*/
    sops[2].sem_op = 1; //wait - to - zero, means you should wait till other readers finishing and decrement it's values.
    sops[2].sem_num = WRITE_TO_ORDER_BOARD_ID_SEMAPHORE;
    sops[2].sem_flg = SEM_UNDO;


    if ((semop(semaphoreSetId, sops, 3)) == -1){
        perror("semop:");
        exit(1);
    }


}



void upWriteOrderBoard(){
    auto * sops = new sembuf;

    /*remove writer*/
    sops->sem_op = -1; //wait - to - zero, means you should wait till other readers finishing and decrement it's values.
    sops->sem_num = WRITE_TO_ORDER_BOARD_ID_SEMAPHORE;
    sops->sem_flg = SEM_UNDO;


    if ((semop(semaphoreSetId, sops, 1)) == -1){
        perror("semop:");
        exit(1);
    }

}


bool PrevIsDone(pid_t pid,const int processIndex){
    bool flag=true;
    for(int j=0;j<Custc;j++){

        if((orderBoardPtr[j].Done == false) && (orderBoardPtr[j].CustomerId==processIndex)) {//means that at least there are one order that not done yet.
            //cout<<processIndex<<")Pending.. cant order"<<endl;
            flag= false; //ohaa, can't order because already order in pending..
            break;
        }

    }
    return flag; //Yes,they can order!!!
}
//lock the print
void pout(string str){
    downPrint();
    printf("%.3f ", getTimeDiff());
    cout<<str<<endl;
    upPrint();

}

void initSemaphores() {
    if ((semaphoreSetId = semget(IPC_PRIVATE, SEMAPHORES_COUNT, 0600 | IPC_CREAT)) == -1){
        perror("semget: semget failed");
        exit(1);
    }
    /*Set all semaphores in the set.*/
    semProporties.buf = new semid_ds();
    u_long length;


    if (semctl(semaphoreSetId, 0, IPC_STAT, semProporties) == -1) {
        perror("semctl:");
        exit(1);
    }

    length = semProporties.buf->sem_nsems;
    /*Set the semaphore set values.*/
    for (u_long i = 0; i < length; i++)
        semvals[i] = 0;

    semProporties.array = semvals;
    if (semctl(semaphoreSetId, 0, SETALL, semProporties) == -1){
        perror("semctl:");
        exit(1);
    }
}

void downPrint(){
    //cout<<"DEBUG: Process PID:"<<getpid()<<"trying to obtain the Lock."<<endl
    auto * sops = new sembuf[2];
    /* Wait for */
    sops[0].sem_op = 0;
    sops[0].sem_num = PRINT_ID_SEMAPHORE;
    sops[0].sem_flg = SEM_UNDO;
    /* set to */
    sops[1].sem_op = 1;
    sops[1].sem_num = PRINT_ID_SEMAPHORE;
    sops[1].sem_flg = SEM_UNDO;

    if ((semop(semaphoreSetId, sops, 2)) == -1){
        perror("semop:");
        exit(1);
    }

    //cout<<"DEBUG: Process PID:"<<getpid()<<", has successed Locked!"<<endl;
}
void upPrint(){
    //cout<<"DEBUG: Process PID:"<<getpid()<<"trying to release the Lock."<<endl;
    struct sembuf *semOpr=new sembuf;
    semOpr->sem_op = -1;
    semOpr->sem_num =PRINT_ID_SEMAPHORE;
    semOpr->sem_flg=SEM_UNDO;

    if((semop(semaphoreSetId,semOpr,1))==-1){
        perror("semop");
        exit(1);
    }
    //cout<<"DEBUG: Process PID:"<<getpid()<<", has successed Unlocked!"<<endl;
}


void initSharedMemory(){
    //here we allocate memory for both menutable and order table.
    key_t menuKey,orderKey,procKey;
    int sharedMenuMemoId,sharedOrderMemoId;
    size_t sharedMenuMemoSize,sharedOrderMemoIdSize;

    if((menuKey=ftok("./ex3",'M')==-1)){
        perror("ftok");
        exit(1);
    }
    if((orderKey=ftok("./ex3",'O')==-1)){
        perror("ftok");
        exit(1);
    }
    if((procKey=ftok("./ex3",'P')==-1)){
        perror("ftok");
        exit(1);
    }
    //connecting and possible create the segments
    sharedOrderMemoIdSize = Custc * sizeof(order);
    sharedMenuMemoSize = menItemC * sizeof(item);
    if((sharedMenuMemoId=shmget(menuKey,sharedMenuMemoSize, 0644 | IPC_CREAT))==-1){
        perror("shmget");
        exit(1);
    }
    if((sharedOrderMemoId=shmget(orderKey,sharedOrderMemoIdSize, 0644 | IPC_CREAT))==-1){
        perror("shmget");
        exit(1);
    }

    //shmat return a pointer to memory that allocated.
    itemMenuTablePtr = (item *)shmat(sharedMenuMemoId,nullptr,0);
    if(itemMenuTablePtr==nullptr)
        perror("nullptr");
    orderBoardPtr = (order *)shmat(sharedOrderMemoId,nullptr,0);
    if(orderBoardPtr==nullptr)
        perror("nullptr");


}

//starting the main timer.
void startTimer() {
    simulationTimer = chrono::steady_clock::now();
    cout << "================================" << endl;
    printf("%.3f", getTimeDiff());
    cout << " Main process ID " << getpid() << " start" << endl;
}

//get the difference (in seconds) between the started one , and the current time.
float getTimeDiff(){
    auto now = chrono::steady_clock::now();
    float diff = chrono::duration_cast<std::chrono::milliseconds>(now - simulationTimer).count();
    return diff / 1000;
}

//this function prints the given arguments
void printSimulationArgs(float simulationTime,int menItemC,int Custc,int Waitc){
    cout<<"=====Simulation arguments====="<<endl;
    cout<<"Simulation time: "<< simulationTime<<endl;
    cout<<"Menu items count: "<< menItemC<<endl;
    cout<<"Customers count: "<< Custc<<endl;
    cout<<"Waiters count: "<< Waitc<<endl;
    cout<<"=============================="<<endl;
}

//this function init the main menu
void initMenu(struct item  _item[],int itemCount){
    for(int i=0 ;i <itemCount;i++)
    {
        _item[i].id=i;
        strcpy(_item[i].Name,nameLists[i].c_str());
        _item[i].Price=PriceArr[i];
        _item[i].Orders=0; //default
    }
}

//printing the main menu, which is shared for all the the processes.
void printMenu(struct item _item[],int itemCount){
    downPrint();
    cout<<"==========Menu list=========="<<endl;
    cout<<"Id"<< " Name" <<"        Price" <<"  Orders"<<endl;
    for(int i=0;i<itemCount;i++)
    {
        cout<<_item[i].id<<"  ";
        cout<<_item[i].Name<<"\t";
        cout<<_item[i].Price<<"\t";
        cout<<_item[i].Orders;
        cout<<endl;
    }
    cout<<"=============================="<<endl;
    upPrint();
}


//this functions check's if the given argument contains numbers, returns true if, else return false.
bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}


//this function checks the validity of the given arguments (from shell)
void checkArgs(int argc,char *argv[]){
    if (argc !=5) {
        cout<<"ERROR: insuffecient amount of arguments."<<endl;
        cout<<"TIP: you must provide 4 arugments."<<endl;
        exit(1); //error
    }
    if (atoi(argv[1]) > 30 || atoi(argv[1]) <= 0)
    {
        cout<<"Total time running of simulation should be less than 30 seconds"<<endl;
        exit(1);
    }
    if (atoi(argv[2])  > 7 || atoi(argv[2]) < 5)
    {
        cout<<"Number of different dishes should be 5 to 7"<<endl;
        exit(1);
    }
    if (atoi(argv[4]) > 3 || atoi(argv[4]) <= 0)
    {
        cout<<"Number of waiters should be up to 3"<<endl;
        exit(1);
    }
    if (atoi(argv[3]) > 10 || atoi(argv[3]) <= 0)
    {
        cout<<"Number of customers should be up to 10"<<endl;
        exit(1);
    }

    for (int i=1;i<argc;i++)
    {
        if(is_number(string(argv[i]))==false){
            cout<<"ERROR: Invalid Arguments, please try again."<<endl;
            exit(1); //error
        }
    }
}

//this function init the variables that given as arguments from shell
void initArgs(float &simulationTime,int &menItemC,int &Custc,int &Waitc,char * argv[]){
    //Take the Arguments which has been sent
    simulationTime=atoi(argv[1]);  //Simulation Time.
    menItemC=atoi(argv[2]); //Menu Items Amount.
    Custc=atoi(argv[3]); //Customers Amount.
    Waitc=atoi(argv[4]); //Waiters Amount.
}
