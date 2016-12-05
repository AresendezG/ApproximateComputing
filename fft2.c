/*
The Purpose of this simple script is to develop a communication
interface between Raspberry PI and Spartan 3 FPGA using Pi's GPIO
Ports

The protocol will transfer an unsigned 8 bit number via serial communication and by
using other 3 signals to control the data flow.

Author: Esli Alejandro Resendez Guerrero

*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPi.h>
#include <assert.h>

/*Macros to operate protocol
 Since those are numeric constants, the use of macros instead of int data will reduce the 
 required memory and will speed up execution
*/

//Input Protocol
#define BUSCLK 3 //GPIO PIN 15
#define DATACLK 2 //GPIO PIN 13
#define READ_DATA 1 //GPIO PIN 12
#define DATA 0  //GPIO PIN 11

//Output Protocol 
#define RESET 4 
#define READ_EN 5 //GPIO 
#define READ_DONE 6 //GPIO 
#define DATAIN 7 //GPIO PIN 7

#define N_bits 8 //This is the width of the processor 

#define Num_Iterations 20 //Number of total iterations of the experiment
#define N_Points 2

FILE *numbers;
FILE *results;
FILE *bin_file;
FILE *log_file;
FILE *results_initial;

int randint(int n) { //Random Number Generator
  int r;
  if ((n - 1) == RAND_MAX) {
    return rand();
  } else {
    // Chop off all of the values that would cause skew...
    long end = RAND_MAX / n; // truncate skew
    assert (end > 0L);
    end *= n;
    
    while ((r = rand()) >= end);
    return r % n;
  }
} 


void SendNumber(int number, int NumOfBits){
int i=0;
int bitValue;

  digitalWrite(BUSCLK, HIGH); //Tranmission of a number starts 
  delay(10); //Delays 10 miliseconds
 for(i=0; i<NumOfBits; i++)
{	
  bitValue=((number >> i)&1);
  if(bitValue==0)
	  digitalWrite(DATA, LOW);
  else
	  digitalWrite(DATA, HIGH);
  delay(5); //Data is set
  digitalWrite(READ_DATA, HIGH); // 
  delay(5);
  digitalWrite(READ_DATA, LOW);
  delay(5);
  digitalWrite(DATACLK, HIGH);
  delay(5);
  digitalWrite(DATACLK, LOW);
  delay(5);
}
  digitalWrite(BUSCLK, LOW);
  delay(10);
}

int ReadNumber(int NumBits){
 int number=0;
 int i=0;
 int bit=0;
   digitalWrite(READ_EN, HIGH); //Establishes to read
   delay(10);
 for (i=0; i<NumBits; i++){
	bit=digitalRead(DATAIN);
    delay(10);
  if(bit==0)   
	  number &= ~(1 << i);
  else 
	  number |= 1 << i;
  //fprintf(bin, "%i,",bit);
  digitalWrite(READ_DONE, HIGH);
  delay(5);
  digitalWrite(READ_DONE, LOW);
  delay(5);
 }
 digitalWrite(READ_EN, LOW);
 delay(5);
 return number;
}

void BinaryWrite(int DSP_read, int exact, int numBits, int point){
	int i=0;
	int bitVal_DSP, bitVal_Exact;
	int WrongBits=0;
    fprintf(bin_file," ,");
	for (i=0; i<numBits; i++){ 
	bitVal_DSP=((DSP_read >> i)&1); //Consults bit to bit
    fprintf(bin_file, "%i,",bitVal_DSP);
	}
    fprintf(bin_file," ,");
	for (i=0; i<numBits; i++){  /*Result of the exact hardware*/
	bitVal_Exact=((exact >> i)&1); //Consults bit to bit
	fprintf(bin_file, "%i,",bitVal_Exact);
	}
    fprintf(bin_file, " ,");	
	for (i=0; i<numBits; i++){
	bitVal_Exact=((exact >> i)&1); //Consults bit to bit
	bitVal_DSP=((DSP_read >> i)&1); //Consults bit to bit
	if (bitVal_DSP != bitVal_Exact) {
		WrongBits++;
		fprintf(bin_file, "1,");
	}
	 else
		 fprintf(bin_file, "0,");
	}
	fprintf(bin_file, " ,%i, ,%i \n",WrongBits, point);

}

