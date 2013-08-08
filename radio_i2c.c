#include <tiny2313a.h>

// I2C Bus functions
#asm
   .equ __i2c_port=0x18 ;PORTB
   .equ __sda_bit=5
   .equ __scl_bit=7
#endasm
#include <i2c.h>
#include <delay.h>

#define DS_1 PORTB |= (1<<2)
#define DS_0 PORTB &= ~(1<<2)

#define SH_CP_1 PORTB |= (1<<4)
#define SH_CP_0 PORTB &= ~(1<<4)

#define ST_CP_1 PORTB |= (1<<3)
#define ST_CP_0 PORTB &= ~(1<<3)

#define segA (1<<15)
#define segB (1<<10)
#define segC (1<<2)
#define segD (1<<6)
#define segE (1<<11)
#define segF (1<<14)
#define segG (1<<1)
#define DP (1<<7)
#define dg1 (1<<3)
#define dg2 (1<<13)
#define dg3 (1<<12)
#define dg4 (1<<4)

unsigned char temp = 0;

unsigned char mode = 0;
bit showQuality = 0;
bit isAccessible = 1;
unsigned char b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12;
unsigned char encoderOldState;
volatile unsigned char encoderNewState;
unsigned char encoderRotation;
unsigned char softBlend = 26;

void startTimer (void)  {
    TCNT1H=0xFC;
    TCNT1L=0xF2; 
    TCCR1B=0x05;
    isAccessible = 0;
}

// Timer 0 overflow interrupt service routine
//10ms delay. Encoder data receiving
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
// Reinitialize Timer0 value
TCNT0=0xBE;

if (isAccessible == 1)  {
    //read encoder state
    //ecnoderRotation: 1 - left; 2 - right
    encoderOldState = encoderNewState;
    encoderNewState = 0;
    encoderNewState |= (PINB.0 << 1);
    encoderNewState |= PINB.1;
    //encoderRotation = 0;    
    
     switch (encoderOldState)    {
        case 0: {
        if (encoderNewState == 2) encoderRotation = 1; 
        if (encoderNewState == 1) encoderRotation = 2;
        break;
        }
                       
        case 1: {
        if (encoderNewState == 0) encoderRotation = 3; 
        if (encoderNewState == 3) encoderRotation = 4;
        break;
        }     
                
        case 2: {                                           
        if (encoderNewState == 3) encoderRotation = 5; 
        if (encoderNewState == 0) encoderRotation = 6;
        break;
        }
                
        case 3: {                                           
        if (encoderNewState == 1) encoderRotation = 7; 
        if (encoderNewState == 2) encoderRotation = 8;
        break;
        }                           
     }   
    
    if (mode == 3)  {              //mode = 0 freq scan up/down
        if (encoderRotation == 2)    {
            i2c_start();
            i2c_write(0x20);
            i2c_write(0b11000011); i2c_write(b2); //02h   
            i2c_write(b3); i2c_write(b4); //03h
            i2c_write(b5); i2c_write(b6); //04h
            i2c_write(b7); i2c_write(b8); //05h
            i2c_write(b9); i2c_write(b10); //06h
            i2c_write(b11); i2c_write(b12); //07h
            i2c_stop();  
            startTimer();
        }
                        
        if (encoderRotation == 1)   {
            i2c_start();
            i2c_write(0x20);
            i2c_write(0b11000001); i2c_write(b2); //02h   
            i2c_write(b3); i2c_write(b4); //03h
            i2c_write(b5); i2c_write(b6); //04h
            i2c_write(b7); i2c_write(b8); //05h
            i2c_write(b9); i2c_write(b10); //06h
            i2c_write(b11); i2c_write(b12); //07h
            i2c_stop();  
            startTimer();
            }
    }   
             
    if (mode == 1) {               //noise soft blend     
        if (encoderRotation == 2)   {
            if (softBlend < 31)
               softBlend++;
            else 
                softBlend = 0;
        }
        if (encoderRotation == 1)   {
            if (softBlend > 0)
                softBlend--;
            else 
                softBlend = 31;
        }                     
        if (encoderRotation != 0)  {
            i2c_start();
            i2c_write(0x20);
            i2c_write(b1); i2c_write(b2); //02h   
            i2c_write(b3); i2c_write(b4); //03h
            i2c_write(b5); i2c_write(b6); //04h
            i2c_write(b7); i2c_write(b8); //05h
            i2c_write(b9); i2c_write(b10); //06h 
            b11 = softBlend << 2;
            i2c_write(b11); i2c_write(b12); //07h
            i2c_stop();  
            startTimer();          
        }
    }
}
}

