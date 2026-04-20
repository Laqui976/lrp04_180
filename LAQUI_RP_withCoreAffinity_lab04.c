#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sched.h>


#define MAX_SLAVES 100

typedef struct {
    char ip[50];
    int port;
} Slave;

int **create_matrix(int n) {
     //create nxn matrix
    int **x = (int **)malloc(n *sizeof(int *));
    if(x == NULL){
        printf("Memory allocation failed\n");
        exit(1);
    } 
    for (int i = 0; i < n; i++) {
         x[i] = (int *)malloc(n * sizeof(int));
    }
    
    //assign random int element in matrix
    for (int i = 0; i<n; i++){
        for (int j = 0; j<n; j++){
            x[i][j] = (rand()%100) + 1 ;//random number between 1 - 100
        }
    }

    return x;
}

int read_config(Slave slaves[]){
    FILE *fp = fopen("config.txt", "r");
    if (!fp) {
        printf("Error opening config.txt\n");
        exit(1);
    }

    int count = 0;
    while (fscanf(fp, "%[^:]:%d\n", slaves[count].ip, &slaves[count].port) != EOF) {
        count++;
    }

    fclose(fp);
    return count;
}

int lab04(){
     
    int s, port,core;
    bool userInput = true;

    while(userInput){
      
        printf("Enter port number: ");
        scanf("%i", &port);
        userInput=false;
        if(!port){
            printf("\nPort number can't be null\n");
            userInput=true;
        }else{
        printf("Are you a slave [1] or master [0]? ");
        scanf("%i", &s);
        userInput=false;
        if(s>1 ||s<0){
            printf("\nInput must be 1 or 0 only \n");
            userInput=true;
        }
        }
    }

    if(s==0){//if master
    int n=0;
    bool userInput = true;
    struct sockaddr_in server_addr; 
    int socket_desc;
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    //user input
    while (userInput)
    {
        printf("Enter the number of core to use: ");
         scanf("%i", &core);   
        userInput = false;
        if(core==0 || !core){
            printf("\ncore cannot be null or 0\n");
            userInput = true;
        }
        else if(core>num_cores){
            printf("\ncore cannot be greater than system number of core\n");
            userInput = true;
        }else{


        printf("Enter the value of n (matrix size): ");
        scanf("%i", &n);   
        userInput = false;
        if(n==0 || !n){
            printf("\nn cannot be null or 0\n");
            userInput = true;
        }
    }
        
    }

        cpu_set_t master_cpuset;
        CPU_ZERO(&master_cpuset);
        int reserved_core = num_cores - 1;

        CPU_SET(reserved_core, &master_cpuset);
        sched_setaffinity(0, sizeof(master_cpuset), &master_cpuset);

        printf("Master pinned to core %d (reserved)\n", reserved_core);
    

        Slave slaves[MAX_SLAVES];
        int t = read_config(slaves)-1;
        int **x = create_matrix(n);
        //divide to submatrices
        int rows_per_slave = n / t;
        //record time
        clock_t time_before, time_after;
        time_before = clock();
        for (int i =0; i<t; i++){
            // Create socket: 
            socket_desc = socket(AF_INET, SOCK_STREAM, 0); 
        
            if(socket_desc < 0){ 
                printf("Unable to create socket\n"); 
                return -1; 
            } 
            printf("Socket created successfully\n"); 

             // Set port and IP the same as server-side: 
            server_addr.sin_family = AF_INET; 
            server_addr.sin_port = htons(slaves[i].port); 
            server_addr.sin_addr.s_addr = inet_addr(slaves[i].ip); 
            
            // Send a connection request to the server, which is waiting at accept():           
           while (connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                printf("Waiting for slave %d...\n", i);
                sleep(1);
            }
            printf("Connected with server successfully\n"); 
            
            int start_row = i * rows_per_slave;
            int rows = (i == t - 1) ? (n - start_row) : rows_per_slave;

            // Send core ID to slave
           int usable_cores = core - 1;
           int core_id = i % usable_cores;

          if (send(socket_desc, &core_id, sizeof(int), 0) < 0) { perror("Unable to send core_id"); return -1; }
            
            // Send the metadata to server: 
            if(send(socket_desc, &rows, sizeof(int), 0) < 0){  printf("Unable to send message\n"); 
                return -1; 
            } 
            if(send(socket_desc, &n, sizeof(int), 0) < 0){  printf("Unable to send message\n"); 
                return -1; 
            } 
            
            //send submatrices
            for(int r = start_row; r < start_row + rows; r++){
                
                if(send(socket_desc, x[r], n*sizeof(int), 0) < 0){  printf("Unable to send message\n"); 
                return -1; 
                } 
                printf("Sent rows %d to %d to slave %d\n", start_row, start_row+rows-1, i);
                //print sent matrix
               
                for(int j =0; j<n; j++){
                        printf("%d ", x[r][j]);
                }
               printf("\n");
                
            }  
                char server_message[1000];
            if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
                printf("Error receiving ACK from slave %d\n", i);
                return -1;
            }

            printf("ACK from slave %d: %s\n", i, server_message);
            int terminate =-1;
            if(send(socket_desc, &terminate, sizeof(int), 0) < 0){  printf("Unable to send message\n"); 
                return -1; 
            }
            close(socket_desc);


        }
            
         time_after = clock();
            //get elapsed time
        float time_elapsed = (float)(time_after - time_before) / CLOCKS_PER_SEC;
        printf("Master Time Elapsed: %f\n", time_elapsed);
        
        //free vector and matrixes to avoid memory leak
        for (int i = 0; i < n; i++) {
                free(x[i]);
            }
        free(x);

    }else if (s==1){
        ///if-; slave
        int socket_desc, client_sock, client_size; 
        struct sockaddr_in server_addr, client_addr;    

        Slave slaves[MAX_SLAVES];
        read_config(slaves);
        
        socket_desc = socket(AF_INET, SOCK_STREAM, 0); 
  
        if(socket_desc < 0){ 
            printf("Error while creating socket\n"); 
            return -1; 
        } 
        printf("Socket created successfully\n"); 
        
            // Initialize the server address by the port and IP: 
            server_addr.sin_family = AF_INET; 
            server_addr.sin_port = htons(port); 
            server_addr.sin_addr.s_addr = INADDR_ANY; 
        
        int opt = 1;
        setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Bind the socket descriptor to the server address (the port and IP):  
        if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){  printf("Couldn't bind to the port\n"); 
            return -1; 
        } 
        
        printf("Done with binding\n"); 
        
        // Turn on the socket to listen for incoming connections: 
        if(listen(socket_desc, MAX_SLAVES) < 0){ 
        printf("Error while listening\n"); 
        return -1; 
        } 
        printf("\nListening for incoming connections.....\n"); 
        
        /* Store the client’s address and socket descriptor by accepting an  incoming connection. The server-side code stops and waits at accept()  until a client calls connect(). 
        */ 
        while (1) {   // infinite loop to handle multiple connections
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, (socklen_t*)&client_size);

        if (client_sock < 0){
            printf("Can't accept\n");
            continue;
        }

        printf("Client connected at IP: %s and port: %i\n", 
        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));   

        clock_t time_before, time_after;
        time_before = clock();
        int core_recv, rows, n;

            // Receive core ID from master first
        if (recv(client_sock, &core_recv, sizeof(int), 0) < 0) { 
            perror("recv"); 
            return -1; 
        }

        // Pin this slave to the received core
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);      // reset
        CPU_SET(core_recv, &cpuset); // set core received from master

        if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
            perror("sched_setaffinity");
        } else {
            printf("Slave pinned to core %d\n", core_recv);
        }

        // Receive client's message: 
        if (recv(client_sock, &rows, sizeof(int), 0) < 0){  printf("Couldn't receive\n"); 
               return -1; 
        } 
        if (recv(client_sock, &n, sizeof(int), 0) < 0){  printf("Couldn't receive\n"); 
               return -1; 
        } 
        // allocate submatrix
         int **sub = malloc(rows * sizeof(int*));                
        if (!sub) {
            printf("Memory allocation failed\n");
            return -1;
        }
                //receive submatrices
        for(int i = 0; i < rows; i++){
            sub[i] = malloc(n * sizeof(int));
            if (recv(client_sock, sub[i], n * sizeof(int), 0) < 0){  printf("Couldn't receive\n"); 
               return -1; 
            } 
            
        }
        
         printf("Received submatrix: %d x %d\n", rows, n);

         //print received matrix
         for (int i =0; i<rows; i++){
            for(int j =0; j<n; j++){
                printf("%d ", sub[i][j]);
            }
            printf("\n");
         }
          //send ack
         if(send(client_sock, "ack" , 4, 0) < 0){  printf("Unable to send message\n"); 
                return -1; 
         } 
    
       
        time_after = clock();
            //get elapsed time
        float time_elapsed = (float)(time_after - time_before) / CLOCKS_PER_SEC;
        printf("Slave Time Elapsed: %f\n", time_elapsed);
\
        //free vector and matrixes to avoid memory leak
        for (int i = 0; i < rows; i++) {
                free(sub[i]);
            }
        free(sub);
        
        int terminate;
        if (recv(client_sock, &terminate, sizeof(int), 0) < 0){  printf("Couldn't receive\n"); 
               return -1; 
        } 
          if (terminate == -1) {
                printf("Termination signal received. Exiting slave...\n");
                close(client_sock);
                break;   // exit loop
            }
        
      
        } 
         
    }
   

    return 0;
}

    

int main(){
    lab04();
}

