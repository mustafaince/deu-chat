#include<stdio.h>
#include<string.h>    // for strlen
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h> // for inet_addr
#include<unistd.h>    // for write
#include<pthread.h>   // for threading, link with lpthread

//I used thread in client.c so you should use -lpthread while you are compiling.
//maximum room number can be created in same time is 10
//maximum customer number can be serviced in same time is 100
//there can be maximum 100 customer in same room. But total customer number in all rooms can be 100.
//if a customer wants to send message, he/she should write msg <message>. if he only write <message>, it is not valid command
//for quit command, you don't need write room name
//bir client herhangi bir anda sürekli kullanıcıdan bir mesaj gelip gelmediğini kontrol etmeli. Ve aynı zamanda kendisiyle aynı odadaki clientların mesaj yazıp yazmadığını kontrol etmeli.
//bu iş için paralel processing gerekli. O yüzden client ve server tarafında fazlandan bir thread daha kullandım. O threadler sadece client bir odaya girdiğinde üretilip odadan çıkınca
//yok ediliyor. Ve tek işleri diğer clientlerden mesaj gelip gelmediği.

char nicknames[100][30];//tüm clientların nickname ini tutmak için
int stateOfCustomers[100];// -2 customer(client) is absent, -1 müşteri var herhangi bir odada değil, 0-9 odada ve oda numarası bu değer
int stateOfRooms[10];//0 oda yok, 1 oda var ve public, 2 oda var ve private
int numberOfCustomerInRooms[10];//odalardaki toplam müşteri sayısını tutuyor
char roomNames[10][30];//var olan oda isimlerini tutuyor
char passwords[10][30];//odalar private ise onların şifrelerini tutuyor
char messages[10][100];//eğer odadaki customerlardan birisi mesaj atarsa bu mesajlar burda tutuluyor. Burası tüm clientlerin threadlerinin erişebileceği biyer olduğu için burdan mesajı alabiliyorlar
int controlMessages[10]; //odadan bir customer mesaj yazdığında bu değeri arttırıyor. odadaki diğer customerlerın threadleri de mesaj yazıldığını anlayıp mesajı cliente gönderiyor.
int whichRoom=-1; //üretilen ikinci threadin kendisi için çalıştığı clientın şuanda hangi odada olduğunu tutuyor.
int whichCustomer=-1; //ikinci threadın kendisi için çalıştığı clientin kim olduğunu tutuyor
int whichSocket=-1;//ikinci thread e clientla kurulan connection ın hangi socket ten yapıldığını söylemek için tutuluyor
int counter=0; // ekrana kaçıncı thread in yani kaçıncı clientle connection kurulduğunu bastırmak için tutuluyor.

int findRoomId(char room[]) //verilen bir oda isminin dizide nerde tutulduğunu buluyor
{
	int i;
	for(i=0; i<10; i++)
	{
		if(strcmp(room, roomNames[i])==0)
			return i;
	}
	return 0;
}

int findFreePlaceForRoom() //diziye yeni bir oda ismi ekleneceğinde boş bir yer buluyor
{
	int i;
	for(i=0; i<100; i++)
	{
		if(stateOfRooms[i] ==0)
			return i;
	}
	return 0;
}

int findFreePlaceForCustomer() //diziye yeni bir customer ismi ekleneceğinde boş bir yer buluyor
{
	int i;
	for(i=0; i<100; i++)
	{
		if(stateOfCustomers[i] ==-2)
			return i;
	}
	return 0;
}

int isThereThisRoom(char roomName[]) //verilen bir oda isminin olup olmadığını buluyor
{
	int i;
	for(i=0; i<10; i++)
	{
		if(strcmp(roomNames[i], roomName)==0)
			return 1; //if there is return 1
	}
	return 0;
}

correct(char a[]) // recv fonksiyonu daha önceden almış olduğu mesajları içinde hep tutuyor. Bu yüzden yeni bir mesaj almak gerektiğinde mesajın sonuna ? karakteri ekliyorum.
{				//mesajı aldıktan sonrada o karakteri \0 karakteriyle değiştiriyorum. böylece sadece elimde istediğim mesaj sonu null bir şekilde kalıyor
	int i;
	for(i=0; i<strlen(a); i++)
	{
		if(a[i]=='?')
		{
			a[i]= '\0';
			break;
		}
	}
}

clear(char a[]) //kullanılmış bir stringin içini boşaltmak için kullanıyorum
{
	
	int i;
	for(i=0; i<strlen(a); i++)
	{
		a[i]= '\0';
	}
}

