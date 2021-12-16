/*
 * main.c
 */
#include <msp430g2452.h>
#include <stdbool.h>
//#include "msp430g2553.h"
//#include <msp430g2231.h>
#define 	CLOCKHZ		16000000
#define		nNotes		12

#define MIDI_DATA_PIN			BIT7
#define SPEAKER_PIN				BIT2
 #define WDT_DIVIDER               64
#define SCALE_SIZE		12
const unsigned long WDT_FREQUENCY = CLOCKHZ / WDT_DIVIDER;

// Poly not implemented, but this is how
// many notes can be hit at the same time
// without forgetting the first
#define MAX_POLYPHONY			4
#define MIDI_CHANNEL			0	// MIDI is 1-indexed so setting this to 0 is midi channel 1
#define USI_COUNTER_LOAD		16
//#define TABLE_SIZE				128
// Synth variables
 unsigned long periodSetting =  (128);//twice WDT_DIVIDER at the moment --- half the period is wdt per. gives 125000kHz
const unsigned int synthNotes[SCALE_SIZE] = {61156, 57334, 55041, 50964, 48925,45867,43000,40771,38223,36694,34400,32617};//a 'five limit' scale
// MIDI Variables
char currentState = 1;
char bitStateCount = 0;
char nextBitFinal = 0;
char opcode;
char midiByte;
char rawMidiByte;
char newByte = 0;
char newVelocity;
char newNote;
char notes[MAX_POLYPHONY] = {0};
char velocities[MAX_POLYPHONY] = {0};
char controllerNumber;
char controllerValue;
int noteIndex = 0;
unsigned int USIData = 0;
unsigned int wdtCounter = 0;

unsigned int wdtPeriodNotes[MAX_POLYPHONY];
    unsigned int tNotes[MAX_POLYPHONY] = {0};
    unsigned int sum = 0;


    char newNote;
        int wNoteIndex = 0;
        bool haveNewMidiByte = false;


        unsigned int midiNoteToWdtPeriod(char input);

char outVal = 1;
unsigned int index = 0;
int NoteIsOn   = 0;
unsigned int dutyCycle =0;
char  volSetting = 127;
unsigned int speedSetting = 1;
unsigned long clocksPerCycle;
unsigned long clocksPerIndex;
unsigned int speed;

unsigned int j = 1;
//char sawTable[TABLE_SIZE];
	int noteDuration = 75;// 500KHZ/75 = 4 ON/OFF CYCLES PER SECOND

void updateSynth(bool on);
char getNextByte();
void updateState();
void noteOn();
void noteOff();
void controlChange();
void updateControls();
void shiftLeft(char index);
char getNoteIndex(char note);

void mixToOutputArray();
void updateWaveTimes();
void waitForMidiLoop();
void synthProcess();
unsigned int sawtooth(unsigned int position, unsigned int wavelength);
unsigned int square(unsigned int position, unsigned int wavelength);
void main(void)
{
	volatile unsigned long i;

 	DCOCTL = CALDCO_16MHZ;
	BCSCTL1 = CALBC1_16MHZ;
	BCSCTL2 |= DIVS_2;						// SMCLK = MCLK / 4 = 4MHZ
	WDTCTL = WDTPW + WDTTMSEL + WDTIS1;// + WDTIS0;
	       IE1 |= WDTIE;
	       _EINT();
	      //set wdt as interval timer
	      // _BIS_SR(GIE);							// enable interrupts
	P1DIR |= BIT2 + BIT0 + BIT6;           	// P1.2 and P1.3 output
	P1OUT = 0;
	CCTL1 = OUTMOD_7;						// CCR1 reset/set
	TACTL = TASSEL_2 + MC_1 + ID_3;         // SMCLK, up mode, /8 = 500KHz
	TACCTL0 |= CCIE;
	CCR0 =1;
	P1SEL |= 0x0C;                            // P1.2 and P1.3 TA1/2 options
	USICKCTL |= USISSEL_3;					// USI CLOCK SOURCE = SMCLK = 16MHZ
	USICKCTL |= USIDIV_5;//7				// DIVIDES SMCLK BY 32 = 31250 * 4 = MIDI SPEC * 4x oversample
	USICTL0 |= USIMST;						// USE THE INTERNAL CLOCK
	USICTL0 |= USIPE7;						// ENABLE USI INPUT PORT 1.7
	USICTL0 |= USILSB;
	USICTL0 &= ~USISWRST;					// START HARDWARE SERIAL RECIEVE
	USICNT |= USI16B;

	USICTL1 |= USIIE;						// INTERRUPT WHEN SERIAL CACHE FULL
	//
  while(1){
	 // waitForMidiLoop();//
	  synthProcess();

	opcode = getNextByte();
  	if(opcode == (0x90 | MIDI_CHANNEL))
  	{
  		noteOn();

  	}

  	else if(opcode == (0x80 | MIDI_CHANNEL))

  		noteOff();

  	else if(opcode == (0xB0 | MIDI_CHANNEL))
  		{
  		controlChange();
  	}
  }
}
void waitForMidiLoop(){
	while(!newByte)
		synthProcess();
}

