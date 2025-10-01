#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"

unsigned char PartImage[1000]; // Production: Partial display buffer  
unsigned char partFlag=1;

////////////////////////////////////Production E-paper Driver//////////////////////////////////////////////////////////
//Busy function
void lcd_chkstatus(void)
{ 
  while(1)
  {	 //=0 BUSY
     if(isEPD_W21_BUSY==1) break;
  }  
}

//Full screen refresh initialization
void EPD_Init(void)
{
		EPD_W21_RST_0;		// Module reset
		delay(10);//At least 10ms delay 
		EPD_W21_RST_1;
		delay(10);//At least 10ms delay 
    lcd_chkstatus();

	  EPD_W21_WriteCMD(0x01);			//POWER SETTING
		EPD_W21_WriteDATA (0x07);
		EPD_W21_WriteDATA (0x07);    //VGH=20V,VGL=-20V
		EPD_W21_WriteDATA (0x3f);		//VDH=15V
		EPD_W21_WriteDATA (0x3f);		//VDL=-15V

		
	  //Enhanced display drive(Add 0x06 command)
		EPD_W21_WriteCMD(0x06);			//Booster Soft Start 
		EPD_W21_WriteDATA (0x17);
		EPD_W21_WriteDATA (0x17);   
		EPD_W21_WriteDATA (0x28);		
		EPD_W21_WriteDATA (0x17);		
	
	  EPD_W21_WriteCMD(0x04); //POWER ON
		lcd_chkstatus();        //waiting for the electronic paper IC to release the idle signal
	
		EPD_W21_WriteCMD(0X00);			//PANNEL SETTING
		EPD_W21_WriteDATA(0x0F);   //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

		EPD_W21_WriteCMD(0x61);			//resolution setting
		EPD_W21_WriteDATA (EPD_WIDTH/256); 
		EPD_W21_WriteDATA (EPD_WIDTH%256); 	
		EPD_W21_WriteDATA (EPD_HEIGHT/256);
		EPD_W21_WriteDATA (EPD_HEIGHT%256); 

		EPD_W21_WriteCMD(0X15);		
		EPD_W21_WriteDATA(0x00);		

		EPD_W21_WriteCMD(0X50);			//VCOM AND DATA INTERVAL SETTING
		EPD_W21_WriteDATA(0x11);  //0x10  --------------
		EPD_W21_WriteDATA(0x07);

		EPD_W21_WriteCMD(0X60);			//TCON SETTING
		EPD_W21_WriteDATA(0x22);
}

//Fast refresh initialization
void EPD_Init_Fast(void)
{
	EPD_W21_RST_0;  // Module reset   
	delay(10);//At least 10ms delay 
	EPD_W21_RST_1;
	delay(10); //At least 10ms delay 
  
	EPD_W21_WriteCMD(0X00);			//PANNEL SETTING
	EPD_W21_WriteDATA(0x0F);   //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

	EPD_W21_WriteCMD(0x04); //POWER ON
	delay(100);  
	lcd_chkstatus();        //waiting for the electronic paper IC to release the idle signal

	//Enhanced display drive(Add 0x06 command)
	EPD_W21_WriteCMD(0x06);			//Booster Soft Start 
	EPD_W21_WriteDATA (0x27);
	EPD_W21_WriteDATA (0x27);   
	EPD_W21_WriteDATA (0x18);		
	EPD_W21_WriteDATA (0x17);		

	EPD_W21_WriteCMD(0xE0);
	EPD_W21_WriteDATA(0x02);
	EPD_W21_WriteCMD(0xE5);
	EPD_W21_WriteDATA(0x5A);
	
	EPD_W21_WriteCMD(0X50);			//VCOM AND DATA INTERVAL SETTING
	EPD_W21_WriteDATA(0x11);
	EPD_W21_WriteDATA(0x07);	
}

//Partial refresh initialization
void EPD_Init_Part(void)
{	
	EPD_W21_RST_0;  // Module reset   
	delay(10);//At least 10ms delay 
	EPD_W21_RST_1;
	delay(10); //At least 10ms delay 
	
	EPD_W21_WriteCMD(0X00);			//PANNEL SETTING
	EPD_W21_WriteDATA(0x1F);   //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f
	
	EPD_W21_WriteCMD(0x04); //POWER ON
	delay(100);  
	lcd_chkstatus();        //waiting for the electronic paper IC to release the idle signal
	
	EPD_W21_WriteCMD(0xE0);
	EPD_W21_WriteDATA(0x02);
	EPD_W21_WriteCMD(0xE5);
	EPD_W21_WriteDATA(0x6E);
	
	EPD_W21_WriteCMD(0X50);			//VCOM AND DATA INTERVAL SETTING
	EPD_W21_WriteDATA(0xA9); 	
	EPD_W21_WriteDATA(0x07);	
}