int isThereThisCustomer(char customerName[]) //bir customerın olup olmadığını bulmak için kullanılıyor
{
	int i;
	for(i=0; i<100; i++)
	{
		if(strcmp(nicknames[i], customerName)==0)
			return 1; //if there is return 1
	}
	return 0;
}

parseCommand(char command[], char command1[], char command2[]) //clientdan alınan komutu parçalayıp ilk kısmı command1 e geriye kalan kısmı command2 ye atıyor
{
    int i, j=0, flag=0;
    	
    for(i=0; i<20; i++)
    {
    	command1[i]='\0';
    	command2[i]='\0';
    }
    for(i=0; command[i]!='\0'; i++)
    {
    	if(command[i]==' ' && flag==0)
    		flag=1;
    	else if(flag==0)
    		command1[i]=command[i];
    	else
    	{
    		command2[j]=command[i];
    		j++;
    	}
    }
}

messageManager(void *socket_desc) //a thread function for handling the messages coming from other user
{// bu thread sadece customer bir odadayken çalışıyor. Ve diğer kullanıcılar mesaj yazdığında client a o mesajı gönderiyor
	int socket = whichSocket;//ikinci thread e kendisi için çalıştığı clientla olan connection ın hangi socket den gerçekleştiğini söylemek için
	int roomId= whichRoom; //ikinci thread e kendisi için çalıştığı client ın hangi odada olduğunu göstermek için
	int customerId= whichCustomer;// ikinci thread e hangi customer için çalıştığını söylemek için
	int control=-1; 
	char messageToSend[2000];
	whichRoom=-1;
	whichCustomer=-1;
	whichSocket=-1;
	control= controlMessages[roomId];//aynı odadan birisinin mesaj yazıp yazmadığını tutmak için
	
	while(stateOfCustomers[customerId]>-1)//eğer customer odadan çıkarsa thread i bitir
	{
		while(control == controlMessages[roomId] && stateOfCustomers[customerId]>-1) {}//wait until a customer in same room with you writing a message
		if(stateOfCustomers[customerId]<0) break;
		control=controlMessages[roomId];
		clear(messageToSend);
		strcpy(messageToSend, messages[roomId]);
		strcat(messageToSend, "?");
    	send(socket, messageToSend , strlen(messageToSend) , 0);//mesajı client a gönder
	}

	pthread_exit(NULL);
}