#pragma vector=USI_VECTOR
__interrupt void USI(){
	USICNT |= USI_COUNTER_LOAD;
	USIData = USISR;

	char samplingBit;
	
	for(samplingBit = 0; samplingBit < 16; samplingBit++){
		if((USIData & BIT0) ^ currentState){  // if they're not the same
			if(bitStateCount > 2){
				rawMidiByte >>= 1;
				if(currentState)
					rawMidiByte |= BIT7;
				if(nextBitFinal){
					newByte = 1;
					midiByte = rawMidiByte;
					rawMidiByte = 0xFF;
					nextBitFinal = 0;
				}
			}
			currentState ^= BIT0;
			bitStateCount = 1;

		}
		else{
			bitStateCount++;
			if(bitStateCount == 4){
				rawMidiByte >>= 1;
				if(currentState)
					rawMidiByte |= BIT7;
				if(nextBitFinal){
					newByte = 1;
					midiByte = rawMidiByte;
					rawMidiByte = 0xFF;
					nextBitFinal = 0;
				}
				bitStateCount = 0;
			}
		}
		USIData >>= 1;
		if(!newByte && (~rawMidiByte & BIT0))
			nextBitFinal = 1;
	}
}

void noteOn(){
	do{
		newNote = getNextByte();

		if(newNote & 0x80){

			newByte = 1;
			break;
		}

		newVelocity = getNextByte();
		if(newVelocity & 0x80){
			newByte = 1;
			break;
		}
		             //newByte =0;
		             updateState();
	}while(1);
}

void noteOff(){
	do{
		newNote = getNextByte();

		if(newNote & 0x80){
					newByte = 1;
					break;
				}
		newVelocity = 0;
		//newByte = 0;
		updateState();
	}while(1);
}

void controlChange(){
	do{
		controllerNumber = getNextByte();
		if(controllerNumber & 0x80){
			newByte = 1;
			break;
		}
		controllerValue = getNextByte();
		if(controllerValue & 0x80){
			newByte = 1;
			break;
		}
		updateControls();
	}while(1);
}

void updateControls(){
	// stub
//if(controllerNumber == 7){//controller 7 == slider
//	volSetting = controllerValue;
//	if (volSetting == 0)
//		volSetting = 1;
//}
//if(controllerNumber ==1){// controller 1 == mod wheel
//	speedSetting = controllerValue;

//	if(speedSetting == 0)
//		speedSetting =1;
	//updateSpeedVals();
//}
// do something with controllerValue
}
// ##################################################
    // ################# Synth Stuff ####################

    void synthProcess(){
      // if(noteIndex){
          updateWaveTimes();
          //if(newByte)
         //    return;
          mixToOutputArray();
        //  updateSynth(1);
        //  if(newByte)
        //    return;
    //      if(!I2C_State){
         // updateState();
    //      }
   //    }
    }

    // basically tracks our position within the waveform
    void updateWaveTimes(){
    	static unsigned int lastWdt = 0;
       char iNote;

       if(lastWdt != wdtCounter){
          unsigned int wdtDelta = wdtCounter - lastWdt;
          lastWdt = wdtCounter;

          for(iNote = 0; iNote < noteIndex; iNote++){
             tNotes[iNote] += wdtDelta;

             if(tNotes[iNote] >= wdtPeriodNotes[iNote]){
                tNotes[iNote] = tNotes[iNote] - wdtPeriodNotes[iNote];
             }
          }
       }
    }
unsigned int sawtooth(unsigned int position, unsigned int wavelength){
           return (unsigned long)position * 0x0FFF / wavelength;
        }
unsigned int square(unsigned int position, unsigned int wavelength){
       if(position > wavelength >> 1)
          return 0x0FFF;
       else
          return 0;
    }

