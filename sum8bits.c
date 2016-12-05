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

#define Num_Iterations 20000 //Number of total iterations of the experiment

int Error_Place [N_bits]; //Counts how many errors in each bit overall
int Error [N_bits+1][N_bits+1]; //Errors [How_Many_Wrong] [Which_Position]

/*Errors array is used to count how many bits were wrong and in which position

 Errors[How_Many_Wrong][Which_Position]
 
 In example, if there is only 1 wrong bit in the reading at the 4th bit, 
 Errors[1][4]++ will count +1. 

 */
 
/* File declarations ***********************/
FILE *bin_file;
FILE *inform_file;
FILE *results_file;


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

void reset(void){

 digitalWrite(RESET, HIGH);
 delay(15);
 digitalWrite(RESET, LOW);
}

/*Function used to parse the information and facilitate the data mining

*/

void BinaryConversion(int DSP_read, int Exact, int numBits){
	int i=0;
	int bitVal_DSP, bitVal_Exact;
	int WrongBits=0;
	for (i=0; i<numBits; i++){ 
	bitVal_DSP=((DSP_read >> i)&1); //Consults bit to bit
    bitVal_Exact=((Exact >> i)&1);
		if (bitVal_DSP != bitVal_Exact){
			WrongBits++;
                        Error_Place[i]++; //Counts a bit error in the given place
                 }
	fprintf(bin_file, "%i,",bitVal_DSP);	// Writes it down in the binary analysis file
	}
	fprintf(bin_file, ","); //Space to separate DSP results from Exact Results
	for (i=0; i<numBits; i++){
    bitVal_Exact=((Exact >> i)&1);
	fprintf(bin_file, "%i,", bitVal_Exact);
	}
	
	fprintf(bin_file, " ,");
	/*Count how many wrong bits at each position*/
	//if(WrongBits>0) //Only if there are more than 1 wrong bit
		for (i=0; i<N_bits; i++){ //Checks which of the bits doesn't match
		  if (((Exact >> i)&1) != ((DSP_read >> i)&1)) {
		   Error[WrongBits][i]++;  //Errors[How Many?] [At which position?]  /*Since we have how many bits were wrong, we can spot in which positions */
		   fprintf(bin_file, "1,");
		  }
		  else
		    fprintf(bin_file,"0,");
		}
	fprintf(bin_file, " ,%i, ,", WrongBits);

}

int main(void){

 int A, B, DSP;  //Handle the operations
 int i=0; //Iteration index
 int j=0;
 char m;
 int match=0;
 int miss=0;
 
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


 /*File management*/

   results_file = fopen("results.csv", "w");
	   bin_file = fopen("binary.csv", "w");
    inform_file = fopen("inform.csv","w"); 
   
if (results_file == NULL || bin_file == NULL || inform_file == NULL) { //Avoid Execution if the file couldn't be created
    printf("Error opening file!\n");
    exit(1);
}

srand(time(NULL)); //Seed for random numbers
fprintf(bin_file, "DSP Binary Read,Binary Exact,Mismatch bits,Wrong Bits,Match \n");
fprintf(results_file, "A, B, Result, DSP Read, Abs A Dist, Match \n");
printf("Starting Simulation!! \n");

//Clearing Error place counters
for(i=0; i<N_bits; i++)
  Error_Place[i]=0;

for(i=0; i<N_bits; i++)
	for(j=0;j<N_bits; j++)
		Error[i][j]=0;

for(i=0; i<Num_Iterations; i++) //Main Loop
{
	A = randint(255);
	B = randint(255);
	reset();
	printf("Iteration: %i \n",i);
	SendNumber(A, N_bits);
	delay(50);
	SendNumber(B, N_bits);
	delay(50);
	DSP=ReadNumber((N_bits+1));
	delay(100);
	printf("Operation: %i + %i \n", A, B);
	printf("Exact: %i - DSP: %i ",(A+B), DSP);
	if(DSP==(A+B)) 	{
		match++; 
		printf(" match \n");
		m = 'y';
	}
	else {
		miss++;
		printf(" Missed!! ** \n");
		m = 'n';
	}
	fprintf(results_file, "%i,%i,%i,%i,%i,%c \n", A, B,(A+B), DSP, abs(DSP-(A+B)), m); //Printing to results
	BinaryConversion(DSP, (A+B), (N_bits+1));
    fprintf(bin_file, "%c, \n",m);
}

/*Printing Errors inform*/
fprintf(inform_file, "Error frequency overall, \n");
for (i=0; i<N_bits; i++)
  fprintf(inform_file, "Bit Error in B%i:, %i, \n",i,Error_Place[i]); 

fprintf(inform_file,"\n Wrong Bit Counter,");
for(i=0; i<N_bits; i++)
	fprintf(inform_file, "%i,",i);

fprintf(inform_file, "\n");

for (i=1; i<N_bits+1; i++) {
	fprintf(inform_file, "%i Error(es) ,",i);
	for(j=0; j<N_bits; j++)
		fprintf(inform_file, "%i,",Error[i][j]);
	fprintf(inform_file, "\n");
}

fclose(results_file); //Closing the results file
fclose(bin_file);
fclose(inform_file);

 return 0;

}


/*Error bits structure*/
