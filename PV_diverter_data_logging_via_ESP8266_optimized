/* February 2024
* Storing the copy of code, only for quick lookup of code file which woeks on my PCB design
* Credits to:  Robin Emley , for original code, Thank you
*/
#include <Arduino.h> 
#include <Wire.h>
#include <SoftwareSerial.h>

#define ESP8266_PRESENT // <- this line should be commented out if the RFM12B module is not present

#ifdef ESP8266_PRESENT
  #define RX 8
  #define TX 2
  #include <SoftwareSerial.h>
  //declaring software objects in class
  SoftwareSerial esp(RX,TX);  //8=rx   2=tx     
#endif

// Physical constants, please do not change!
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define JOULES_PER_WATT_HOUR 3600 //  (0.001 kWh = 3600 Joules)

// -----------------------------------------------------
// Change these values to suit the local mains frequency and supply meter
#define CYCLES_PER_SECOND 50 
#define WORKING_RANGE_IN_JOULES 3600 
#define REQUIRED_EXPORT_IN_WATTS 0 // when set to a negative value, this acts as a PV generator 

// --------------------------
// Dallas DS18B20 commands
#define SKIP_ROM 0xcc 
#define CONVERT_TEMPERATURE 0x44
#define READ_SCRATCHPAD 0xbe
#define BAD_TEMPERATURE 30000 // this value (300C) is sent if no sensor is present

// ----------------
// general literals
#define DATALOG_PERIOD_IN_MAINS_CYCLES  500 
// #define POST_DATALOG_EVENT_DELAY_MILLIS 40
#define ANTI_CREEP_LIMIT 0 // <- to prevent the diverted energy total from 'creeping'
                           // in Joules per mains cycle (has no effect when set to 0)

// to prevent the diverted energy total from 'creeping'
#define ANTI_CREEP_LIMIT 5 // in Joules per mains cycle (has no effect when set to 0)
long antiCreepLimit_inIEUperMainsCycle;

// -------------------------------
// definitions of enumerated types
enum polarities {NEGATIVE, POSITIVE};
enum loadStates {LOAD_ON, LOAD_OFF}; // the external trigger device is active low
enum outputModes {ANTI_FLICKER, NORMAL}; // retained for compatibility with previous versions.

// ----  Output mode selection -----
enum outputModes outputMode = ANTI_FLICKER; // <- needs to be set here unless an                                                  
//enum outputModes outputMode = NORMAL;     //    external switch is in use                                              

/* --------------------------------------*/

#ifdef ESP8266_PRESENT 
  unsigned long sendDataPrevMillis = 0;
  unsigned long timerDelay = 9000;
#endif

typedef struct { 
  int powerAtSupplyPoint_Watts; // import = +ve, to match OEM convention
  int divertedEnergyTotal_Wh; // always positive
  int divertedPower_Watts; // always positive
  int Vrms_times100;
  int temperature_times100;
} Tx_struct; 
Tx_struct tx_data; 

// allocation of digital pins when pin-saving hardware is in use
// *************************************************************
// D0 & D1 are reserved for the Serial i/f

const byte tempSensorPin = 3; // <-- the "mode" port 
const byte outputForTrigger = 4;   

// allocation of analogue pins
/*****************************************************************************************/
const byte voltageSensor = 3;          // A3 is for the voltage sensor
const byte currentSensor_diverted = 4; // A4 is for CT2 which measures diverted current
const byte currentSensor_grid = 5;     // A5 is for CT1 which measures grid current

const byte delayBeforeSerialStarts = 5;  // in seconds, to allow Serial window to be opened
const byte startUpPeriod = 3;  // in seconds, to allow LP filter to settle
const int DCoffset_I = 512;    // nominal mid-point value of ADC @ x1 scale

/* -------------------------------------------------------------------------------------
 * Global variables that are used in multiple blocks so cannot be static.
 * For integer maths, many variables need to be 'long'
 */
