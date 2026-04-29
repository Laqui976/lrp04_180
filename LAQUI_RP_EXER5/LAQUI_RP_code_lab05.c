#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>   
#include <string.h>
#include <errno.h>

#define MAX_SLAVES 100

typedef struct {
    char ip[50];
    int port;
} Slave;

void mse_wma(int **x, int *w, int q, int n, int m, double *p){
    //equation 2
    int sum_w=0;
    for (int i =0; i<q; i++){//sum of weight
        sum_w+=w[i];
    }

    //calculate WMA 
    for (int j = 0; j < m; j++) {//only m columns 
        int mse=0;
         for (int i = q; i<n; i++){
            int wma=0;
            for (int k = i-q; k<=i-1; k++){

                int index =k-(i+q+1);
                wma += w[index] * x[k][j];
            }
            wma = wma/sum_w;
            //equation 1
            int temp = x[i][j] - wma;
            mse += temp*temp; //squared
        }
        //squareroot
        p[j] = sqrt(mse/(n-q));
    }

}

int **create_matrix(int n) {
     //create nxn matrix
    int **x = (int **)malloc(n *sizeof(int *));
    if(x == NULL){
        printf("Memory allocation failed\n");
        exit(1);
    } 
       for (int i = 0; i < n; i++) {
         x[i] = (int *)malloc(n * sizeof(int));
         if (!x[i]) {  
            printf("Memory allocation failed\n");
            exit(1);
         }
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
    return count-1;
}

//  safe send function
int send_all(int sock, void *buf, int len) {
    int total = 0;
    while (total < len) {
        int sent = send(sock, (char*)buf + total, len - total, 0);
        if (sent <= 0) return -1;
        total += sent;
    }
    return 0;
}

//  safe recv function
int recv_all(int sock, void *buf, int len) {
    int total = 0;
    while (total < len) {
        int rec = recv(sock, (char*)buf + total, len - total, 0);
        if (rec <= 0) return -1;
        total += rec;
    }
    return 0;
}

int lab05(){
     
    int s, port;
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


    //user input
    while (userInput){

        printf("Enter the value of n (matrix size): ");
        scanf("%i", &n);   
        userInput = false;
        if(n==0 || !n){
            printf("\nn cannot be null or 0\n");
            userInput = true;
        }
    }

        
        Slave slaves[MAX_SLAVES];
        int t = read_config(slaves); 
        
        if (t <= 0) { 
            printf("No slaves available\n");
            exit(1);
        }

        int **x = create_matrix(n);
        //divide to submatrices
        int rows_per_slave = n / t;
        
        //record time
        clock_t time_before, time_after;
        time_before = clock();

        double *temp_p = malloc(n * sizeof(double));
        double *global_p = calloc(n, sizeof(double));

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

            // resolve hostname OR IP
            struct hostent *he = gethostbyname(slaves[i].ip);
            if (he == NULL) {
                printf("Error resolving hostname %s\n", slaves[i].ip);
                close(socket_desc);
                continue;
            }
            memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
            
            int retries = 5; // limit retries
            while (connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("connect failed");
                printf("Waiting for slave %d...\n", i);
                if (--retries == 0) {
                    printf("Failed to connect to slave %d\n", i);
                    close(socket_desc);
                    continue;
                }
                sleep(1);
            }
            printf("Connected with server successfully\n"); 
            
            int start_row = i * rows_per_slave;
            int rows = (i == t - 1) ? (n - start_row) : rows_per_slave;

            
            // Send the metadata to server: 
            if(send_all(socket_desc, &rows, sizeof(int)) < 0){  
                printf("Unable to send message\n"); 
                return -1; 
            } 
            if(send_all(socket_desc, &n, sizeof(int)) < 0){  
                printf("Unable to send message\n"); 
                return -1; 
            } 
            
            //send submatrices
            for(int r = start_row; r < start_row + rows; r++){
                
                if(send_all(socket_desc, x[r], n*sizeof(int)) < 0){  
                    printf("Unable to send message\n"); 
                    return -1; 
                } 
                printf("Sent rows %d to %d to slave %d\n", start_row, start_row+rows-1, i);

                
            }  


            if (recv_all(socket_desc, temp_p, n * sizeof(double)) < 0) {
                printf("Failed to receive p\n");
                return -1;
            }

            for (int j = 0; j < n; j++) {
                global_p[j] += temp_p[j];
            }
            

            int terminate =-1;
            if(send_all(socket_desc, &terminate, sizeof(int)) < 0){  
                printf("Unable to send message\n"); 
                return -1; 
            }

            close(socket_desc);
        }
        printf("Final vector p:\n");
        for (int j = 0; j < n; j++) {
            printf("%lf ", global_p[j]);
        }
        printf("\n");
            
        time_after = clock();
        //get elapsed time
        float time_elapsed = (float)(time_after - time_before) / CLOCKS_PER_SEC;
        printf("Master Time Elapsed: %f\n", time_elapsed);
        
        //free vector and matrixes to avoid memory leak
        for (int i = 0; i < n; i++) {
                free(x[i]);
        }
        free(x);
        free(temp_p);
        free(global_p);

    }else if (s==1){
        ///if slave
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
        if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){  
            printf("Couldn't bind to the port\n"); 
            return -1; 
        } 
        
        printf("Done with binding\n"); 
        
        // Turn on the socket to listen for incoming connections: 
        if(listen(socket_desc, MAX_SLAVES) < 0){ 
            printf("Error while listening\n"); 
            return -1; 
        } 
        printf("\nListening for incoming connections.....\n"); 
        
        while (1) {
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, (socklen_t*)&client_size);

        if (client_sock < 0){
            printf("Can't accept\n");
            continue;
        }

        printf("Client connected at IP: %s and port: %i\n", 
        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));   

        
        int rows, n;
        

        // Receive client's message: 
        if (recv_all(client_sock, &rows, sizeof(int)) < 0){  
            printf("Couldn't receive\n"); 
            return -1; 
        } 
        if (recv_all(client_sock, &n, sizeof(int)) < 0){  
            printf("Couldn't receive\n"); 
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
            if (recv_all(client_sock, sub[i], n * sizeof(int)) < 0){  
                printf("Couldn't receive\n"); 
                return -1; 
            } 
        }
        
        printf("Received submatrix: %d x %d\n", rows, n);
            

        //bubble sort 5digit student number in descending order
        int snum[5] = {0, 6, 9, 9, 5};

        int len = sizeof(snum) / sizeof(snum[0]);

        for (int i = 0; i < len - 1; i++) {
            for (int j = 0; j < len - i - 1; j++) {
                if (snum[j+1] > snum[j]) {
                    int temp = snum[j];
                    snum[j] = snum[j+1];
                    snum[j+1] = temp;
                }
            }
        }

        //calculate q 
        int S1 = snum[0], S2 = snum[1];
        double temp = fmin((10*S1+S2)*n/100, 3*n/4); //Adapted from: https://www.geeksforgeeks.org/cpp/fmax-fmin-c/
        int q = (int) fmax(n / 2, temp);
        
        //create non-zero weight vector qx1
        int *w = (int *)malloc(q*sizeof(int));
        if(w == NULL){
            printf("Memory allocation failed\n");
            return 1;
        } 
        //assing random element
        for (int j = 0; j<q; j++){
                w[j] = (rand()%100) + 1 ;//random number between 1 - 100
            }
        
        //allocate vector p
        double *p = (double *)malloc(n*sizeof(double));
        if(p == NULL){
            printf("Memory allocation failed\n");
            return 1;
        } 
        clock_t time_before, time_after;
        time_before = clock();
          //assign value to p
        mse_wma(sub, w,q,n,n,p);

    
        time_after = clock();
        float time_elapsed = (float)(time_after - time_before) / CLOCKS_PER_SEC;
                    //print submatrix
        for (int i =0; i<rows; i++){
            for(int j =0; j<n; j++){
                printf("%d ", sub[i][j]);
            }
            printf("\n");
        }


          //send back submatrix

        if (send_all(client_sock, p, n * sizeof(double)) < 0) {
            printf("Failed to send vector p\n");
            return -1;
        }
        
        printf("Slave Time Elapsed: %f\n", time_elapsed);

        //free vector and matrixes to avoid memory leak
        for (int i = 0; i < rows; i++) {
            free(sub[i]);
        }
        free(sub);
        
        int terminate;
        if (recv_all(client_sock, &terminate, sizeof(int)) < 0){  
            printf("Couldn't receive\n"); 
            return -1; 
        } 

        if (terminate == -1) {
            printf("Termination signal received. Exiting slave...\n");
            close(client_sock);
            break;
        }
        } 
    }

    return 0;
}

int main(){
    srand(time(NULL)); //randomixer
    lab05();
}