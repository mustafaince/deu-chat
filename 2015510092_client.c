#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h> // for inet_addr
#include<string.h>
#include<unistd.h>
#include<stdlib.h>

int socketc, validNickName=1;//connetion ın yapıldığı socket
struct sockaddr_in server;
char command[30], command1[20], command2[20];//komutların tutulduğu stringler
char receivedMessage[2000], nickname[30];//nickname i tut
int isCustomerExit=0;//exit komutu girildi mi
int flag=0;
int customerState=-1;//customer ın odada mı yoksa lobby demi olduğunu tutmak için

//bu fonksiyonlar server dakilerle aynı işi yapıyor
correct(char a[])
{
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

clear(char a[])
{
	int i;
	for(i=0; i<strlen(a); i++)
	{
		a[i]= '\0';
	}
}

parseCommand()
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

messageManager(void *socket_cus) //thread function for handling the messages coming from other customers
{
	while(isCustomerExit==0)//customer odadan çıkmışsa thread i bitir
	{
		clear(receivedMessage);
		recv(socketc, receivedMessage , 2000 , 0);
    	correct(receivedMessage);
    	
    	if(strcmp(receivedMessage, "exit")!=0)
    	{
    	    printf("%s\n", receivedMessage);
    	}
    	
    	flag=0;
	}

	isCustomerExit=0;
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    //Create socket
    socketc = socket(AF_INET, SOCK_STREAM, 0);
    if (socketc == -1)
    {
        puts("Could not create socket");
        return 1;
    }
         
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(3205);
    //Connect to remote server
    if (connect(socketc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
         puts("Connection error");
         return 1;
    }
    
    printf("Welcome to DEUchat\n");
    
    while(validNickName)//nickname gir
    {
    	printf("Enter your nickname> ");
    	clear(command);
    	gets(command);
    	strcat(command, "?");
    	send(socketc , command , strlen(command), 0);
    	clear(receivedMessage);
    	correct(command);
    	recv(socketc, receivedMessage , 2000 , 0);
    	correct(receivedMessage);
    	if(strcmp(receivedMessage, "ok")==0)
    	{
    		validNickName=0;
    		strcpy(nickname, command);
    		printf("Now you are in lobby\n");
    	}
    	else
    		printf("This nickname already exists\n");
    }
    
    while(strcmp(command1, "exit")!= 0)
    {
    	clear(command);
    	clear(command1);
    	clear(command2);
    	clear(receivedMessage);
    	while(flag==1) {}//diğer client lar mesaj yazdığında, diğer thread o mesajı basana kadar bekleyip sonra enter command> demek için.(consolda güzel görünmesi için)
    	printf("Enter command> ");
    	gets(command);
    	parseCommand();
    	strcat(command, "?");
    	
    	if(strcmp(command1, "list")== 0)
    	{	
    		if(customerState==0)//customer odadaysa hata ver
    		{
    			printf("you should exit from this room\n");
    		}
    		else
    		{
    			send(socketc , command , strlen(command), 0);
    			recv(socketc, receivedMessage , 2000 , 0);
    			correct(receivedMessage);
    			printf("%s\n", receivedMessage);
    		}
    	}
    	
    	else if(strcmp(command1, "create")== 0)
    	{
    		if(customerState==0)//customer odadaysa hata ver
    		{
    			printf("you should exit from this room\n");
    		}
    		else
    		{
    			send(socketc , command , strlen(command), 0);
    			recv(socketc, receivedMessage , 2000 , 0);
    			correct(receivedMessage);
    		
    			if(strcmp(receivedMessage, "room")==0)
    			{
    				printf("this room already exists\n");
    			}
    			else
    			{	
    				customerState=0;
    				pthread_t msgManager;
    				if(pthread_create(&msgManager, NULL, messageManager, (void*)socketc) < 0)//diğer clientlerden gelen mesajı kontrol etmek için yeni thread üret
        			{
            			puts("Could not create thread");
            			return 1;
        			}
    			}
    		}
    		
    	}
    	
    	else if(strcmp(command1, "pcreate")== 0)
    	{
    		if(customerState==0)//customer odadaysa hata ver
    		{
    			printf("you should exit from this room\n");
    		}
    		else
    		{
    			send(socketc , command , strlen(command), 0);
    			recv(socketc, receivedMessage , 2000 , 0);
    			correct(receivedMessage);
    		
    			if(strcmp(receivedMessage, "ok")==0)
    			{	
    				customerState=0;
    				pthread_t msgManager;
    				if(pthread_create(&msgManager, NULL, messageManager, (void*)socketc) < 0)//diğer clientlerden gelen mesajı kontrol etmek için yeni thread üret
        			{
            			puts("Could not create thread");
            			return 1;
        			}
        		
    				printf("enter password for room> \n");
    				clear(command);
    				gets(command);
    				strcat(command, "?");
    				send(socketc , command , strlen(command), 0);
    			}
    			else if(strcmp(receivedMessage, "lobby")==0)
    			{
    				printf("you should exit from this room\n");
    			}
    			else if(strcmp(receivedMessage, "room")==0)
    			{
    				printf("this room already exists\n");
    			}
    		}
    		
    	}
    	
    	else if(strcmp(command1, "enter")== 0)
    	{
    		if(customerState==0)//customer odadaysa hata ver
    		{
    			printf("you should exit from this room\n");
    		}
    		else
    		{
    			send(socketc , command , strlen(command), 0);
    			recv(socketc, receivedMessage , 2000 , 0);
    			correct(receivedMessage);
    		
    			if(strcmp(receivedMessage, "lobby")==0)
    			{
    				printf("you should exit from this room\n");
    			}
    			else if(strcmp(receivedMessage, "room")==0)
    			{
    				printf("this room does not exist\n");
    			}
    			else if(strcmp(receivedMessage, "private")==0)
    			{
    				printf("enter password> ");
    				clear(command);
    				gets(command);
    				strcat(command, "?");
    				send(socketc , command , strlen(command), 0);
    				recv(socketc, receivedMessage , 2000 , 0);
    				correct(receivedMessage);
    				if(strcmp(receivedMessage, "password")==0)
    				{
    					printf("password is wrong\n");
    				}
    				else
    				{
    					customerState=0;
    					pthread_t msgManager;
    					if(pthread_create(&msgManager, NULL, messageManager, (void*)socketc) < 0)//diğer clientlerden gelen mesajı kontrol etmek için yeni thread üret
        				{
            				puts("Could not create thread");
            				return 1;
        				}
    				}
    			}
    			else
    			{
    				customerState=0;
    				pthread_t msgManager;
    				if(pthread_create(&msgManager, NULL, messageManager, (void*)socketc) < 0)//diğer clientlerden gelen mesajı kontrol etmek için yeni thread üret
        			{
            			puts("Could not create thread");
            			return 1;
        			}
    			}
    		}
    	}
    	
    	else if(strcmp(command1, "quit")== 0)
    	{
    		if(customerState==0)
    		{
    			send(socketc , command , strlen(command), 0);
    			customerState=-1;
    			isCustomerExit=1;
    			printf("Now you are in lobby\n");
    		}
    		else//customer odada değilse
    		{
    			printf("You should enter a room\n");
    		}
    	}
    	
    	else if(strcmp(command1, "msg")== 0)
    	{
    		if(customerState==0)
    		{
    			send(socketc , command , strlen(command), 0);
    			flag=1;
    		}
    		else
    		{
    			printf("You should enter a room\n");
    		}
    	}
    	
    	else if(strcmp(command1, "whoami")== 0)
    	{
    		printf("you are: %s\n", nickname);
    	}
    	
    	else if(strcmp(command1, "exit")== 0)
    	{
    		isCustomerExit=1;//thread i bitir döngüden çık
    		send(socketc , command , strlen(command), 0);
    		printf("thank you for using DEUchat, see you!\n\n");
    	}
    	
    	else
    	{
    		printf("this command is not valid\n");
    	}
    }
	
    close(socketc);
    return 0;
}