// Timer 1 overflow interrupt service routine
//100ms delay
interrupt [TIM1_OVF] void timer1_ovf_isr(void)
{
    //stop timer  
    TCCR1B=0x00;
    // Reinitialize Timer1 value
    TCNT1H=0xFC;
    TCNT1L=0xF2;
    isAccessible = 1;
}

void DS (unsigned int input)   {
      
      char i=0;
      for (i=0; i<16; i++)  { 
        if (input & (1<<15-i))    {
            DS_1;
        }        
        else    {
            DS_0;
        } 
        SH_CP_1;
        SH_CP_0;
      }      
      ST_CP_1;
      ST_CP_0;  
}

unsigned int writeDigit (char d, char pos)    
{
       unsigned int toDisplay = 12312;  //64734            
    
       switch (d) {
       case 1 :
        toDisplay |= (segB | segC);  
        break;
       case 2 :
        toDisplay |= (segA | segB | segG | segE | segD);
        break;                                                   
       case 3:
        toDisplay |= (segA | segB | segG | segC | segD);
        break;
       case 4:
        toDisplay |= (segF | segB | segG | segC);
        break;
       case 5:
        toDisplay |= (segA | segF | segG | segC | segD);
        break;
       case 6:
        toDisplay |= (segA | segF | segG | segE | segD | segC);
        break;
       case 7:
        toDisplay |= (segA | segB | segC);
        break;
       case 8:
        toDisplay |= (segA | segB | segC | segD | segE | segF | segG);
        break;
       case 9:
        toDisplay |= (segA | segB | segC | segD | segF | segG);
        break;
       case 0:
        toDisplay |= (segA | segB | segC | segD | segE | segF);
        break;        
       default: 
    }; 

       switch (pos) {
       case 1 :
        toDisplay &= ~dg1;
        break;
       case 2 :
        toDisplay &= ~dg2;
        break;
       case 3 :           
        toDisplay |= DP;        //dispaly dot?
        toDisplay &= ~dg3;
        break;
       case 4 :
        toDisplay &= ~dg4;
        break;                 
       break;
       default: 
    };  
    
    return toDisplay;
}