// A fast but lo-fi digital mixer.
    void mixToOutputArray(){
       char iSum;
       //sum=0;
       for(iSum = 0; iSum < noteIndex; iSum++)
          sum += square(tNotes[iSum], wdtPeriodNotes[iSum]);
          // using waveforms other than square takes extra cpu time and can lead to unexpected results,
          // but they're there to mess with.  (you may miss midi events and get crudd(y/ier) sound)
    //      sum += triangle(tNotes[iSum], wdtPeriodNotes[iSum]);
    //      sum += sawtooth(tNotes[iSum], wdtPeriodNotes[iSum]);

          // use this one for velocity sensitivity
    //      sum += square(tNotes[iSum], wdtPeriodNotes[iSum], velocities[iSum]);

      //Get ccr`1 = sum/periodSetting?
       // Hack to keep the volume more level
             if(noteIndex == 1)
                sum /= 2;
             else
                sum /= noteIndex;
            dutyCycle = sum>>9;// & 0x7F ;
            P1OUT |= BIT6;
    }

    unsigned int midiNoteToWdtPeriod(char midiNote){
       return ((WDT_FREQUENCY) / (synthNotes[midiNote % 12] >> ((127 - midiNote) / 12)));
    }

char getNextByte(){
	char nextByte;
	do{
		P1OUT &= ~BIT0;
		while(!newByte){
		}
		newByte = 0;
		nextByte = midiByte;
	}while((nextByte == 0xFE) || (nextByte == 0xFF));
	P1OUT |= BIT0;
	return nextByte;
}
#pragma vector=WDT_VECTOR
    __interrupt void watchdog_timer(void){
       wdtCounter++;
       }
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0 (void){

	CCR0 =periodSetting;

	CCR1 = ( dutyCycle);///temp); // divide period by 2 the 0-127 for vol control thru pwm
	//updateSynth(1);// turn out on here?
}
//#pragma vector=TIMER0_A0 VECTOR __interrupt void Timer_A0 (void){	CCR0 = period Setting;	CCR1 = (CCR0 >> 1); }
void updateState(){
	if(noteIndex == 0 && newVelocity != 0){
		notes[0] = newNote;
		velocities[0] = newVelocity;
		wdtPeriodNotes[0] = midiNoteToWdtPeriod(newNote);
		//synthProcess();//
		updateSynth(1);
		noteIndex++;
	}
	else{
		if(newVelocity != 0){
			// add a new note when the poly cache is full, replacing the oldest
			if(MAX_POLYPHONY == noteIndex){
				shiftLeft(0);
				notes[MAX_POLYPHONY - 1] = newNote;
				velocities[MAX_POLYPHONY - 1] = newVelocity;
				wdtPeriodNotes[MAX_POLYPHONY - 1] = midiNoteToWdtPeriod(newNote);
				//synthProcess();//
				updateSynth(1);
			}
			// add a new note
			else{
				notes[noteIndex] = newNote;
				velocities[noteIndex] = newVelocity;
				wdtPeriodNotes[noteIndex] = midiNoteToWdtPeriod(newNote);
				//synthProcess();//
				updateSynth(1);
				noteIndex++;
			}
		}
		else if(getNoteIndex(newNote) < MAX_POLYPHONY){
			shiftLeft(getNoteIndex(newNote));
			noteIndex -= 2;
			if(noteIndex >= 0){
				//synthProcess();//
				updateSynth(1);
				noteIndex++;
			}
			else{
				//synthProcess();
				updateSynth(0);
				noteIndex = 0;
			}
		}
	}
}

void shiftLeft(char index){
	int i;
	for(i = index; i < MAX_POLYPHONY - 1; i++){
		notes[i] = notes[i + 1];
		velocities[i] = velocities[i + 1];
		wdtPeriodNotes[i] = wdtPeriodNotes[i +1];
	}
}

char getNoteIndex(char note){
	int i;
	for(i = 0; i < MAX_POLYPHONY; i++)
		if(notes[i] == note)
			return i;
	return MAX_POLYPHONY + 1;
}

void updateSynth(bool on){
	if(on){
	P1SEL |= 0x0C;
		//P1OUT |= BIT6;
	}
	else{
		P1SEL &= ~0x0C;
		P1OUT &= ~BIT6;
	}
		//if(noteIndex >= 0 && noteIndex < MAX_POLYPHONY)
		//	dutyCycle = (sum>>4);
}