boolean beyondStartUpPhase = false;     // start-up delay, allows things to settle
long energyInBucket_long = 0; // in Integer Energy Units (for controlling the dump-load) 
long capacityOfEnergyBucket_long;  // depends on powerCal, frequency & the 'sweetzone' size.
long lowerEnergyThreshold_long;    // for turning load off
long upperEnergyThreshold_long;    // for turning load on
int phaseCal_grid_int;             // to avoid the need for floating-point maths
int phaseCal_diverted_int;         // to avoid the need for floating-point maths
long DCoffset_V_long;              // <--- for LPF
long DCoffset_V_min;               // <--- for LPF
long DCoffset_V_max;               // <--- for LPF

long divertedEnergyRecent_IEU = 0; // Hi-res accumulator of limited range
unsigned int divertedEnergyTotal_Wh = 0; // WattHour register of 63K range
long IEU_per_Wh; // depends on powerCal, frequency & the 'sweetzone' size.
unsigned long displayShutdown_inMainsCycles; 
unsigned long absenceOfDivertedEnergyCount = 0;
float IEUtoJoulesConversion_CT1;

float offsetOfEnergyThresholdsInAFmode = 0.1; // <-- not wise to exceeed 0.4

long sumP_forEnergyBucket;        // for per-cycle summation of 'real power' 
long sumP_forDivertedEnergy;              // for per-cycle summation of diverted energy
long sumP_atSupplyPoint;         // for summation of 'real power' values during datalog period 
long sumP_forDivertedPower;              // for summation of diverted power values during datalog period 
long sum_Vsquared;                // for summation of V^2 values during datalog period            
int sampleSetsDuringThisCycle;    // for counting the sample sets during each mains cycle
long sampleSetsDuringThisDatalogPeriod; // for counting the sample sets during each datalogging period

long cumVdeltasThisCycle_long;    // for the LPF which determines DC offset (voltage)
long lastSampleVminusDC_long;     //    for the phaseCal algorithm
int cycleCountForDatalogging = 0;  
long sampleVminusDC_long;
long requiredExportPerMainsCycle_inIEU;

// for interaction between the main code and the ISR
volatile boolean datalogEventPending = false;
volatile boolean newMainsCycle = false;
volatile long copyOf_sumP_atSupplyPoint;          
volatile long copyOf_sumP_forDivertedPower;          
volatile long copyOf_sum_Vsquared;
volatile long copyOf_divertedEnergyTotal_Wh;          
volatile int copyOf_lowestNoOfSampleSetsPerMainsCycle;
volatile long copyOf_sampleSetsDuringThisDatalogPeriod;

// For an enhanced polarity detection mechanism, which includes a persistence check
#define PERSISTENCE_FOR_POLARITY_CHANGE 2
enum polarities polarityOfMostRecentVsample;   
enum polarities polarityConfirmed;  // for zero-crossing detection
enum polarities polarityConfirmedOfLastSampleV;  // for zero-crossing detection

// For a mechanism to check the integrity of this code structure
int lowestNoOfSampleSetsPerMainsCycle;
unsigned long timeAtLastDelay;

/******************************************************************************************************/
const float powerCal_grid = 0.062;  
const float powerCal_diverted = 0.062;  
/*********************************************************************************************************/
const float  phaseCal_grid = 1.0; 
const float  phaseCal_diverted = 1.0;  


// For datalogging purposes, voltageCal has been included too.  When running at 
// 230 V AC, the range of ADC values will be similar to the actual range of volts, 
// so the optimal value for this cal factor will be close to unity.  
//
const float voltageCal = 1.0; 
#define DISPLAY_SHUTDOWN_IN_HOURS 8 // auto-reset after this period of inactivity

volatile boolean EDD_isActive = false; // energy diversion detection
//volatile boolean EDD_isActive = true; // energy diversion detection