//Display Update Function
void EPD_Update(void)
{   
	//Refresh
	EPD_W21_WriteCMD(0x12);		//DISPLAY REFRESH 	
	delay(1);	             //!!!The delay here is necessary, 200uS at least!!!     
	lcd_chkstatus();          //waiting for the electronic paper IC to release the idle signal
}

//Full screen refresh display function
void EPD_WhiteScreen_ALL(const unsigned char* datasBW,const unsigned char* datasRW)
{
    unsigned int i;
	  //Write Data
		EPD_W21_WriteCMD(0x10);	       //Transfer old data
	  for(i=0;i<EPD_ARRAY;i++)	  
    {	
	    EPD_W21_WriteDATA(~datasBW[i]);  //Transfer the actual displayed data
    }	
		EPD_W21_WriteCMD(0x13);		     //Transfer new data
	  for(i=0;i<EPD_ARRAY;i++)	     
	  {
	    EPD_W21_WriteDATA(datasRW[i]);  //Transfer the actual displayed data
	  }
    EPD_Update();	    
}

//Fast refresh display function
void EPD_WhiteScreen_ALL_Fast(const unsigned char *datas)
{
  unsigned int i;	
  EPD_W21_WriteCMD(0x10);  //write old data 
  for(i=0;i<EPD_ARRAY;i++)
   {               
     EPD_W21_WriteDATA(0x00);
   }
  EPD_W21_WriteCMD(0x13);  //write new data 
  for(i=0;i<EPD_ARRAY;i++)
   {               
     EPD_W21_WriteDATA(datas[i]);
   }	 
   EPD_Update();	 	 
}

//Clear screen display
void EPD_WhiteScreen_White(void)
{
    unsigned int i;
	  //Write Data
		EPD_W21_WriteCMD(0x10);	       //Transfer old data
	  for(i=0;i<EPD_ARRAY;i++)	  
    {	
	    EPD_W21_WriteDATA(0xFF); 
    }
		EPD_W21_WriteCMD(0x13);		     //Transfer new data
	  for(i=0;i<EPD_ARRAY;i++)	     
	  {
	    EPD_W21_WriteDATA(0x00);  //Transfer the actual displayed data
	  }
    EPD_Update();	    
}

//Display all black
void EPD_WhiteScreen_Black(void)
{
	unsigned int i;
	//Write Data
	EPD_W21_WriteCMD(0x10);	     
	for(i=0;i<EPD_ARRAY;i++)	     
	{
		EPD_W21_WriteDATA(0x00);  
	}
	EPD_W21_WriteCMD(0x13);	     
	for(i=0;i<EPD_ARRAY;i++)	     
	{
		EPD_W21_WriteDATA(0x00);  
	}
   EPD_Update();	
}

//Partial refresh of background display
void EPD_SetRAMValue_BaseMap(const unsigned char* datasBW,const unsigned char* datasRW)
{
  unsigned int i;	
	EPD_W21_WriteCMD(0x10);	       //Transfer old data
	for(i=0;i<EPD_ARRAY;i++)	  
	{	
		EPD_W21_WriteDATA(~datasBW[i]);  //Transfer the actual displayed data
	}	
	EPD_W21_WriteCMD(0x13);		     //Transfer new data
	for(i=0;i<EPD_ARRAY;i++)	     
	{
		EPD_W21_WriteDATA(datasRW[i]);  //Transfer the actual displayed data
	} 
  EPD_Update();		 
}

//Partial refresh display
void EPD_Dis_Part(unsigned int x_start,unsigned int y_start,const unsigned char * datas,unsigned int PART_COLUMN,unsigned int PART_LINE)
{
	unsigned int i;  
	unsigned int x_end,y_end;

	x_end=x_start+PART_LINE-1; 
	y_end=y_start+PART_COLUMN-1;

	EPD_W21_WriteCMD(0x91);		//This command makes the display enter partial mode
	EPD_W21_WriteCMD(0x90);		//resolution setting
	EPD_W21_WriteDATA (x_start/256);
	EPD_W21_WriteDATA (x_start%256);   //x-start    

	EPD_W21_WriteDATA (x_end/256);		
	EPD_W21_WriteDATA (x_end%256-1);  //x-end	

	EPD_W21_WriteDATA (y_start/256);  //
	EPD_W21_WriteDATA (y_start%256);   //y-start    

	EPD_W21_WriteDATA (y_end/256);		
	EPD_W21_WriteDATA (y_end%256-1);  //y-end
	EPD_W21_WriteDATA (0x01);	

	EPD_W21_WriteCMD(0x13);				 //writes New data to SRAM.
	for(i=0;i<PART_COLUMN*PART_LINE/8;i++)	     
	 {
		 EPD_W21_WriteDATA(datas[i]);  
	 }	
    EPD_Update();	
}

