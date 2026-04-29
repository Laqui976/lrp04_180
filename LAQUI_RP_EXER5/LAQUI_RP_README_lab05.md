#Laboratory Exercise 4: Distributing Parts of a Matrix over Sockets  

 - Distributed computation using TCP socket communication between a master and multiple slave processes, 
wherein a square matrix is divided into submatrices and sent to different instances.

#Author: **Rochelle Laqui** - CMSC 180 AB-4L
#Programming Language used : c
#Dependencies: stdio, stdlib, stdbool, time, unistd, arpa/inet, sched, unistd
#Filename: LAQUI_RP_code_lab04.c

##DESCRIPTION

    The programs performs the following:

    1. Ask and read user input for the value of 'n' (size of the square matrix), port (port number), and s (computer status: master or slave).
    2. If the instance is a master (s = 0):
        - Creates an n x n square matrix with random non-zero elements between 1 and 100.
        - Reads a configuration file (config.txt) containing the IP addresses and ports of all slave instances.
        - Determines the number of slaves t.
        - Divides the matrix into t submatrices of size (n/t) x n.
        - Records the system time time_before.
        - Sends each submatrix to its corresponding slave via TCP socket communication.
        - Receives an acknowledgment "ack" from each slave after successful transmission.
        - Waits until all slaves have responded.
        - Records the system time time_after.
    3.If the instance is a slave (s = 1):
        - Reads the configuration file (for consistency with the specification).
        - Opens a socket and listens on the assigned port.
        - Waits for the master to initiate communication.
        - Records the system time time_before.
        - Receives the assigned submatrix from the master.
        - Sends an acknowledgment "ack" back to the master after complete reception.
        - Records the system time time_after.
    4. Prints the calculated elapsed time.
    5. Print sent and received matrices for verification.

##HOW TO RUN:

    Follow the following steps to compile and run the program:

    1. Open the terminal in the directory the program is downloaded.
    2. Use gcc to compile the code. Type the command below in the terminal:
         
         'gcc -Wall -Wextra -g3 LAQUI_RP_code_lab04.c -o output'

        If running core affinity version run:

             'gcc -Wall -Wextra -g3 LAQUI_RP_withCoreAffinity_lab04.c -o output'

    3. Prepare a configuration file named config.txt in the same directory. Example:

                    127.0.0.1:5001
                    127.0.0.1:5002
                    127.0.0.1:5003

    4. Run the generated executable file by entering the command below in the terminal: 

        './output'

##AFTER COMPILING

   After successfully compiling and running the program:

    1. Open multiple terminals (one for each slave and one for the master).
    2. Run slave and master instances
    3. Enter any value for n
    4. Enter the assigned port from config.txt
    5. Enter if instance is slave or master (0 or 1).

##REFERENCES
    - Used AI to compile correctly with '-lm'
    - https://www.geeksforgeeks.org/c/tcp-server-client-implementation-in-c/4
    - https://www.geeksforgeeks.org/c/socket-programming-cc/