void setup(){  
  //start hardware serial coms
  Serial.begin(9600);
  //start software serial coms
  esp.begin(9600);
  //declare pin functions
  pinMode (RX,INPUT);
  pinMode (TX,OUTPUT);
  pinMode(outputForTrigger, OUTPUT);  
  digitalWrite (outputForTrigger, LOAD_OFF); // the external trigger is active low

  delay(delayBeforeSerialStarts * 1000); // allow time to open Serial monitor      
  
  Serial.println();
  Serial.println("-------------------------------------");
  Serial.println("Sketch ID:   Mk2_RFdatalog_7.ino");
  Serial.println();

  phaseCal_grid_int = phaseCal_grid * 256; // for integer maths
  phaseCal_diverted_int = phaseCal_diverted * 256; // for integer maths     

  // For the flow of energy at the 'grid' connection point (CT1): 
  capacityOfEnergyBucket_long = (long)WORKING_RANGE_IN_JOULES * CYCLES_PER_SECOND * (1/powerCal_grid);
  
  IEUtoJoulesConversion_CT1 = powerCal_grid / CYCLES_PER_SECOND; // may be useful

  IEU_per_Wh = 
     (long)JOULES_PER_WATT_HOUR * CYCLES_PER_SECOND * (1/powerCal_diverted); 
 
  // to avoid the diverted energy accumulator 'creeping' when the load is not active
  antiCreepLimit_inIEUperMainsCycle = (float)ANTI_CREEP_LIMIT * (1/powerCal_diverted);

  long mainsCyclesPerHour = (long)CYCLES_PER_SECOND * 
                             SECONDS_PER_MINUTE * MINUTES_PER_HOUR;                           
  displayShutdown_inMainsCycles = DISPLAY_SHUTDOWN_IN_HOURS * mainsCyclesPerHour;                           
      
  requiredExportPerMainsCycle_inIEU = (long)REQUIRED_EXPORT_IN_WATTS * (1/powerCal_grid); 

  // Define operating limits for the LP filter which identifies DC offset in the voltage 
  // sample stream.  By limiting the output range, the filter always should start up 
  // correctly.
  DCoffset_V_long = 512L * 256; // nominal mid-point value of ADC @ x256 scale  
  DCoffset_V_min = (long)(512L - 100) * 256; // mid-point of ADC minus a working margin
  DCoffset_V_max = (long)(512L + 100) * 256; // mid-point of ADC plus a working margin

  Serial.println ("ADC mode:       free-running");
  
  // Set up the ADC to be free-running 
  ADCSRA  = (1<<ADPS0)+(1<<ADPS1)+(1<<ADPS2);  // Set the ADC's clock to system clock / 128
  ADCSRA |= (1 << ADEN);                 // Enable the ADC 
  
  ADCSRA |= (1<<ADATE);  // set the Auto Trigger Enable bit in the ADCSRA register.  Because 
                         // bits ADTS0-2 have not been set (i.e. they are all zero), the 
                         // ADC's trigger source is set to "free running mode".
                         
  ADCSRA |=(1<<ADIE);    // set the ADC interrupt enable bit. When this bit is written 
                         // to one and the I-bit in SREG is set, the 
                         // ADC Conversion Complete Interrupt is activated. 

  ADCSRA |= (1<<ADSC);   // start ADC manually first time 
  sei();                 // Enable Global Interrupts  

  Serial.print ( "Output mode:    ");
  if (outputMode == NORMAL) {
    Serial.println ( "normal"); }
  else 
  {  
    Serial.println ( "anti-flicker");
    Serial.print ( "  offsetOfEnergyThresholds  = ");
    Serial.println ( offsetOfEnergyThresholdsInAFmode);    
  }
    
  Serial.print ( "powerCal_CT1, for grid consumption =      "); Serial.println (powerCal_grid,4);
  Serial.print ( "powerCal_CT2, for diverted power  =      "); Serial.println (powerCal_diverted,4);
  Serial.print ( "voltageCal, for Vrms  =      "); Serial.println (voltageCal,4);
  
  Serial.print ("Anti-creep limit (Joules / mains cycle) = ");
  Serial.println (ANTI_CREEP_LIMIT);
  Serial.print ("Export rate (Watts) = ");
  Serial.println (REQUIRED_EXPORT_IN_WATTS);
  
  Serial.print ("zero-crossing persistence (sample sets) = ");
  Serial.println (PERSISTENCE_FOR_POLARITY_CHANGE);

  Serial.print(">>free RAM = ");
  Serial.println(freeRam());  // a useful value to keep an eye on
  configureParamsForSelectedOutputMode(); 
  Serial.println ("----");    
}