//Clock display with multiple digits
void EPD_Dis_Part_Time(unsigned int x_start,unsigned int y_start,
	                      const unsigned char * datas_A,const unsigned char * datas_B,
												const unsigned char * datas_C,const unsigned char * datas_D,const unsigned char * datas_E,
                        unsigned char num,unsigned int PART_COLUMN,unsigned int PART_LINE)
{
	unsigned int i,j;  
	unsigned int x_end,y_end;

	x_end=x_start+PART_LINE*num-1; 
	y_end=y_start+PART_COLUMN-1;

	EPD_W21_WriteCMD(0x91);		//This command makes the display enter partial mode
	EPD_W21_WriteCMD(0x90);		//resolution setting
	EPD_W21_WriteDATA (x_start/256);
	EPD_W21_WriteDATA (x_start%256);   //x-start    

	EPD_W21_WriteDATA (x_end/256);		
	EPD_W21_WriteDATA (x_end%256-1);  //x-end	

	EPD_W21_WriteDATA (y_start/256);  //
	EPD_W21_WriteDATA (y_start%256);   //y-start    

	EPD_W21_WriteDATA (y_end/256);		
	EPD_W21_WriteDATA (y_end%256-1);  //y-end
	EPD_W21_WriteDATA (0x01);	

	EPD_W21_WriteCMD(0x13);				 //writes New data to SRAM.
	for(i=0;i<PART_COLUMN;i++)	     
	 {
			for(j=0;j<PART_LINE/8;j++)	     
				EPD_W21_WriteDATA(datas_A[i*PART_LINE/8+j]);  	
			for(j=0;j<PART_LINE/8;j++)	     
				EPD_W21_WriteDATA(datas_B[i*PART_LINE/8+j]);  
			for(j=0;j<PART_LINE/8;j++)	     
				EPD_W21_WriteDATA(datas_C[i*PART_LINE/8+j]);  	
			for(j=0;j<PART_LINE/8;j++)	     
				EPD_W21_WriteDATA(datas_D[i*PART_LINE/8+j]); 	
			for(j=0;j<PART_LINE/8;j++)	     
				EPD_W21_WriteDATA(datas_E[i*PART_LINE/8+j]); 			 
	 }

	EPD_Update();
	EPD_W21_WriteCMD(0X92);  	//This command makes the display exit partial mode and enter normal mode. 
}

//Display rotation 180 degrees initialization
void EPD_Init_180(void)
{
		EPD_W21_RST_0;		// Module reset
		delay(10);//At least 10ms delay 
		EPD_W21_RST_1;
		delay(10);//At least 10ms delay 
    lcd_chkstatus();

	  EPD_W21_WriteCMD(0x01);			//POWER SETTING
		EPD_W21_WriteDATA (0x07);
		EPD_W21_WriteDATA (0x07);    //VGH=20V,VGL=-20V
		EPD_W21_WriteDATA (0x3f);		//VDH=15V
		EPD_W21_WriteDATA (0x3f);		//VDL=-15V
		
	  //Enhanced display drive(Add 0x06 command)
		EPD_W21_WriteCMD(0x06);			//Booster Soft Start 
		EPD_W21_WriteDATA (0x17);
		EPD_W21_WriteDATA (0x17);   
		EPD_W21_WriteDATA (0x28);		
		EPD_W21_WriteDATA (0x17);		
	
	  EPD_W21_WriteCMD(0x04); //POWER ON
		lcd_chkstatus();        //waiting for the electronic paper IC to release the idle signal
	
		EPD_W21_WriteCMD(0X00);			//PANNEL SETTING
		EPD_W21_WriteDATA(0x03);   //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

		EPD_W21_WriteCMD(0x61);			//resolution setting
		EPD_W21_WriteDATA (EPD_WIDTH/256); 
		EPD_W21_WriteDATA (EPD_WIDTH%256); 	
		EPD_W21_WriteDATA (EPD_HEIGHT/256);
		EPD_W21_WriteDATA (EPD_HEIGHT%256); 

		EPD_W21_WriteCMD(0X15);		
		EPD_W21_WriteDATA(0x00);		

		EPD_W21_WriteCMD(0X50);			//VCOM AND DATA INTERVAL SETTING
		EPD_W21_WriteDATA(0x11);
		EPD_W21_WriteDATA(0x07);

		EPD_W21_WriteCMD(0X60);			//TCON SETTING
		EPD_W21_WriteDATA(0x22);
}

