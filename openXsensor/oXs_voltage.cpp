#include "oXs_voltage.h"
 
#ifdef DEBUG
#define DEBUGNEWVALUE
//#define DEBUGDELAY
//#define DEBUGCELLCALCULATION
//#define DEBUGLOWVOLTAGE
#endif

#ifdef PIN_VOLTAGE

extern unsigned long micros( void ) ;
extern unsigned long millis( void ) ;
extern void delay(unsigned long ms) ;


#ifdef DEBUG
OXS_VOLTAGE::OXS_VOLTAGE(HardwareSerial &print) 
#else
OXS_VOLTAGE::OXS_VOLTAGE(uint8_t x) 
#endif
{
#ifdef DEBUG  
  printer = &print; //operate on the address of print   
#endif
}

// **************** Setup the Current sensor *********************
void OXS_VOLTAGE::setupVoltage( void ) {
  uint16_t tempRef ;
#ifdef USE_INTERNAL_REFERENCE   
  analogReference(INTERNAL) ;
#endif
#ifdef PIN_VOLTAGE  
  uint8_t tempPin[6 ] = { PIN_VOLTAGE };
#else
  uint8_t tempPin[6 ] = { 8 , 8 , 8 , 8 , 8 , 8 } ;
#endif  
#ifdef RESISTOR_TO_GROUND 
  float tempResistorToGround[6] = { RESISTOR_TO_GROUND } ;
#else
  float tempResistorToGround[6] = { 0 , 0 , 0 , 0 , 0 , 0 } ;
#endif
#ifdef RESISTOR_TO_VOLTAGE  
  float tempResistorToVoltage[6] = { RESISTOR_TO_VOLTAGE } ;
#else
  float tempResistorToVoltage[6] = { 0 , 0 , 0 , 0 , 0 , 0 } ;
#endif
#ifdef OFFSET_VOLTAGE  
  int tempOffsetVoltage[6] = { OFFSET_VOLTAGE} ;
#else 
  int tempOffsetVoltage[6] =  { 0 , 0 , 0 , 0 , 0 , 0 } ;
#endif
#ifdef SCALE_VOLTAGE   
  float tempScaleVoltage[6] = { SCALE_VOLTAGE }  ;
#else
  float tempScaleVoltage[6] =  { 1 , 1 , 1 , 1 , 1 , 1 } ;
#endif
 
#if defined(USE_INTERNAL_REFERENCE) && defined(REFERENCE_VOLTAGE) && REFERENCE_VOLTAGE < 2000
  tempRef = REFERENCE_VOLTAGE  ;
#elif defined(USE_INTERNAL_REFERENCE) && defined(REFERENCE_VOLTAGE)
  #error REFERENCE_VOLTAGE must be less than 2000 when USE_INTERNAL_REFERENCE is defined
#elif defined(USE_INTERNAL_REFERENCE)
  tempRef = 1100 ;
#elif defined(REFERENCE_VOLTAGE) && REFERENCE_VOLTAGE > 2000
  tempRef = REFERENCE_VOLTAGE  ;
#elif defined(REFERENCE_VOLTAGE)
  #error REFERENCE_VOLTAGE must be greater than 2000 when USE_INTERNAL_REFERENCE is not defined
#else 
  tempRef = 5000 ;
#endif  

#ifdef DEBUG  
  printer->print("Reference voltage:");
  printer->println(tempRef);
#endif

  for (int cntInit = 0 ; cntInit < 6 ; cntInit++) {
    if ( tempPin[ cntInit ] < 8 ) {
      voltageData.mVoltPin[cntInit] =  tempPin[ cntInit ] ;
      pinMode(voltageData.mVoltPin[cntInit],INPUT);
    } else {
      voltageData.mVoltPin[cntInit] = 8 ;
    }
    voltageData.offset[cntInit] = tempOffsetVoltage[ cntInit ] ;
    if ( tempResistorToGround[cntInit] > 0 && tempResistorToVoltage[cntInit] > 0 && tempScaleVoltage[cntInit] > 0 ) {
      voltageData.mVoltPerStep[cntInit] = tempRef / 1023.0 * ( tempResistorToGround[cntInit] + tempResistorToVoltage[cntInit] ) / tempResistorToGround[cntInit]  * tempScaleVoltage[cntInit];
    } else {
      voltageData.mVoltPerStep[cntInit] = tempRef / 1023.0 ;  
    }
    voltageData.sumVoltage[cntInit] = 0 ;
    voltageData.mVoltAvailable[cntInit] = false ; 
#ifdef DEBUG  
    printer->print("Voltage:"); printer->print( cntInit + 1 );
    printer->print(" , pin="); printer->print( voltageData.mVoltPin[cntInit] );
    printer->print(" , offset="); printer->print( voltageData.offset[cntInit] );
    printer->print("  , mVoltPerStep="); printer->println( voltageData.mVoltPerStep[cntInit] );
#endif
    
  }
//  voltageData.atLeastOneVoltage = ( voltageData.mVoltPin[0] < 8 || voltageData.mVoltPin[1] < 8 || voltageData.mVoltPin[2] < 8 ||voltageData.mVoltPin[3] < 8 ||voltageData.mVoltPin[4] < 8 || voltageData.mVoltPin[5] < 8 ) ;
}