void loop(){ 
  //  unsigned long timeNow = millis();
  static byte perSecondTimer = 0;
  //  
  // The ISR provides a 50 Hz 'tick' which the main code is free to use.
  if (newMainsCycle)
  {
    newMainsCycle = false;
    perSecondTimer++;
    
    if(perSecondTimer >= CYCLES_PER_SECOND) 
    {       
      perSecondTimer = 0; 
      
      // After a pre-defined period of inactivity, the 4-digit display needs to 
      // close down in readiness for the next's day's data. 
      //
      if (absenceOfDivertedEnergyCount > displayShutdown_inMainsCycles)
      {
        // Clear the accumulators for diverted energy.  These are the "genuine" 
        // accumulators that are used by ISR rather than the copies that are 
        // regularly made available for use by the main code.
        //
        divertedEnergyTotal_Wh = 0;
        divertedEnergyRecent_IEU = 0;
        EDD_isActive = false; // energy diversion detector is now inactive
      }
      
      //configureValueForDisplay(); // this timing is not critical so does not need to be in the ISR
    }  
  }
  
  if (datalogEventPending){
    datalogEventPending= false; 
    // To provide sufficient range for a dataloging period of 10 seconds, the accumulators for grid power 
    // and diverted power are now scaled at 1/16 of their previous V_ADC * I_ADC values. Hence the * 16 factor 
    // that appears below.
    // Similarly, the accumulator for Vsquared is now scaled at 1/16 of its previous  V_ADC * V_ADC value. 
    // Hence the * 4 factor that appears below after the sqrt() operation.
    //    
    tx_data.powerAtSupplyPoint_Watts = copyOf_sumP_atSupplyPoint * powerCal_grid / copyOf_sampleSetsDuringThisDatalogPeriod * 16;
    tx_data.powerAtSupplyPoint_Watts *= -1; // to match the OEM convention (import is =ve; export is -ve)
    tx_data.divertedEnergyTotal_Wh = copyOf_divertedEnergyTotal_Wh;    
    tx_data.divertedPower_Watts = copyOf_sumP_forDivertedPower * powerCal_diverted / copyOf_sampleSetsDuringThisDatalogPeriod * 16;
    tx_data.Vrms_times100 = (int)(100 * voltageCal * sqrt(copyOf_sum_Vsquared / copyOf_sampleSetsDuringThisDatalogPeriod) * 4);
    //tx_data.temperature_times100 = readTemperature();
    
    #ifdef ESP8266_PRESENT
      send_esp8266_data();         
    #endif            
    
    Serial.print("grid power ");  Serial.print(tx_data.powerAtSupplyPoint_Watts);
    Serial.print(", diverted energy (Wh) ");  Serial.print(tx_data.divertedEnergyTotal_Wh);
    Serial.print(", diverted power ");  Serial.print(tx_data.divertedPower_Watts);
    Serial.print(", Vrms ");  Serial.print((float)tx_data.Vrms_times100 / 100);
    Serial.print(", temperature ");  Serial.print((float)tx_data.temperature_times100 / 100);
    Serial.print("  [minSampleSets/MC ");  Serial.print(copyOf_lowestNoOfSampleSetsPerMainsCycle);
    //    Serial.print(",  #ofSampleSets "); Serial.print(copyOf_sampleSetsDuringThisDatalogPeriod);
    Serial.println(']');
    //    delay(POST_DATALOG_EVENT_DELAY_MILLIS);
    //convertTemperature(); // for use next time around    
  }  

}

/*
 * This Interrupt Service Routine looks after the acquisition and processing of
 * raw samples from the ADC sub-processor.  By means of various helper functions, all of 
 * the time-critical activities are processed within the ISR.  The main code is notified
 * by means of a flag when fresh copies of loggable data are available.
 */