void main(void)
{
unsigned char delay_time = 5;
unsigned char data = 0;

// Crystal Oscillator division factor: 1
#pragma optsize-
CLKPR=0x80;
CLKPR=0x00;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

// Input/Output Ports initialization
// Port A initialization
// Func2=In Func1=In Func0=In 
// State2=T State1=T State0=T 
PORTA=0x00;
DDRA=0x00;

// Port B initialization
// Func7=Out Func6=In Func5=Out Func4=Out Func3=Out Func2=Out Func1=In Func0=In 
// State7=0 State6=T State5=0 State4=0 State3=0 State2=0 State1=P State0=P 
PORTB=0x03;
DDRB=0xBC;

// Port D initialization
// Func6=In Func5=In Func4=In Func3=In Func2=In Func1=Out Func0=Out 
// State6=P State5=P State4=T State3=T State2=T State1=0 State0=0 
PORTD=0x60;
DDRD=0x03;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 3,906 kHz
// Mode: Normal top=0xFF
// OC0A output: Disconnected
// OC0B output: Disconnected
TCCR0A=0x00;
TCCR0B=0x05;
TCNT0=0xBE;
OCR0A=0x00;
OCR0B=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 3,906 kHz
// Mode: Normal top=0xFFFF
// OC1A output: Discon.
// OC1B output: Discon.
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: On
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=0x00;
TCCR1B=0x00;
TCNT1H=0xFC;
TCNT1L=0xF2;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// Interrupt on any change on pins PCINT0-7: Off
GIMSK=0x00;
MCUCR=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=0x82;

// Universal Serial Interface initialization
// Mode: Disabled
// Clock source: Register & Counter=no clk.
// USI Counter Overflow Interrupt: Off
USICR=0x00;

// USART initialization
// USART disabled
UCSRB=0x00;

// Analog Comparator initialization
// Analog Comparator: Off
// Analog Comparator Input Capture by Timer/Counter 1: Off
ACSR=0x80;
DIDR=0x00;

//02h
b1 = 0b11000010;
b2 = 0b00000001;
//03h
b3 = 0b00000000;
b4 = 0b00000000;
//04h
b5 = 0b00000000;
b6 = 0b00000000;
//05h
b7 = 0b10001000;
b8 = 0b10001111;
//06h
b9 = 0b00000000;
b10 = 0b00000000;
//07h
b11 = (softBlend << 2);
b12 = 0b00000010;

// I2C Bus initialization
delay_ms(500);
i2c_init();  
i2c_start();
i2c_write(0x20);
i2c_write(b1); i2c_write(b2); //02h   
i2c_write(0b00110010); i2c_write(0b01010000); //03h
i2c_write(b5); i2c_write(b6); //04h
i2c_write(b7); i2c_write(b8); //05h
i2c_write(b9); i2c_write(b10); //06h
i2c_write(b11);i2c_write(b12); //07h	
i2c_stop();        

// Global enable interrupts
#asm("sei")

    while (1) 
    {         
    
    if (PIND.6 == 0)   {      
    //Freq reading section            
            //read radio state
            i2c_start();
            i2c_write(0x21); //read command
            i2c_read(1);
            data = i2c_read(0);                               
            i2c_stop();
            //                        
            
             if (data >= 130)
            {                         
                data = data - 130;
                DS(writeDigit(1,1));
                delay_ms(delay_time);
                DS(writeDigit(0,2));
                delay_ms(delay_time); 
                DS(writeDigit((int)(data/10),3));
                delay_ms(delay_time); 
                DS(writeDigit((int)(data%10),4));
                delay_ms(delay_time);
            }
            else
            { 
                if (data >= 30)
                {                        
                    data = data - 30;
                    DS(writeDigit(9,2)); 
                    delay_ms(delay_time); 
                    DS(writeDigit((int)(data/10),3));
                    delay_ms(delay_time); 
                    DS(writeDigit((int)(data%10),4));
                    delay_ms(delay_time);
                }                    
                else
                {
                    DS(writeDigit(8,2)); 
                    delay_ms(delay_time);   
                    DS(writeDigit(((int)(data/10))+7,3));
                    delay_ms(delay_time); 
                    DS(writeDigit((int)(data%10),4));
                    delay_ms(delay_time);
                }
            } 
            
    //end of frequency reading section    
    }
    else    { 
    DS(writeDigit(encoderRotation,2));
    delay_ms(delay_time); 
    DS(writeDigit(encoderOldState,3));
    delay_ms(delay_time);
    DS(writeDigit(encoderNewState,4));
    delay_ms(delay_time);
    
    /*    
    //Signal quality reading section 
            i2c_start();
            i2c_write(0x21); //read command
            i2c_read(1);
            i2c_read(1);
            data = i2c_read(1);             
            i2c_stop();        
            data = data >> 1;  
            
            if (mode == 1)  {
                data = softBlend;
            }        
            //only 2 digits needed
            //DS(writeDigit((int)(data/100),2));
            //delay_ms(delay_time); 
            DS(writeDigit((int)((data%100)/10),3));
            delay_ms(delay_time);
            DS(writeDigit((int)(data%10),4));
            delay_ms(delay_time);     
            
    //end of signal quality reading
    */ 
    }
            
             
             if (PIND.5 == 0)           //mode select
             {
                if (mode < 1)   //2 modes
                    mode++; 
                else
                    mode = 0;
                delay_ms(500);
             }                                                             
    } 
                                        
}