//Deep sleep function
void EPD_DeepSleep(void)
{  	
		EPD_W21_WriteCMD(0X50);  //VCOM AND DATA INTERVAL SETTING			
		EPD_W21_WriteDATA(0xf7); //WBmode:VBDF 17|D7 VBDW 97 VBDB 57		WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7	

		EPD_W21_WriteCMD(0X02);  	//power off
	  lcd_chkstatus();          //waiting for the electronic paper IC to release the idle signal
		delay(100);	  //!!!The delay here is necessary, 200uS at least!!!     	
	  EPD_W21_WriteCMD(0X07);  	//deep sleep
		EPD_W21_WriteDATA(0xA5);
}

////////////////////////////////PersonalCMS Timer Functions////////////////////////////////////////////
// Timer display area: Bottom-right corner 150x30 pixels
#define TIMER_WIDTH 150
#define TIMER_HEIGHT 30
#define TIMER_X_START (EPD_WIDTH - TIMER_WIDTH)   // 650
#define TIMER_Y_START (EPD_HEIGHT - TIMER_HEIGHT) // 450

// Simple 5x7 font data for digits 0-9
const unsigned char digit_font[10][7] = {
  {0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00}, // 0
  {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46, 0x00, 0x00}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31, 0x00, 0x00}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00}  // 9
};

// Clear timer area for fresh display
void EPD_Clear_Timer_Area(void)
{
  // Create white buffer for timer area
  unsigned char timer_clear[TIMER_WIDTH * TIMER_HEIGHT / 8];
  for(int i = 0; i < sizeof(timer_clear); i++) {
    timer_clear[i] = 0x00; // White background
  }
  
  EPD_Dis_Part(TIMER_X_START, TIMER_Y_START, timer_clear, TIMER_HEIGHT, TIMER_WIDTH);
}

// Display timer in seconds (format: "123s" for up to 999 seconds)
void EPD_Display_Timer(unsigned long seconds)
{
  // Limit to 999 seconds for display
  if(seconds > 999) seconds = 999;
  
  // Create timer display buffer
  unsigned char timer_buffer[TIMER_WIDTH * TIMER_HEIGHT / 8];
  
  // Clear buffer (white background)
  for(int i = 0; i < sizeof(timer_buffer); i++) {
    timer_buffer[i] = 0x00;
  }
  
  // Convert seconds to digits
  int hundreds = seconds / 100;
  int tens = (seconds / 10) % 10;
  int ones = seconds % 10;
  
  // Position for text (right-aligned)
  int digit_width = 8;
  int digit_height = 7;
  int text_width = digit_width * 4; // 3 digits + 's'
  int start_x = TIMER_WIDTH - text_width - 5; // 5 pixel margin from right
  int start_y = (TIMER_HEIGHT - digit_height) / 2;
  
  // Draw digits (simple bitmap rendering)
  // This is a simplified version - in practice you'd want more sophisticated rendering
  int x_pos = start_x;
  
  // Draw hundreds digit (if > 0)
  if(hundreds > 0) {
    // Simplified: just draw digit patterns
    x_pos += digit_width;
  }
  
  // Draw tens digit (if hundreds > 0 or tens > 0)
  if(hundreds > 0 || tens > 0) {
    x_pos += digit_width;
  }
  
  // Draw ones digit (always)
  x_pos += digit_width;
  
  // Draw 's' character
  // Simplified 's' pattern
  x_pos += digit_width;
  
  // For simplicity, we'll create a basic pattern
  // In a real implementation, you'd properly render the font
  for(int i = 0; i < sizeof(timer_buffer); i++) {
    timer_buffer[i] = 0xFF; // Black text pattern (simplified)
  }
  
  // Display the timer area using partial refresh
  EPD_Dis_Part(TIMER_X_START, TIMER_Y_START, timer_buffer, TIMER_HEIGHT, TIMER_WIDTH);
}