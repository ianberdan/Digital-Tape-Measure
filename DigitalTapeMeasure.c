//////////////////////////////////////////////////////////////////////////////
// Written By: Ian Berdan and Mason Rakowicz
// Class: EE347
// Professor: Dr. Fourney
// Data: 4/29/21
// Description:
//		Digital Tape Measure. Utilizes an analog optical distance sensor to
//		measure any distance from 8 to 42cm. The ADC data is gathered through
//		the ADC sample sequencer interrupt which allows faster sampling.
//		The calculated distance is then communicated with an LCD using I2C.
	
#include "TM4C123GH6PM.h"

//Function Prototypes
void delay(volatile unsigned int delay);
void startSample(void);
void delayms(int n);
void measureDist(void);
void ADCinit(void);
void i2cInit(void);
void i2cSend(unsigned char Data);
void lcdInit(void);
void lcdClear(void);
void sendCmd(unsigned char command);
void sendData(unsigned char data);
void displayDist(void);
void truncateDist(void);
void wholeLottaSpaces(int n);
void finalSetup(void);

// variables
int adcData, addr;
float distance;
float newDist;
float useDist;

//datapoints
int status = 1;
int dataPointsX[24] = {2923,2675,2325,2060,1834,
1661,1537,1422,1309,1233,1140,1096,1023,998,952,
932,901,851,828,808,783,760,785,761};
float dataPointsY[24] = {0.125,0.1,0.083333333,
0.071428571,0.0625,0.055555556,0.05,
0.045454545,0.041666667,0.038461538,0.035714286,
0.033333333,0.03125,0.029411765,0.027777778,
0.026315789,0.025,0.023809524,0.022727273,0.02173913,
0.020833333,0.02,0.019230769,0.018518519};

float slope[24];
float useSlope;
float dist1, dist2, dist3, avgDist;
float useSlope;
int modDist[4];
float temp;
int temp2;
int start = 1;

int main(void)
{
	ADCinit();
	i2cInit();
	lcdInit();
	lcdClear();
	finalSetup();
	while(1){
		startSample();
		measureDist();
		displayDist();
		delayms(1000);
	}
}

// Displays the distance to the LCD
void displayDist(){
	lcdClear();
	if(start == 0){
		wholeLottaSpaces(22);
	}
	else{
		start = 0;
	}
	sendData('D');
	sendData('i');
	sendData('s');
	sendData('t');
	sendData('a');
	sendData('n');
	sendData('c');
	sendData('e');
	sendData(':');
	sendData(' ');
	
	truncateDist();
	sendData(0x30 + modDist[3]);
	sendData(0x30 + modDist[2]);
	sendData('.');
	sendData(0x30 + modDist[1]);
	sendData(0x30 + modDist[0]);
	sendData('c');
	sendData('m');
	sendData(' ');
}

void wholeLottaSpaces(int n){
	int i;
	for(i=0;i<=n;i++){
		sendData(' ');
	}
}

//multiplies the distance to mod within 100th degree of accuracy
void truncateDist(){
	int i;
	temp = avgDist * 10000;
	temp2 = temp/100;
	for(i=0;i<4;i++){
		modDist[i] = temp2 % 10;
		temp2 /= 10;
	}
}

//sends a 4-bit command three times for the LCD
void sendCmd(unsigned char cmd)
{
	i2cSend(cmd + 8);
	delay(1);
	i2cSend(cmd + 12);
	delay(1);
	i2cSend(cmd + 8);
	delay(4);
}

// calls I2C send three times for each nibble. Needed for display
void sendData(unsigned char data)
{
	//Splits the data into the lower and upper nibbles
	//Upper nibble is sent first
	int lower = (data<<4) & 0xf0;
	int upper = data & 0xf0;
	
	i2cSend(upper + 9);
	delay(1);
	i2cSend(upper + 13);
	delay(1);
	i2cSend(upper + 9);
	delay(4);
	
	i2cSend(lower + 9);
	delay(1);
	i2cSend(lower + 13);
	delay(1);
	i2cSend(lower + 9);
	delay(4);
}

void lcdInit(void)
{
	delay(1500);
	
	//Config LCD function
	sendCmd(0x20);
	sendCmd(0x20);
	sendCmd(0x00);
	sendCmd(0x00);
	sendCmd(0xE0);//E
	sendCmd(0x00);
	sendCmd(0x60);
	sendCmd(0x00);
	/////
	sendCmd(0x00);
	sendCmd(0x8);
}

void lcdClear(void)
{
	sendCmd(0x10);
	sendCmd(0x00);
	delay(170);
	sendCmd(0x20);
	sendCmd(0x00);
	delay(170);
}


void delay(volatile unsigned int delay)
{
	volatile unsigned int i, j;
	for(i = 0; i < delay; i++)
	{
		for(j = 0; j < 12; j++)
		{}
	}
}

void i2cInit(void)
{
	SYSCTL->RCGCGPIO |= 0x10;
	SYSCTL->RCGCI2C |= 0x04;
	GPIOE->DEN |= 0x030;
	GPIOE->AFSEL |= 0x030;
	GPIOE->PCTL |= 0x0330000;
	GPIOE->ODR |= 0x020;
	
	I2C2->MCR = 0x10;
	I2C2->MTPR = 0x07;
}

// Sends an 8-bit package to I2C device
void i2cSend(unsigned char data)
{
	unsigned char slaveAddr = 0x3F;
	while(I2C2->MCS & status) {}
	
	I2C2->MSA = (slaveAddr << 1);
	I2C2->MDR = data;
	I2C2->MCS = 7;
	
	while(I2C2->MCS & status) { }
}

void measureDist(){
	int i;
	for(i=0;i<24;i++){
		if(adcData >= dataPointsX[i] && adcData <= dataPointsX[i+1]){
			useSlope = slope[i];
			i=24;
		}
	}
	useDist = 1/(adcData*useSlope);
	avgDist = useDist-5.5; // work on this equation a bit more
	if(avgDist > 24){
		avgDist -= 1;
		if(avgDist > 30){
			avgDist -= 1;
		}
	}
}

void ADCinit(){
	//Uses PE2
	SYSCTL->RCGCGPIO |= 0x10;
	GPIOE->DEN &= ~0x4;
	GPIOE->AMSEL |= 0x4;
	SYSCTL->RCGCADC |= 0x1;
	delay(10);
	
	ADC0->PC |= 0x3; //peri config
	ADC0->SSMUX3 |= 0x1;
	ADC0->SSCTL3 |= 0x6;
	ADC0->SAC |= 0x6;
	ADC0->ACTSS |= 0x8; //ACTIVE_SS_R
	
	//still need ADC sequencer initialize stuff
	ADC0->IM |= 0x8; //INT_MASK_R
	NVIC_EnableIRQ(ADC0SS3_IRQn);
}

void ADC0SS3_Handler(){
	adcData = (ADC0->SSFIFO3 & 0xFFFFFF);
	delayms(10); //dont know why i put that there
	ADC0->ISC |= 0x8;
}

void finalSetup(){
	int i;
	for(i=0;i<24;i++){
		slope[i] = dataPointsY[i]/dataPointsX[i];
	}
	sendCmd(0x08);	//turns on display
	wholeLottaSpaces(4);
}

void delayms(int n){
	int i;
	int j;
	for(i=0;i<n;i++){
		for(j=0;j<3180;j++){
		}
	}
}

void startSample(){
	ADC0->PSSI |= 0x8;
}