// Maximum voltage that is allowed (theoretical) on the voltage divider is 5/R2/(R1+R2);

  static byte voltageNr = 0;

void OXS_VOLTAGE::readSensor() {
// here we should test if Current sensor is used too; otherwise, it is not necessary to use the 2 next instructions

#ifdef DEBUGDELAY
        long milliVoltBegin = millis() ;
#endif

    while ( voltageData.mVoltPin[voltageNr] > 7) { // Skip nr if voltageNr have not a pin defined between 0 and 7
        voltageNrIncrease();
    }  
    voltageData.sumVoltage[voltageNr] += readVoltage(voltageNr) ;
#ifdef DEBUGDELAY
        printer->print("readVoltage voltageNr =  ");
        printer->print(voltageNr);
        printer->print(" begin at =  ");
        printer->print(milliVoltBegin);
        printer->print(" end at =  ");
        printer->println(millis());
#endif

    voltageNrIncrease();

}      



// Select next voltage to read ; if all voltages have been read and 500 ms are enlapsed since previous average calculation, calculates the new averages for each voltage 
void OXS_VOLTAGE::voltageNrIncrease() {
  static int cnt = 0;
  static unsigned long lastVoltMillis = millis() ;
#if defined ( NUMBEROFCELLS ) && (NUMBEROFCELLS > 0)
    static int32_t secondMVolt ;
#endif  
  //static int32_t previousMVolt ;  
  
  voltageNr++;
  if(voltageNr == 6) { 
      voltageNr = 0 ;
      cnt++;
      if(millis() > ( lastVoltMillis + 500) ){   // calculate average only once every 500 msec 
        for (int cntVolt = 0 ; cntVolt < 6 ; cntVolt++) {      
          if ( voltageData.mVoltPin[cntVolt] < 8) {
            voltageData.mVolt[cntVolt] = (voltageData.sumVoltage[cntVolt] / cnt  * voltageData.mVoltPerStep[cntVolt] ) + voltageData.offset[cntVolt];
//            voltageData.mVolt[cntVolt] = (1 + cntVolt) * 3000 + cntVolt * (millis() & 0xFF)  ; // this is just to test the cell calculation ; !!!!!!!!!!!to be removed
            voltageData.mVoltAvailable[cntVolt] = true ;
            voltageData.sumVoltage[cntVolt] = 0 ;
#ifdef DEBUGNEWVALUE
            printer->print("At ");
            printer->print(millis());
            printer->print(" Cnt = ");
            printer->print(cnt);
            printer->print(" mVolt ");
            printer->print(cntVolt);
            printer->print(" = ");
            printer->println( voltageData.mVolt[cntVolt] );
#endif
          } // if
        } // End For 
        
#if defined ( NUMBEROFCELLS ) && (NUMBEROFCELLS > 0)
        for (uint8_t cellIndex = 0; cellIndex < NUMBEROFCELLS ; cellIndex++) {
          int32_t mVoltOneCell ;
          uint8_t prevIndex ;
          if (cellIndex == 0) {
            mVoltOneCell = voltageData.mVolt[cellIndex];
          } else { 
            mVoltOneCell = voltageData.mVolt[cellIndex] - voltageData.mVolt[prevIndex] ;
          }
          prevIndex = cellIndex ;        
          if (mVoltOneCell  < 500) mVoltOneCell = 0 ;
          voltageData.mVoltCell[cellIndex]  = mVoltOneCell ;
          voltageData.mVoltCell_Available[cellIndex] = true ;
        }
        voltageData.mVoltCellMin = 0 ;
        voltageData.mVoltCellTot = 0 ;
        for (uint8_t cellIndex = 0; cellIndex < NUMBEROFCELLS ; cellIndex++) {
          if (voltageData.mVoltCell[cellIndex] == 0 ) {
            break ;
          } else {
            if ( (voltageData.mVoltCellMin == 0) || ( voltageData.mVoltCellMin > voltageData.mVoltCell[cellIndex] ) ){
              voltageData.mVoltCellMin = voltageData.mVoltCell[cellIndex] ;
            }
              voltageData.mVoltCellTot = voltageData.mVolt[cellIndex] ;            
          }  
        }
        if ( voltageData.mVoltCellMin > 0 ) {
          voltageData.mVoltCellMin_Available = true ;
        }
        voltageData.mVoltCellTot_Available = true ;
#ifndef  MULTIPLEX // not multiplex
        if (NUMBEROFCELLS == 1) {
          secondMVolt = 0 ; 
        }
        else { 
          secondMVolt = voltageData.mVolt[1]; 
        }
        voltageData.mVoltCell_1_2 = calculateCell(0, voltageData.mVolt[0] , secondMVolt , 0) ;
        voltageData.mVoltCell_1_2_Available = true ;   
        if (NUMBEROFCELLS > 2) {
            if (NUMBEROFCELLS == 3) {
              secondMVolt = 0 ; 
            }
            else { 
              secondMVolt = voltageData.mVolt[3] ;
            }  
            voltageData.mVoltCell_3_4 = calculateCell(voltageData.mVolt[1] , voltageData.mVolt[2] , secondMVolt , 2) ;
            voltageData.mVoltCell_3_4_Available = true ;
        }
        if (NUMBEROFCELLS > 4) {
            if (NUMBEROFCELLS == 5) {
              secondMVolt = 0 ; 
            }
            else { 
              secondMVolt = voltageData.mVolt[5] ;
            }  
            voltageData.mVoltCell_5_6 = calculateCell(voltageData.mVolt[3] , voltageData.mVolt[4] , secondMVolt , 4) ;
            voltageData.mVoltCell_5_6_Available = true ;
        }
#endif // Enf of multiplex/non multiplex
#endif // ( NUMBEROFCELLS ) && (NUMBEROFCELLS > 0)
        cnt=0;
        lastVoltMillis = millis() ;
#ifdef SEQUENCE_OUTPUTS
        lowVoltage = false ;
#if defined( SEQUENCE_MIN_CELL)  
        if ( voltageData.mVoltCellMin < SEQUENCE_MIN_CELL) {        
          lowVoltage = true ;
        }
#endif //SEQUENCE_MIN_CELL
#if defined( SEQUENCE_MIN_VOLT_6) && defined (PIN_VOLTAGE_6 )
        if ( voltageData.mVolt[5] < SEQUENCE_MIN_VOLT_6) {        
          lowVoltage = true ;
        }
#endif //( SEQUENCE_MIN_VOLT_6) && defined (PIN_VOLTAGE_6 )
#ifdef DEBUGLOWVOLTAGE
            printer->print("LowVoltage= ");
            printer->println(lowVoltage) ;
#endif
#endif // SEQUENCE_OUTPUTS
        
      }   // End if VOLT_BUFFER_LENGTH
   }     // End if == 6
}



