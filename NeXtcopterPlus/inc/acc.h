/*********************************************************************
 * acc.h
 ********************************************************************/

//***********************************************************
//* Externals
//***********************************************************

extern void ReadAcc(void);
extern void CalibrateAcc(void);

extern bool AccCalibrated;
extern int16_t accADC[2];		// Holds Acc ADC values
extern int16_t accZero[2];		// Used for calibrating Accs on ground
