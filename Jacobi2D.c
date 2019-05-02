/*
Compilação: mpicc jacobi2d.c -o jacobi -fopenmp
Run: mpirun -np 10 jacobi
*/


#include "mpi.h"
#include <omp.h>
#include <stdio.h>
#include <math.h>
#define MAXSIZE 1000
#define MEDSIZE 100
#define LOWSIZE 10

int main(int argc, char **argv)
{
	int myid, numprocs;
	float matrix[MEDSIZE][MEDSIZE];
	float minMatrix[LOWSIZE][MEDSIZE];
	float calcMatrix[LOWSIZE+2][MEDSIZE];
	float borderSuperior[MEDSIZE], borderInferior[MEDSIZE];
	int i, j, lim, lim2, contador = 0;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	omp_set_num_threads(2);
	

	//Initialize a matrix with the first and last column with one and the rest 0
	// Each process initialize a matrix part
		for(i=0; i<MEDSIZE; i++){
			for(j=0; j<MEDSIZE; j++){
				if(j==0 || j== (MEDSIZE-1 )){
					minMatrix[i][j] = 1;
				}
				else{
					minMatrix[i][j] = 0;
				}
			}
		}

		
	while(contador < MAXSIZE){
	

	// Send and receive the superior matrix border to the previous process and the inferior border to the next process 
	// The if sequence create a treatment for the first and last process
	if(myid != 0){
		MPI_Send(&minMatrix, 100, MPI_FLOAT, myid-1, myid, MPI_COMM_WORLD); //send da borda superior-inferior
	}
	if(myid < 9){
		MPI_Recv(borderInferior, 100, MPI_FLOAT, myid+1, myid+1, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // recv da borda superior-inferior
		MPI_Send(&minMatrix[9], 100, MPI_FLOAT, myid+1, myid, MPI_COMM_WORLD); // send da borda inferior-superior
	}
	if(myid != 0){
			MPI_Recv(borderSuperior, 100, MPI_FLOAT, myid-1, myid-1, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //recv da borda inferior-superior
	}

	//Join the matrix border generatinga new matrix to be used to re-calculate
	// The if sequence create a treatment for the first and last process
	if(myid != 0){
		for(j=0; j<MEDSIZE; j++){
			calcMatrix[0][j] = borderSuperior[j];
		}
	}
	
	for(i=0; i<LOWSIZE; i++){
		for(j=0; j<MEDSIZE; j++){
			calcMatrix[i+1][j] = minMatrix[i][j];
		}
	}

	if(myid !=9){
		for(j=0; j<MEDSIZE; j++){
			calcMatrix[11][j] = borderInferior[j];
		}
	}
	

	// Jump the extra superior and inferior borders
	if(myid == 0){
		lim2 = 2;
		lim = 1;
	}
	else if(myid == 9){
		lim2=1;
		lim = 0;
	}
	else{
		lim2 =1;
		lim = 1;
	}
	

//Each treat will get one line and calculate the value in parralell
#pragma omp parallel for  private (i, j) schedule (dynamic, 1)
	for(i = lim2; i<(LOWSIZE+lim);i++){
		for(j =1;j<(MEDSIZE-1);j++){
			minMatrix[i-1][j]= 0.25 * (calcMatrix[i][j-1] + calcMatrix[i][j+1] +calcMatrix[i-1][j] +calcMatrix[i+1][j] );
		}
	}
	
	contador++;

	}

//The process 0 receive the final matrix from each process and combine it in the rite order
	MPI_Gather(minMatrix, (MEDSIZE*LOWSIZE), MPI_INT, matrix, (MEDSIZE*LOWSIZE), MPI_FLOAT, 0, MPI_COMM_WORLD);


//The process 0 print the final matrix on screen and generate a file
	if(myid == 0){
	
	FILE *file;
	file =fopen("jacobi2dResult2.txt", "w");
	
	for(i=0; i<MEDSIZE; i++){
		fprintf(file, "|");
		printf("|");
		for(j=0; j<MEDSIZE; j++){
			fprintf(file, "%.2f|", matrix[i][j]);
			printf("%.2f|", matrix[i][j]);
		}
		fprintf(file, " \n");
		printf(" \n");
	}
	
	fclose(file);
	}

	MPI_Finalize();
	return 0;

}
/* Final Considerations:

The development of this system had as focus to save the amount of data transfered between process and the amount of comunication between process.

The first and the last process will sent and received one matrix's line per iteration, the others process will sent and received 2 lines
Each process will communicate with their neighbors once per iteration, then total transfer per iteration will be:
(number of process * 2) - 2 => Resulting in 18 connection per iteration.

Each connection sends one line of the matrix, in this case 100 numbers.

At the end of all 1000 iteration, 1,800,000 numbers (100*1000*18) will be sent in 18,000 connections (18*1000). 
The first and the last process will send and receive 100,00 numbers and the others process 200.000 numbers  

The system has to break points
1-> At the moemnt to send and receive, because each process only can continues after theirs neighbors finish the task
2-> At the end to join all the result

To achieve the best performance, each machine should have similar configuration to avoid idles machine.
*/