void OXS_VOLTAGE::resetValues() {
  // not used currently
}    


int OXS_VOLTAGE::readVoltage( int value ) { // value is the index in an aray giving the pin to read
  //******** First discharge the capacitor of ADCMux to ground in order to avoid that measurement from another pin has an impact on this measurement  
#ifdef USE_INTERNAL_REFERENCE
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0); // binary = 11 00 1111 (11 = use VRef as max, 1111 = measure ground level)
#else
  ADMUX =  _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0); // binary = 01 00 1111 (01 = use Vcc as max, 1111 = measure ground level)
#endif
    delayMicroseconds(200); // Wait for Vref to settle 
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // wait that conversion is done
  
  analogRead( voltageData.mVoltPin[value]); // read the value from the sensor 
  // discard the first measurement
  delayMicroseconds(100); // Wait for ADMux to settle 
  return analogRead(voltageData.mVoltPin[value]); // use the second measurement
}

#if defined ( NUMBEROFCELLS ) && (NUMBEROFCELLS > 0 )
// calculate 2 cell voltages, make some checks and format in Frsky format.
uint32_t OXS_VOLTAGE::calculateCell(int32_t V0 , int32_t V1 , int32_t V2 , int cellId) {
  int32_t cell_1 ;
  int32_t cell_2 ;
  if (V0 < 500) V0 = 0 ;
  if (V1 < 500) V1 = 0 ;
  if (V2 < 500) V2 = 0 ;
  cell_1= V1 - V0 ;
  cell_2= V2 - V1 ;
  if (cell_1 < 500) cell_1 = 0 ;
  if (cell_2 < 500) cell_2 = 0 ;
  cell_1 = (cell_1 >> 1) & 0xFFF ; //Frsky expects that the value are divided by 2
  cell_2 = (cell_2 >> 1) & 0xFFF ; //Frsky expects that the value are divided by 2
#ifdef DEBUGCELLCALCULATION
            printer->print("Cell calculation for cellId ");
            printer->print(cellId) ;
            printer->print(" Frist Cell = ");
            printer->print(cell_1) ;
            printer->print(" Second Cell = ");
            printer->print(cell_2) ;
            printer->print(" Frsky value = ");
            printer->println( (cell_2 << 20) | (cell_1 << 8) | ( ( (int32_t) NUMBEROFCELLS)<<4 ) | (int32_t) cellId  , HEX );
#endif
  return (cell_2 << 20) | (cell_1 << 8) | ( ( (int32_t) NUMBEROFCELLS)<<4 ) | (int32_t) cellId ;
}
#endif // end calculateCell

#endif // end of PIN_VOLTAGE
