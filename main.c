/******************************************************************************
* DISCLAIMER

* This software is supplied by Renesas Electronics Corporation and is only 
* intended for use with Renesas products. No other uses are authorized.

* This software is owned by Renesas Electronics Corporation and is protected under 
* all applicable laws, including copyright laws.

* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES 
* REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, 
* INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
* PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY 
* DISCLAIMED.

* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS 
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE 
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES 
* FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS 
* AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

* Renesas reserves the right, without notice, to make changes to this 
* software and to discontinue the availability of this software.  
* By using this software, you agree to the additional terms and 
* conditions found by accessing the following link:
* http://www.renesas.com/disclaimer
******************************************************************************/
/* Copyright (C) 2015 Renesas Electronics Corporation. All rights reserved.  */
/******************************************************************************	
* File Name    : main.c
* Version      : 1.00
* Device(s)    : 
* Tool-Chain   : 
* H/W Platform : 
* Description  : 
******************************************************************************
* History : DD.MM.YYYY Version Description
*         : 01.07.2015 1.00    First Release
******************************************************************************/


/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
//#pragma interrupt INTIT timer_isr
#pragma sfr
#include "r_macro.h"  /* System macro and standard type definition */
#include "r_spi_if.h" /* SPI driver interface */
#include "lcd.h"      /* LCD driver interface */    
#include "timer.h"    /* Timer driver interface */
#include "stdio.h"    /* Standard IO library: sprintf */
#include "Glyph_API.h" 
#include "string.h"
/******************************************************************************
Typedef definitions
******************************************************************************/

/******************************************************************************
Macro definitions
******************************************************************************/

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/
#define RUNNING "Running...  "
#define PAUSING "Pausing...  "
#define NO_RECORD "No record   "
#define FIRST_RECORD "First record"
#define LAST_RECORD "Last record "

#define PAUSE_STAGE		1
#define RUN_STAGE		2
#define NO_RECORD_STAGE		3
#define FIRST_RECORD_STAGE	4
#define LAST_RECORD_STAGE	5

#define SECOND  1000
#define CYCLE 	1778
#define OFF_STAGE 0x7
#define STAGE_SW123 ((P7&0x70)>>4)
#define CHATTERING	200

#define SW3_ON	  0x5
#define SW2_ON	  0x6
#define SW1_ON	  0x3
#define SW12_ON	  0x2

#define SW3	  0x2
#define SW2	  0x1
#define SW1	  0x4
#define SW12	  0x5

#define TURN_ON 	1
#define TURN_OFF	0

#define MAX_DATA	20
#define SIX_LINE	6

uint8_t chatter(void);
void r_main_userinit(void);
void check_sw(uint8_t sw);
void check_stage(void);
void display_time(time_t *mtime);
void record (void);
void data_convert (data_type data);
void shiftarray(void);
void Scroll_up(void);
void Scroll_down(void);
void show_records(unsigned int min,unsigned int max);
void display_records (uint8_t line);
void reset_data(void);
void Wait1MiliSecond(void);
void update_time(void);

data_type data[20];
time_t p_time;

uint8_t pause_flag=TURN_ON;
uint8_t reset_flag=TURN_OFF;
uint8_t mode_change_flag=TURN_OFF;
uint8_t time_change_flag=TURN_OFF;
uint8_t display_record_flag=TURN_OFF;
uint8_t display_2s_flag=TURN_OFF;
uint8_t record_flag=TURN_OFF;
uint8_t scroll_up_flag=TURN_OFF;
uint8_t scroll_down_flag=TURN_OFF;

uint8_t sw=0;
uint8_t stage=PAUSE_STAGE;
uint8_t store=PAUSE_STAGE;

char string_time[13];
unsigned int second=0;
unsigned int centisecond=0;
unsigned int number=0;
unsigned int datanumber=0;
unsigned int count_record=0;
unsigned int max_record =0;
unsigned int min=0,max=0 ;