void *connection_handler(void *socket_desc) //first thread function for handling the customers
{
	char nickname[30];
	char receivedMessage[2000], command1[20], command2[20];
    int socket = *((int*)socket_desc); //connection ın yapıldığı socket
    int validNickName=1, isExit=1, customerId=0, roomId=0, i, j;
    char messageToSend[2000];
    
    while(validNickName) //taking nickname from customer
    {
    	clear(receivedMessage);
    	recv(socket, receivedMessage , 2000 , 0);
    	correct(receivedMessage);
    	if(isThereThisCustomer(receivedMessage)==0)
    	{
    		validNickName=0;
    		strcpy(messageToSend, "ok?");
    		send(socket, messageToSend , strlen(messageToSend) , 0);
    		strcpy(nickname, receivedMessage);
    		customerId= findFreePlaceForCustomer();
    		strcpy(nicknames[customerId], nickname);
    		stateOfCustomers[customerId]= -1; // in lobby
    	}
    	else
    	{
    		strcpy(messageToSend, "error");
    		send(socket, messageToSend , strlen(messageToSend) , 0);
    	}
    }
    
    while(isExit)//customer exit demediği sürece devam et
    {
    	clear(receivedMessage);
    	clear(messageToSend);
    	clear(command1);
    	clear(command2);
    	recv(socket, receivedMessage , 2000 , 0);
    	correct(receivedMessage);
    	parseCommand(receivedMessage, command1, command2);

    	if(strcmp(command1, "list")== 0)
    	{
    		if(stateOfCustomers[customerId]!=-1)//customer odadayken list demişse hata ver
    		{
    			strcat(messageToSend, "lobby?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);
    		}
    		else//tüm odaları ve içindekileri bastır
    		{
    			for(i=0; i<10; i++)
    			{
    				if(stateOfRooms[i]!=0)//oda varsa
    				{
    					if(stateOfRooms[i]==1) //if it is not private(public)
    					{
    						strcat(messageToSend, roomNames[i]);//odanın adını mesaja ekle
    						strcat(messageToSend, ": ");
    						for(j=0; j<100; j++)
    						{
    							if(stateOfCustomers[j]==i)//eger customer bu odadaysa
    							{
    								strcat(messageToSend, nicknames[j]);//odadakilerin adını ekle
    								strcat(messageToSend, ", ");
    							}
    						}
    					}
    					else if(stateOfRooms[i]==2) //if it is private
    					{
    						strcat(messageToSend, roomNames[i]);//sadece odanın adını ekle
    						strcat(messageToSend, ": ");
    						strcat(messageToSend, "private");
    					}
    					strcat(messageToSend, "\n");
    				}
    			}
    			strcat(messageToSend, "?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);
    		}
    	}
    	
    	else if(strcmp(command1, "create")== 0)
    	{
    		if(stateOfCustomers[customerId]!= -1)//customer odadaysa hata ver
    		{
    			strcpy(messageToSend, "lobby?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);
    		}
    		else if(strlen(command2)>0 && isThereThisRoom(command2)== 0)//daha önce bu oda oluşmamışsa ve bir oda adı girilmişse
    		{
    			roomId= findFreePlaceForRoom();//yeni oda için dizide bosyer bul
    			whichRoom= roomId;//yeni thread için roomId sini yolla
    			whichCustomer= customerId;//yeni thread için customerId yi yolla
    			whichSocket=socket;// socket i yolla. yani en yukardaki değişkene at ki değeri, thread ordan ulaşsın
    			pthread_t msgManager;//thread tanımlama
    			if(pthread_create(&msgManager, NULL, messageManager, (void*)socket) < 0)//başka clientlardan gelen mesajları kontrol için yeni thread oluştur
        		{
            		puts("Could not create thread");
            		return 1;
        		}
        		
    			stateOfCustomers[customerId]= roomId; //this customer is on this room
    			stateOfRooms[roomId]= 1;//odanın durumunu public yap
    			numberOfCustomerInRooms[roomId]= 1;//odadaki customer sayısını arttır
    			strcpy(roomNames[roomId], command2);//odanın ismini kaydet
    			strcpy(messageToSend, "ok?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);//client a herşey yolunda mesajı yolla
    		}
    		else//bu oda zaten varsa hata ver
    		{
    			strcpy(messageToSend, "room?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);
    		}
    	}
    	
    	else if(strcmp(command1, "pcreate")== 0)//creat de yapılanlarla benzer işler
    	{
    		if(stateOfCustomers[customerId]!= -1)
    		{
    			strcpy(messageToSend, "lobby?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);
    		}
    		else if(strlen(command2)>0 && isThereThisRoom(command2)== 0)
    		{
    			roomId= findFreePlaceForRoom();
    			whichRoom= roomId;
    			whichCustomer= customerId;
    			whichSocket=socket;
    			pthread_t msgManager;
    			if(pthread_create(&msgManager, NULL, messageManager, (void*)socket) < 0)
        		{
            		puts("Could not create thread");
            		return 1;
        		}
    		
    			stateOfCustomers[customerId]= roomId; //this customer is on this room
    			stateOfRooms[roomId]= 2; //room is private
    			numberOfCustomerInRooms[roomId]= 1;
    			strcpy(roomNames[roomId], command2);
    			strcpy(messageToSend, "ok?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);
    			recv(socket, receivedMessage , 2000 , 0);
    			correct(receivedMessage);
    			strcpy(passwords[roomId], receivedMessage);//alınan şifreyi kaydet
    		}
    		else
    		{
    			strcpy(messageToSend, "room?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);
    		}
    	}
    	
    	else if(strcmp(command1, "enter")== 0)
    	{
    		if(stateOfCustomers[customerId]!= -1)
    		{
    			strcpy(messageToSend, "lobby?");
    			send(socket, messageToSend , strlen(messageToSend) , 0);
    		}
    		else
    		{
    			if(strlen(command2)>0 && isThereThisRoom(command2)==1)
    			{
    				roomId= findRoomId(command2);
    				whichRoom= roomId;
    				whichCustomer= customerId;
    				whichSocket=socket;
    				
    				if(stateOfRooms[roomId]==1) // if room is public
    				{
    					pthread_t msgManager;
    					if(pthread_create(&msgManager, NULL, messageManager,//odaya girdiği için yine ikinci thread i oluşturuyorum
                          	(void*)socket) < 0)
        				{
            				puts("Could not create thread");
            				return 1;
        				}
    					
    					strcpy(messageToSend, "ok?");
    					send(socket, messageToSend , strlen(messageToSend) , 0);
    					stateOfCustomers[customerId]= roomId;//customerin durumunu girdiği oda numarası yap
    					numberOfCustomerInRooms[roomId]= numberOfCustomerInRooms[roomId]+1;//odadaki kişi sayısını arttır
    				}
    				else //eğer oda private sa kullanıcıdan şifre istiyorum
    				{
    					strcpy(messageToSend, "private?");
    					send(socket, messageToSend , strlen(messageToSend) , 0);
    					recv(socket, receivedMessage , 2000 , 0);
    					correct(receivedMessage);
    					if(strcmp(receivedMessage, passwords[roomId])!=0)//şifre yanlışsa hata ver
    					{
    						strcpy(messageToSend, "password?");
    						send(socket, messageToSend , strlen(messageToSend) , 0);
    					}
    					else
    					{
    						pthread_t msgManager;
    						if(pthread_create(&msgManager, NULL, messageManager,
                          		(void*)socket) < 0)
        					{
            					puts("Could not create thread");
            					return 1;
        					}
    					
    						strcpy(messageToSend, "ok?");
    						send(socket, messageToSend , strlen(messageToSend) , 0);
    						stateOfCustomers[customerId]= roomId;
    						numberOfCustomerInRooms[roomId]= numberOfCustomerInRooms[roomId]+1;
    					}
    				}
    			}
    			else
    			{
    				strcpy(messageToSend, "room?");
    				send(socket, messageToSend , strlen(messageToSend) , 0);
    			}
    		}
    	}
    	
    	else if(strcmp(command1, "quit")== 0)
    	{
    		strcpy(messageToSend, "exit?");
    		send(socket, messageToSend , strlen(messageToSend) , 0);
    		stateOfCustomers[customerId]=-1;//customer ın lobby de olduğunu göster
    		numberOfCustomerInRooms[roomId]= numberOfCustomerInRooms[roomId]-1;//odadaki kişi sayısını bir azalt
    		if(numberOfCustomerInRooms[roomId]==0)//eğer bu çıkan odadaki sonuncu kişiyse
    		{
    			stateOfRooms[roomId]=0;//odayı kapat
    			clear(roomNames[roomId]);//oda adını
    			clear(passwords[roomId]);//ve şifreyi sil
    		}
    	}
    	
    	else if(strcmp(command1, "msg")== 0)
    	{
    		clear(messages[roomId]);//eski mesajı temizle
        	strcat(messages[roomId], nickname);//mesaj yazan kişinin adını mesaja ekle
        	strcat(messages[roomId], "> ");
        	strcat(messages[roomId], command2);//mesajı ekle
        	controlMessages[roomId]=controlMessages[roomId]+1;//yeni bir mesaj yazıldığını bildirmek için bu değeri arttır
    	}
    	
    	else if(strcmp(command1, "exit")== 0)
    	{
    		strcpy(messageToSend, "exit?");
    		send(socket, messageToSend , strlen(messageToSend) , 0);
    		isExit=0;//thread i bitir
    		clear(nicknames[customerId]);
    		if(stateOfCustomers[customerId]>-1)//eğer customer odadayken çıkmışsa
    			numberOfCustomerInRooms[roomId]= numberOfCustomerInRooms[roomId]-1;//odadaki kişi sayısını bir azalt
    		stateOfCustomers[customerId]=-2;//customeri diziden sil
    		
    		if(numberOfCustomerInRooms[roomId]==0) //bu customer çıktıktan sonra oda boşalacaksa
    		{
    			stateOfRooms[roomId]=0;//odayı sil
    			clear(roomNames[roomId]);//oda adını ve
    			clear(passwords[roomId]);//şifresini sil
    		}
    	}
    }
    
    free(socket_desc); //Free the socket pointer     
    pthread_exit(NULL);//client a hizmet veren thread i bitir
    return 0;
}

int main(int argc, char *argv[])
{
    int socket_desc, new_socket, c, *new_sock, i, j, k;
    struct sockaddr_in server, client;
    char *message;
    
    for(i=0; i<100; i++)
    {
    	stateOfCustomers[i]= -2; // -2 customer is absent, -1 in lobby, 0-9 in which room
    }
    
    for(i=0; i<10; i++)
    {
    	stateOfRooms[i]= 0; //state of room. 0: room is empty 1: not empty and public 2: private
    	numberOfCustomerInRooms[i]= 0; //number of customer in room
    }
    
    for(i=0; i<10; i++)
    {
    	controlMessages[i]=0;//odadan birisinin mesaj yazıp yazmadığını görmek için
    }
     
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        puts("Could not create socket");
        return 1;
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(3205);
     
    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
{
        puts("Binding failed");
        return 1;
    }
     
    listen(socket_desc, 3);
     
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while((new_socket = 
           accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;
         
        if(pthread_create(&sniffer_thread, NULL, connection_handler,//her client için yeni bir thread ata
                          (void*)new_sock) < 0)
        {
            puts("Could not create thread");
            return 1;
        }
		counter++;
        printf("Thread %d assigned\n", counter);//bu client in kaçıncı client olduğunu yazdır
    }
     
    return 0;
}
