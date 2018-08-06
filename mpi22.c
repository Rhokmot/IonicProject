//mpicc -o mpi22 mpi22.c -lrt

#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <malloc.h>
#include <mpi.h>
#include <unistd.h>

#define n_initials (26 * 26)

#define n_dates (31 * 12)

#define n_initials_and_dates  n_initials * n_dates

#define SALT "$6$KB$"

struct timespec start, finish;

int time_difference(struct timespec *start, struct timespec *finish, 
                              long long int *difference) {
  long long int ds =  finish->tv_sec - start->tv_sec; 
  long long int dn =  finish->tv_nsec - start->tv_nsec; 

  if(dn < 0 ) {
    ds--;
    dn += 1000000000; 
  } 
  *difference = ds * 1000000000 + dn;
  return !(*difference > 0);
}

void two_letter_initial(int index, char *buffer){

  char alphabet[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n',

                     'o','p','q','r','s','t','u','v','w','x','y','z'};

  int first, second;

  first = index / 26;

  second = index % 26;

  sprintf(buffer, "%c%c", alphabet[first], alphabet[second]);

}

void four_digit_date(int index, char *buffer){

  int day, month;

  month = (index / 12) + 1;

  day = (index % 12) + 1;

  sprintf(buffer, "%02d%02d", day, month);

}



void combine(int index, char *buffer) {

  char initials[3];

  char date[5];

  int i = index / n_dates; 

  int d = index % n_dates; 

  two_letter_initial(i, initials);

  four_digit_date(d, date);

  sprintf(buffer, "%s%s", initials, date);

}



int main() {
  int size, rank;
  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // printf("I am %d of %d\n", rank, size);

  if(rank ==0){   
    clock_gettime(CLOCK_MONOTONIC, &start);

    char combined[7];
    MPI_Status status;
    MPI_Request request[65]; // Assume there are at most 65 processes.
                               // Use malloc for more.
    int done = 0;
    int i;

    while(!done){
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &done, &status);

      if(done==1) {
        MPI_Recv(&combined, 7, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, 
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);        

        // printf("received solution (%ld, %ld, %ld, %ld) from process %d\n", 
        //        solution[0], solution[1], solution[2], solution[3],
        //        status.MPI_SOURCE);

        for(i=1;i<size;i++){
          if(i!=status.MPI_SOURCE){
            // printf("sending done to process %d\n", i);
            MPI_Isend(&done, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &request[i]);
          }   
        }
      }          
    }
  }else{
    long c;
    int done = 0;
    MPI_Status status;
    MPI_Request request;
    char combined[7];
    for(c=rank-1; c<n_initials_and_dates && !done; c+=size-1) {
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &done, 
                 &status);
      if(done){
        // printf("process %d has received done signal\n", rank);
        break;
      }

      combine(c, combined);

      if(strcmp(crypt(combined, SALT), "$6$KB$LA65ib37/D5XNZV2WiSzIHnjAJOKKv/qYDlsd9LUU4Ur6zXp/0zl8LqntstpNiiHJkspjoyv68lHMoVUUjY6s1") == 0){
        MPI_Send(&combined, 7, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        done = 1;
      }

    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize(); 
  if(rank == 0){
    long long int time_elapsed;
    clock_gettime(CLOCK_MONOTONIC, &finish);
    time_difference(&start, &finish, &time_elapsed);
    printf("%0.9lf\n", (time_elapsed/1.0e9)); 
  }
  return 0;
}