ISR(ADC_vect){                                         
  static unsigned char sample_index = 0;
  int rawSample;
  long sampleIminusDC;
  long  phaseShiftedSampleVminusDC;
  long filtV_div4;
  long filtI_div4;
  long instP;
  long inst_Vsquared;
   
  switch(sample_index) { 
    case 0:
      rawSample = ADC;                    // store the ADC value (this one is for Voltage)
      ADMUX = 0x40 + currentSensor_diverted;  // the conversion for I_grid is already under way
      sample_index++;                   // increment the control flag
      //
      lastSampleVminusDC_long = sampleVminusDC_long;  // required for phaseCal algorithm
      sampleVminusDC_long = ((long)rawSample<<8) - DCoffset_V_long; 
      if(sampleVminusDC_long > 0) { 
        polarityOfMostRecentVsample = POSITIVE; }
      else { 
        polarityOfMostRecentVsample = NEGATIVE; }
      confirmPolarity();
      //  
      checkProgress(); // deals with aspects that only occur at particular stages of each mains cycle
      //
      // for the Vrms calculation (for datalogging only)
      filtV_div4 = sampleVminusDC_long>>2;  // reduce to 16-bits (x64, or 2^6)
      inst_Vsquared = filtV_div4 * filtV_div4; // 32-bits (x4096, or 2^12)
      //      inst_Vsquared = inst_Vsquared>>12;     // 20-bits (x1), not enough range :-(
      inst_Vsquared = inst_Vsquared>>16;     // 16-bits (x1/16, or 2^-4) for more datalog range
      sum_Vsquared += inst_Vsquared; // scaling is x1/16
      sampleSetsDuringThisDatalogPeriod++; 
      //  
      // store items for use during next loop
      cumVdeltasThisCycle_long += sampleVminusDC_long; // for use with LP filter
      //      lastSampleVminusDC_long = sampleVminusDC_long;  // required for phaseCal algorithm
      polarityConfirmedOfLastSampleV = polarityConfirmed;  // for identification of half cycle boundaries
      sampleSetsDuringThisCycle++;  // for real power calculations
      //refreshDisplay();
    break;
    case 1:
      rawSample = ADC;               // store the ADC value (this one is for Grid Current)
      ADMUX = 0x40 + voltageSensor;  // the conversion for I_diverted is already under way
      sample_index++;                   // increment the control flag
      //
      // remove most of the DC offset from the current sample (the precise value does not matter)
      sampleIminusDC = ((long)(rawSample-DCoffset_I))<<8;
      //
      // phase-shift the voltage waveform so that it aligns with the grid current waveform
      phaseShiftedSampleVminusDC = lastSampleVminusDC_long
         + (((sampleVminusDC_long - lastSampleVminusDC_long)*phaseCal_grid_int)>>8);  
      //                                                          
      // calculate the "real power" in this sample pair and add to the accumulated sum
      filtV_div4 = phaseShiftedSampleVminusDC>>2;  // reduce to 16-bits (x64, or 2^6)
      filtI_div4 = sampleIminusDC>>2; // reduce to 16-bits (x64, or 2^6)
      instP = filtV_div4 * filtI_div4;  // 32-bits (x4096, or 2^12)
      instP = instP>>12;     // reduce to 20-bits (x1)     
      sumP_forEnergyBucket+=instP; // scaling is x1
      //
      instP = instP>>4;     // reduce to 16-bits (x1/16, or 2^-4) for more datalog range      
      sumP_atSupplyPoint +=instP; // scaling is x1/16
    break;
    case 2:
      rawSample = ADC;               // store the ADC value (this one is for Diverted Current)
      ADMUX = 0x40 + currentSensor_grid;  // the conversion for Voltage is already under way
      sample_index = 0;                   // reset the control flag
      //       
      // remove most of the DC offset from the current sample (the precise value does not matter)
      sampleIminusDC = ((long)(rawSample-DCoffset_I))<<8;
      //
      // phase-shift the voltage waveform so that it aligns with the diverted current waveform
      phaseShiftedSampleVminusDC = lastSampleVminusDC_long
         + (((sampleVminusDC_long - lastSampleVminusDC_long)*phaseCal_diverted_int)>>8);  
      //
      // calculate the "real power" in this sample pair and add to the accumulated sum
      filtV_div4 = phaseShiftedSampleVminusDC>>2;  // reduce to 16-bits (x64, or 2^6)
      filtI_div4 = sampleIminusDC>>2; // reduce to 16-bits (x64, or 2^6)
      instP = filtV_div4 * filtI_div4;  // 32-bits (x4096, or 2^12)
      instP = instP>>12;     // reduce to 20-bits (x1)  
      sumP_forDivertedEnergy +=instP; // scaling is x1
      //
      instP = instP>>4;     // reduce to 16-bits (x1/16, or 2^-4) for more datalog range      
      sumP_forDivertedPower +=instP; // scaling is x1/16
    break;
    default:
      sample_index = 0;                 // to prevent lockup (should never get here)      
  }
} 