void reset(void){

 digitalWrite(RESET, HIGH);
 delay(15);
 digitalWrite(RESET, LOW);
}


int main(void){
	 		

 int A[Num_Iterations][N_Points];
 int DSP[Num_Iterations][N_Points];
 int exact_results[Num_Iterations][N_Points];
 //Handle the operations
 int i=0; //Iteration index
 int j=0;
 
 wiringPiSetup(); 

 /*Sending Protocol Pins*/
 pinMode(DATA, OUTPUT);
 pinMode(READ_DATA, OUTPUT);
 pinMode(DATACLK, OUTPUT);
 pinMode(BUSCLK, OUTPUT);
 /*Reading Protocol Pins*/
 pinMode(RESET, OUTPUT);
 pinMode(READ_EN, OUTPUT);
 pinMode(READ_DONE, OUTPUT);
 pinMode(DATAIN, INPUT);



srand(time(NULL)); //Seed for random numbers
 
 numbers = fopen("numbers_fft2.txt", "r");
 results = fopen("results_eh_2p.txt", "r"); 
 bin_file = fopen("binary_FFT2point.txt", "w");
 log_file = fopen("LogFile_fft2p.txt", "w");
 results_initial = fopen("results_dsp_fft2p.txt", "w");

if (numbers == NULL || bin_file == NULL || results == NULL || log_file == NULL) { //Avoid Execution if the file couldn't be created
    printf("Error opening file!\n");
    exit(1);
}

/*Loading Arrays from file*/

for (i = 0; i < Num_Iterations; i++)
 	for(j=0; j<N_Points; j++)
    {
        fscanf(numbers, "%i\n", &A[i][j]);
	fscanf(results, "%i\n", &exact_results[i][j]);
    }

  printf("Starting Simulation!! \n");
  fprintf(bin_file, "DSP Read, , , , , , , , ,Expected, , , , , , , , ,Missmatch, , , , , , , , , ,W, ,P,\n"); 
  fprintf(log_file, "Sent A0, SentB0,,Read X0, Read X1,,Exp X0, Exp X1 \n"); 
  for(j=0; j<Num_Iterations; j++) //Main Loop
    { 
	reset();
	printf("Iteration: %i \n",j);
     //	fprintf(log_file, "Sent A0, Sent B0, ,Read X0, Read X1, ,Expected X0, Expected X1 \n");
	for(i=0; i<N_Points; i++){ 
		SendNumber(A[j][i], N_bits);
		fprintf(log_file, "%i,",A[j][i]);
		delay(50);
	}
	fprintf(log_file, " ,");
	delay(50);	
       for(i=0; i<N_Points; i++){
		DSP[j][i]=ReadNumber(N_bits);
		fprintf(log_file, "%i,",DSP[j][i]);
		fprintf(results_initial, "%i\n",DSP[j][i]);
		delay(50);		
	}
        fprintf(log_file, " ,");
      for(i=0; i<N_Points; i++){
  	  fprintf(log_file, "%i,", exact_results[j][i]); //Printing Expected result in Log File
      }    
      fprintf(log_file,"\n");
      
   for(i=0; i<N_Points; i++){
	printf("Exact: %i - Read: %i \n", exact_results[j][i], DSP[j][i]);
	BinaryWrite(DSP[j][i], exact_results[j][i], N_bits, i);
	
	if(DSP[j][i]==exact_results[j][i]) 	
		printf(" match -> \n");
	else
		printf(" Missed!! **** \n");
  }
}

	fclose(bin_file);
	fclose(results_initial);
	fclose(log_file);
	fclose(results);
	fclose(numbers);


 return 0;

}


/*Error bits structure*/