/******************************************************************************
* Function Name: main
* Description  : Main program
* Arguments    : none
* Return Value : none
******************************************************************************/
void main(void)
{
	/* Local time structure to temporary store the global time */
	
	/* Clear LCD display */
	r_main_userinit();
	timer_init();
	R_TAU0_Create();
	
	ClearLCD();

	/* Print message to LCD */
	DisplayLCD(LCD_LINE1,(uint8_t*)PAUSING);
	DisplayLCD(LCD_LINE2,(uint8_t*)"00:00:00   ");
	//timer_start();

	/* Initialize external interrupt for switches */

	/* Initialize timer driver */

	/* Main loop - Infinite loop */

	while (1) 
	{
		sw=chatter();
		check_sw(sw);
		check_stage();
		if(time_change_flag==TURN_ON)
		{	
			display_time(&p_time);
			time_change_flag=TURN_OFF;
		}
		if(record_flag==TURN_ON)
		{
			record();
			record_flag=TURN_OFF;
		}
		
		if((scroll_up_flag==TURN_ON)&&(scroll_down_flag==TURN_OFF))
		{
			Scroll_up();
			scroll_up_flag=TURN_OFF;
		}
		else if((scroll_up_flag==TURN_OFF)&&(scroll_down_flag==TURN_ON))
		{
			Scroll_down();
			scroll_down_flag=TURN_OFF;
		}	
		
		if(display_record_flag==TURN_ON)
		{
			if(reset_flag==TURN_OFF)
			{
				show_records(min,max);
			}
			display_record_flag=TURN_OFF;
		}
	}
}
/******************************************************************************
* Function Name: r_main_userinit
* Description  : User initialization routine
* Arguments    : none
* Return Value : none
******************************************************************************/
void r_main_userinit(void)
{
	uint16_t i;
	T_glyphError  result;
	T_glyphHandle hlcd;

	/* Enable interrupt */
	EI();

	/* Output a logic LOW level to external reset pin*/
	P13_bit.no0 = 0;
	for (i = 0; i < 1000; i++)
	{
		NOP();
	}

	/* Generate a raising edge by ouput HIGH logic level to external reset pin */
	P13_bit.no0 = 1;
	for (i = 0; i < 1000; i++)
	{
		NOP();
	}

	/* Output a logic LOW level to external reset pin, the reset is completed */
	P13_bit.no0 = 0;
	
	/* Initialize SPI channel used for LCD */
	R_SPI_Init(SPI_LCD_CHANNEL);
	
	/* Initialize Chip-Select pin for LCD-SPI: P145 (Port 14, pin 5) */
	R_SPI_SslInit(
		SPI_SSL_LCD,             /* SPI_SSL_LCD is the index defined in lcd.h */
		(unsigned char *)&P14,   /* Select Port register */
		(unsigned char *)&PM14,  /* Select Port mode register */
		5,                       /* Select pin index in the port */
		0,                       /* Configure CS pin active state, 0 means active LOW level  */
		0                        /* Configure CS pin active mode, 0 means active per transfer */
	);
	result = GlyphOpen (&hlcd, 0);        /* Open the Glyph API */
	result = GlyphNormalScreen(hlcd);   /* Sets up the normal LCD Screen */
	result =GlyphClearScreen(hlcd);       /* Clears the LCD Screen */
	/* Set writing position to (0, 0) */
	/* Initialize LCD driver */
	InitialiseLCD();	
}
/******************************************************************************
* Function Name: chatter
* Description  : Detect falling edge and remove chattering, return which switch is pressed
* Arguments    : None
* Return Value : sw
******************************************************************************/
uint8_t chatter(void){
	uint8_t current_data = 0;
	static uint8_t predata =OFF_STAGE,outdata = OFF_STAGE,lastout = OFF_STAGE;
	static int match_time = 0;
	
	lastout = outdata;
	current_data=(uint8_t)(STAGE_SW123);
	
	if(current_data!=predata)
	{
		match_time=0;
		predata=current_data;
	}
	else
	{
		match_time++;
		if(match_time>=CHATTERING)
		{	
			match_time=0;
			outdata=current_data;
			if(outdata==SW12_ON&&lastout!=SW12_ON)
			{	lastout=OFF_STAGE;
			}
		}
	}
	return ((outdata ^ lastout)&((~outdata)&OFF_STAGE));//detect falling edge
}
/******************************************************************************
* Function Name: check_sw
* Description  : check which switch is pressed and check switch's fuction
* Arguments    : uint8_t sw
* Return Value : none
******************************************************************************/
void check_sw(uint8_t sw)
 {
    	switch(sw)
	{
		case 0:
			break;
			
		/*switch 1 is pressed*/
		case SW1: 
			if(count_record!=0)
			{	
				if(number==0)
				{
					stage=FIRST_RECORD_STAGE;
				}
				else
				{
					scroll_up_flag=TURN_ON;
					scroll_down_flag=TURN_OFF;
				}
				mode_change_flag=TURN_ON;
				time_change_flag=TURN_ON;
			}
			else
			{
					stage=NO_RECORD_STAGE;
					mode_change_flag=TURN_ON;
					time_change_flag=TURN_ON;
			}
			
			break;
		/*switch 2 is pressed*/
		case SW2:
			if(count_record!=0)
			{	
				if((max_record-number)>6)
				{

						scroll_down_flag=TURN_ON;
						scroll_up_flag=TURN_OFF;
				}
				else
				{
					stage=LAST_RECORD_STAGE;
				}
				mode_change_flag=TURN_ON;
				time_change_flag=TURN_ON;
			}
			else
			{
					stage=NO_RECORD_STAGE;
					mode_change_flag=TURN_ON;
					time_change_flag=TURN_ON;
				
			}
			break;
			

		/*switch 3 is pressed*/
		case SW3:
			if(pause_flag==1)
			{	
				timer_start();
				stage=RUN_STAGE;
				pause_flag=0;
				mode_change_flag=TURN_ON;
				reset_flag=TURN_OFF;
				
				
				break;
			}
			else
			{
				count_record++;
				display_record_flag=TURN_ON;
				record_flag=TURN_ON;
				
			}		
			break;
			
		/* both switch 1 and 2 are pressed*/
		case SW12:
			if(pause_flag==0)
			{	
				timer_stop();
				stage=PAUSE_STAGE;
				pause_flag=1;
				display_record_flag=TURN_ON;
				time_change_flag=TURN_ON;
				mode_change_flag=TURN_ON;
			}
			else if(pause_flag==1)
			{
				ClearLCD();
				stage=PAUSE_STAGE;
				time_change_flag=TURN_ON;
				mode_change_flag=TURN_ON;
				reset_data();
			}
			break;
		}
}
/******************************************************************************
* Function Name: check_stage
* Description  : Check mode is changed or not and display current mode
* Arguments    : none
* Return Value : count_flag
******************************************************************************/
void check_stage(void){
	if(mode_change_flag==TURN_ON)
	{
		//ClearLCD();
		switch(stage)
		{
			default:
				break;
				
			case PAUSE_STAGE:
			
				DisplayLCD(LCD_LINE1,(uint8_t*)PAUSING);
				pause_flag=1;
				store=stage;
				R_TAU0_Stop();
				break;
				
			case RUN_STAGE:
			
				DisplayLCD(LCD_LINE1,(uint8_t*)RUNNING);
				pause_flag=0;
				store=stage;
				if(count_record!=0)
				display_record_flag=TURN_ON;
				break;
				
			case NO_RECORD_STAGE:
			
				DisplayLCD(LCD_LINE1,(uint8_t*)NO_RECORD);
				display_2s_flag=TURN_ON;
				if(pause_flag==TURN_ON)
				R_TAU0_Start();
				break;
				
			case FIRST_RECORD_STAGE:
			
				DisplayLCD(LCD_LINE1,(uint8_t*)FIRST_RECORD);
				display_2s_flag=TURN_ON;
				display_record_flag=TURN_ON;
				if(pause_flag==TURN_ON)
				R_TAU0_Start();	
				break;
				
			case LAST_RECORD_STAGE:
			
				DisplayLCD(LCD_LINE1,(uint8_t*)LAST_RECORD);
				display_2s_flag=TURN_ON;
				display_record_flag=TURN_ON;
				if(pause_flag==TURN_ON)
				R_TAU0_Start();
				break;		
				
		}
		mode_change_flag=TURN_OFF;
	}
}
/******************************************************************************
* Function Name: update_time
* Description  : time count when in counting mode and second >0 
* Arguments    : none
* Return Value : none
******************************************************************************/
void update_time(void){
	static int j=0;
	if(second>0)
	{
		j++;
		Wait1MiliSecond();
		if(j>=1000)
		{
			j=0;
			second--;
			time_change_flag=TURN_ON;
		}
	}

}
/******************************************************************************
* Function Name: display_time
* Description  : display time on LCD when time change
* Arguments    : time_t *mtime
* Return Value : none
******************************************************************************/
void display_time(time_t *mtime){
	char string_time1[8];
	mtime->centisecond=(uint8_t)(centisecond%100U);
	mtime->second=(uint8_t)((centisecond/100U)%60);
	mtime->minute=(uint8_t)((centisecond/100U)/60);
	sprintf(string_time1, "%0.2d:%0.2d:%0.2d",mtime->minute,mtime->second,mtime->centisecond);
	DisplayLCD(LCD_LINE2,(uint8_t*)string_time1);
}
 /******************************************************************************
* Function Name: data_convert
* Description  : convert data record to string
* Arguments    : data_type data
* Return Value : none
******************************************************************************/
void data_convert (data_type data)
{
	unsigned char centisecond,second,minute,no;
	centisecond=(unsigned char)(data.data%100U);	
	second=(unsigned char)((data.data/100U)%60);
	minute=(unsigned char)((data.data/100U)/60);
	no=(unsigned char)data.datano;
	sprintf(string_time, "#%0.2d %0.2d:%0.2d:%0.2d",no,minute,second,centisecond);
}
 /******************************************************************************
* Function Name: record
* Description  : record data
* Arguments    : none
* Return Value : none
******************************************************************************/
void record (void)
{
	max_record++;
	/*reset Scroll variable*/ 
	/*Will srcoll again from last record*/	
	if (max_record >(MAX_DATA+1))
	{
		max_record =(MAX_DATA+1);
	}
	/*save new data in last record*/
	else if(max_record<=MAX_DATA)
	{
	data[datanumber].data=centisecond;
	data[datanumber].datano=datanumber+1;
	datanumber++;
	}
	/*if not Scrolling*/
	
	if (max_record <=SIX_LINE) /*If number of record <6*/
	{
		min=0;
		max=max_record;
		display_record_flag=TURN_ON;
	}
	else if (max_record >SIX_LINE && max_record <=MAX_DATA)/*if number of record >=6 till number of record <= max number*/
	{
		if((number+SIX_LINE+1)==datanumber)
			number++;
			
		if(number>=(MAX_DATA-SIX_LINE))
			number=(MAX_DATA-SIX_LINE);
			
		min=number;
		max=number+SIX_LINE;
		display_record_flag=TURN_ON;
			
	}
	else if (max_record >MAX_DATA)	/*if number of record > max number*/
	{ 
		datanumber++;
		
		if((number>0)&&(number<(MAX_DATA-SIX_LINE)))
		number--;
		
		if(number>(MAX_DATA-SIX_LINE))
			number=(MAX_DATA-SIX_LINE);
			
		shiftarray();/*shift array*/
		
		min=number;
		max=number+SIX_LINE;
		
		display_record_flag=TURN_ON;	
		max_record =MAX_DATA;
	}
	
}
/******************************************************************************
* Function Name: display_records
* Description  : diplay each record on LCD
* Arguments    : line
* Return Value : none
******************************************************************************/
void display_records (uint8_t line)
{
	uint8_t a;
	switch (line)
	{

	case 0:
		a=LCD_LINE3;
		break;
	case 1:
		a=LCD_LINE4;
		break;
	case 2:
		a=LCD_LINE5;
		break;
	case 3:
		a=LCD_LINE6;
		break;
	case 4:
		a=LCD_LINE7;
		break;
	case 5:
		a=LCD_LINE8;
		break;
		
	default:
	 	break;
	}
	DisplayLCD(a,(uint8_t*) string_time);
}
/******************************************************************************
* Function Name: show_records
* Description  : show all records on LCD
* Arguments    : min,max
* Return Value : none
******************************************************************************/
void show_records (unsigned int min,unsigned int max)
{
	uint8_t k=0;
	unsigned int i=0;
	for(i =min ; i < max; i++)
	{	
		data_convert(data[i]);
		display_records(k);
		k++;
							
	}
}
/******************************************************************************
* Function Name: shiftarray
* Description  : Shift value when number of record over max length of record and update last record
* Arguments    : none
* Return Value : none
******************************************************************************/
void shiftarray(void)
{
	int i=0;
	for(i=0; i <(MAX_DATA-1);i++)
	{
		data[i].data=data[i+1].data;
		data[i].datano=data[i+1].datano;
	}
	data[19].data=centisecond;
	data[19].datano=datanumber;
}
/******************************************************************************
* Function Name: reset_data
* Description  : reset system
* Arguments    : none
* Return Value : none
******************************************************************************/
void reset_data(void)
{
	int i=0;
	centisecond=0;
	datanumber=0;
	number=0;
	stage=PAUSE_STAGE;
	count_record=0;
	max_record=0;
	
	for(i=0; i <MAX_DATA;i++)
	{
		data[i].data=0;
		data[i].datano=0;
	}
	reset_flag=TURN_ON;
}
/******************************************************************************
* Function Name: Scroll_up
* Description  : Using to scroll down the list record
* Arguments    : none
* Return Value : none
******************************************************************************/
void Scroll_up(void)
{
	
	int i,k=0;
	number--;
	min=number;
	max=number+SIX_LINE;
	display_record_flag=TURN_ON;		

}
/******************************************************************************
* Function Name: Scroll_down
* Description  : Using to scroll down the list record
* Arguments    : none
* Return Value : none
******************************************************************************/
void Scroll_down(void)
{
	
	int i,y,k=0;
	i=0;
	y=1;
	number++;
	if(number>=(MAX_DATA-SIX_LINE))
		number=(MAX_DATA-SIX_LINE);
	min=number;
	max=number+SIX_LINE;
	display_record_flag=TURN_ON;
	
}
/******************************************************************************
End of file
******************************************************************************/