void checkProgress()
/* 
 * This routine is called by the ISR when each voltage sample becomes available. 
 * At the start of each new mains cycle, another helper function is called.  
 * All other processing is done within this function.
 */
{  
  static enum loadStates nextStateOfLoad = LOAD_OFF;  

  if (polarityConfirmed == POSITIVE) 
  { 
    if (polarityConfirmedOfLastSampleV != POSITIVE)
    {
      if (beyondStartUpPhase)
      {     
        // The start of a new mains cycle, just after the +ve going zero-crossing point.    
        
        // a simple routine for checking the performance of this new ISR structure     
        if (sampleSetsDuringThisCycle < lowestNoOfSampleSetsPerMainsCycle) {
          lowestNoOfSampleSetsPerMainsCycle = sampleSetsDuringThisCycle; }

        processLatestContribution(); // for activities at the start of each new mains cycle        
      }  
      else
      {  
        // wait until the DC-blocking filters have had time to settle
        if(millis() > (delayBeforeSerialStarts + startUpPeriod) * 1000) 
        {
          beyondStartUpPhase = true;
          sumP_forEnergyBucket = 0;
          sumP_atSupplyPoint = 0;
          sumP_forDivertedEnergy = 0;
          sumP_forDivertedPower = 0;
          sampleSetsDuringThisCycle = 0; // not yet dealt with for this cycle
          sampleSetsDuringThisDatalogPeriod = 0;
          // can't say "Go!" here 'cos we're in an ISR!
        }
      }   
    } // end of processing that is specific to the first Vsample in each +ve half cycle
    
    // still processing samples where the voltage is POSITIVE ...
    // check to see whether the trigger device can now be reliably armed
    // 
    if (sampleSetsDuringThisCycle == 5) // part way through the +ve half cycle
    {
      if (beyondStartUpPhase)
      {           
        if (energyInBucket_long < lowerEnergyThreshold_long) {
          // when below the lower threshold, always set the load to "off" 
          nextStateOfLoad = LOAD_OFF; }
        else
        if (energyInBucket_long > upperEnergyThreshold_long) {
          // when above the upper threshold, always set the load to "off"
          nextStateOfLoad = LOAD_ON; }
        else { 
        } // leave the load's state unchanged (hysteresis)
                                        
        // set the Arduino's output pin accordingly
        digitalWrite(outputForTrigger, nextStateOfLoad);   
      
        // update the Energy Diversion Detector
        if (nextStateOfLoad == LOAD_ON) {
          absenceOfDivertedEnergyCount = 0; 
          EDD_isActive = true; }            
        else {
          absenceOfDivertedEnergyCount++; }   
      }    
    }
  } // end of processing that is specific to samples where the voltage is positive
  
  else // the polatity of this sample is negative
  {     
    if (polarityConfirmedOfLastSampleV != NEGATIVE)
    {
      // This is the start of a new -ve half cycle (just after the zero-crossing point)      
      // which is a convenient point to update the Low Pass Filter for DC-offset removal
      //  The portion which is fed back into the integrator is approximately one percent
      // of the average offset of all the Vsamples in the previous mains cycle.
      //
      long previousOffset = DCoffset_V_long;
      DCoffset_V_long = previousOffset + (cumVdeltasThisCycle_long>>12); 
      cumVdeltasThisCycle_long = 0;
      
      // To ensure that the LPF will always start up correctly when 230V AC is available, its
      // output value needs to be prevented from drifting beyond the likely range of the 
      // voltage signal.  This avoids the need for a HPF as was done for initial Mk2 builds.
      //
      if (DCoffset_V_long < DCoffset_V_min) {
        DCoffset_V_long = DCoffset_V_min; }
      else  
      if (DCoffset_V_long > DCoffset_V_max) {
        DCoffset_V_long = DCoffset_V_max; }
        
//      checkOutputModeSelection(); // updates outputMode if the external switch is in use
           
    } // end of processing that is specific to the first Vsample in each -ve half cycle
  } // end of processing that is specific to samples where the voltage is negative
} //  end of checkProgress()

void confirmPolarity()
{
  /* This routine prevents a zero-crossing point from being declared until 
   * a certain number of consecutive samples in the 'other' half of the 
   * waveform have been encountered.  
   */ 
  static byte count = 0;
  if (polarityOfMostRecentVsample != polarityConfirmedOfLastSampleV) { 
    count++; }  
  else {
    count = 0; }
    
  if (count > PERSISTENCE_FOR_POLARITY_CHANGE)
  {
    count = 0;
    polarityConfirmed = polarityOfMostRecentVsample;
  }
}

/* 
 * This routine runs once per mains cycle.  It forms part of the ISR.
 */
void processLatestContribution(){
  newMainsCycle = true; // <--  a 50 Hz 'tick' for use by the main code  

  // For the mechanism which controls the diversion of surplus power, the AVERAGE power 
  // at the 'grid' point during the previous mains cycle must be quantified. The first 
  // stage in this process is for the sum of all instantaneous power values to be divided 
  // by the number of sample sets that have contributed to its value.  A similar operation 
  // is required for the diverted power data. 
  //
  // The next stage would normally be to apply a calibration factor so that real power 
  // can be expressed in Watts.  That's fine for floating point maths, but it's not such
  // a good idea when integer maths is being used.  To keep the numbers large, and also 
  // to save time, calibration of power is omitted at this stage.  Real Power (stored as 
  // a 'long') is therefore (1/powerCal) times larger than the actual power in Watts.
  //
  long realPower_for_energyBucket  = sumP_forEnergyBucket / sampleSetsDuringThisCycle; 
  long realPower_diverted = sumP_forDivertedEnergy / sampleSetsDuringThisCycle; 
  //
  // The per-mainsCycle variables can now be reset for ongoing use 
  sampleSetsDuringThisCycle = 0;
  sumP_forEnergyBucket = 0;
  sumP_forDivertedEnergy = 0;


  // Next, the energy content of this power rating needs to be determined.  Energy is 
  // power multiplied by time, so the next step is normally to multiply the measured
  // value of power by the time over which it was measured.
  //   Average power is calculated once every mains cycle. When integer maths is 
  // being used, a repetitive power-to-energy conversion seems an unnecessary workload.  
  // As all sampling periods are of similar duration, it is more efficient simply to 
  // add all of the power samples together, and note that their sum is actually 
  // CYCLES_PER_SECOND greater than it would otherwise be.
  //   Although the numerical value itself does not change, I thought that a new name 
  // may be helpful so as to minimise confusion.  
  //   The 'energy' variables below are CYCLES_PER_SECOND * (1/powerCal) times larger than 
  // their actual values in Joules.
  
  long realEnergy_for_energyBucket = realPower_for_energyBucket; 
  long realEnergy_diverted = realPower_diverted; 
          
  // The latest energy contribution from the grid connection point can now be added
  // to the energy bucket which determines the state of the dump-load.  

  energyInBucket_long += realEnergy_for_energyBucket;   
  energyInBucket_long -= requiredExportPerMainsCycle_inIEU; // <- useful for PV simulation
         
  // Apply max and min limits to the bucket's level.  This is to ensure correct operation
  // when conditions change, i.e. when import changes to export, and vici versa.
  //
  if (energyInBucket_long > capacityOfEnergyBucket_long) { 
    energyInBucket_long = capacityOfEnergyBucket_long; } 
  else         
  if (energyInBucket_long < 0) {
    energyInBucket_long = 0; }           
 
               
  if (EDD_isActive) // Energy Diversion Display
  {
    // For diverted energy, the latest contribution needs to be added to an 
    // accumulator which operates with maximum precision.  To avoid the displayed
    // value from creeping, any small contributions which are likely to be 
    // caused by noise are ignored.
     
    if (realEnergy_diverted > antiCreepLimit_inIEUperMainsCycle) {
      divertedEnergyRecent_IEU += realEnergy_diverted; }
      
    // Whole Watt-Hours are then recorded separately
    if (divertedEnergyRecent_IEU > IEU_per_Wh)
    {
      divertedEnergyRecent_IEU -= IEU_per_Wh;
      divertedEnergyTotal_Wh++;
    }  
  }

  /* At the end of each datalogging period, copies are made of the relevant variables
   * for use by the main code.  These variable are then reset for use during the next 
   * datalogging period.
   */       
  cycleCountForDatalogging ++;       
  if (cycleCountForDatalogging  >= DATALOG_PERIOD_IN_MAINS_CYCLES ) 
  { 
    cycleCountForDatalogging = 0;
    
    copyOf_sumP_atSupplyPoint = sumP_atSupplyPoint;
    copyOf_sumP_forDivertedPower = sumP_forDivertedPower;
    copyOf_divertedEnergyTotal_Wh = divertedEnergyTotal_Wh;
    copyOf_sum_Vsquared = sum_Vsquared; 
    copyOf_sampleSetsDuringThisDatalogPeriod = sampleSetsDuringThisDatalogPeriod; // (for diags only)  
    copyOf_lowestNoOfSampleSetsPerMainsCycle = lowestNoOfSampleSetsPerMainsCycle; // (for diags only)
    
    sumP_atSupplyPoint = 0;
    sumP_forDivertedPower = 0;
    sum_Vsquared = 0;
    lowestNoOfSampleSetsPerMainsCycle = 999;
    sampleSetsDuringThisDatalogPeriod = 0;
    datalogEventPending = true;
  }
} 
/* End of helper functions which are used by the ISR
 * -------------------------------------------------
 */

// this function changes the value of outputMode if the external switch is in use for this purpose 
void checkOutputModeSelection()  
{
  static byte count = 0;
  int pinState; 
  // pinState = digitalRead(outputModeSelectorPin); <- pin re-allocated for Dallas sensor
  if (pinState != outputMode)
  {
    count++;
  }  
  if (count >= 20)
  {
    count = 0;
    outputMode = (enum outputModes)pinState;  // change the global variable
    Serial.print ("outputMode selection changed to ");
    if (outputMode == NORMAL) {
      Serial.println ( "normal"); }
    else {  
      Serial.println ( "anti-flicker"); }
    
    configureParamsForSelectedOutputMode();
  }
}


/* 
 * retained for compatibility with previous versions
 */
void configureParamsForSelectedOutputMode(){
  if (outputMode == ANTI_FLICKER)
  {
    // settings for anti-flicker mode
    lowerEnergyThreshold_long = 
       capacityOfEnergyBucket_long * (0.5 - offsetOfEnergyThresholdsInAFmode); 
    upperEnergyThreshold_long = 
       capacityOfEnergyBucket_long * (0.5 + offsetOfEnergyThresholdsInAFmode);   
  }
  else
  { 
    // settings for normal mode
    lowerEnergyThreshold_long = capacityOfEnergyBucket_long * 0.5; 
    upperEnergyThreshold_long = capacityOfEnergyBucket_long * 0.5;   
  }
  
  // display relevant settings for selected output mode
  Serial.print("  capacityOfEnergyBucket_long = ");
  Serial.println(capacityOfEnergyBucket_long);
  Serial.print("  lowerEnergyThreshold_long   = ");
  Serial.println(lowerEnergyThreshold_long);
  Serial.print("  upperEnergyThreshold_long   = ");
  Serial.println(upperEnergyThreshold_long);
  
  Serial.print(">>free RAM = ");
  Serial.println(freeRam());  // a useful value to keep an eye on
}

#ifdef ESP8266_PRESENT
void send_esp8266_data(){

// To avoid disturbance to the sampling process, the RFM12B needs to remain in its
// active state rather than being periodically put to sleep.

  if (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0){
    sendDataPrevMillis = millis();
    esp.print("<");
    esp.print(tx_data.powerAtSupplyPoint_Watts);
    esp.print(", ");
    esp.print(tx_data.divertedEnergyTotal_Wh);
    esp.print(", ");
    esp.print(tx_data.divertedPower_Watts);
    esp.print(">");  
  }
}
#endif

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}



