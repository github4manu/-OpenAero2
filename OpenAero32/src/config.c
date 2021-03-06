//*********************************************************************
//* config.c
//*********************************************************************

//***********************************************************
//* Includes
//***********************************************************

#include "board.h"
#include "mw.h"

//************************************************************
// Variables
//************************************************************
#include <string.h>

#ifndef FLASH_PAGE_COUNT
#define FLASH_PAGE_COUNT 128
#endif

#define FLASH_PAGE_SIZE                 ((uint16_t)0x400)
#define FLASH_WRITE_ADDR                (0x08000000 + (uint32_t)FLASH_PAGE_SIZE * (FLASH_PAGE_COUNT - 1))       // use the last KB for storage

config_t cfg;
const char rcChannelLetters[] = "AERT1234";

static uint8_t EEPROM_CONF_VERSION = 36;
static uint32_t enabledSensors = 0;
static void resetConf(void);

void parseRcChannels(const char *input)
{
    const char *c, *s;

    for (c = input; *c; c++) 
	{
        s = strchr(rcChannelLetters, *c);
        if (s)
		{
            cfg.rcmap[s - rcChannelLetters] = c - input;
     	}
	}
}

static uint8_t validEEPROM(void)
{
    const config_t *temp = (const config_t *)FLASH_WRITE_ADDR;
    const uint8_t *p;
    uint8_t chk = 0;

    // check version number
    if (EEPROM_CONF_VERSION != temp->version)
        return 0;

    // check size and magic numbers
    if (temp->size != sizeof(config_t) || temp->magic_be != 0xBE || temp->magic_ef != 0xEF)
        return 0;

    // verify integrity of temporary copy
    for (p = (const uint8_t *)temp; p < ((const uint8_t *)temp + sizeof(config_t)); p++)
        chk ^= *p;

    // checksum failed
    if (chk != 0)
        return 0;

    // looks good, let's roll!
    return 1;
}

void readEEPROM(void)
{
    uint8_t i;

    // Read flash
    memcpy(&cfg, (char *)FLASH_WRITE_ADDR, sizeof(config_t));

    // Create expo lookup
	for (i = 0; i < 6; i++)
        lookupPitchRollRC[i] = (2500 + cfg.rcExpo8 * (i * i - 25)) * i * (int32_t) cfg.rcRate8 / 2500;

	// Create throttle expo lookup
    for (i = 0; i < 11; i++) {
        int16_t tmp = 10 * i - cfg.thrMid8;
        uint8_t y = 1;
        if (tmp > 0)
            y = 100 - cfg.thrMid8;
        if (tmp < 0)
            y = cfg.thrMid8;
        lookupThrottleRC[i] = 10 * cfg.thrMid8 + tmp * (100 - cfg.thrExpo8 + (int32_t) cfg.thrExpo8 * (tmp * tmp) / (y * y)) / 10;      // [0;1000]
        lookupThrottleRC[i] = cfg.minthrottle + (int32_t) (cfg.maxthrottle - cfg.minthrottle) * lookupThrottleRC[i] / 1000;     // [0;1000] -> [MINTHROTTLE;MAXTHROTTLE]
    }

    cfg.tri_yaw_middle = constrain(cfg.tri_yaw_middle, cfg.tri_yaw_min, cfg.tri_yaw_max);       //REAR
}

void writeParams(uint8_t b)
{
    FLASH_Status status;
    uint32_t i;
    uint8_t chk = 0;
    const uint8_t *p;

    cfg.version = EEPROM_CONF_VERSION;
    cfg.size = sizeof(config_t);
    cfg.magic_be = 0xBE;
    cfg.magic_ef = 0xEF;
    cfg.chk = 0;
    // recalculate checksum before writing
    for (p = (const uint8_t *)&cfg; p < ((const uint8_t *)&cfg + sizeof(config_t)); p++)
        chk ^= *p;
    cfg.chk = chk;

    // write it
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    if (FLASH_ErasePage(FLASH_WRITE_ADDR) == FLASH_COMPLETE) {
        for (i = 0; i < sizeof(config_t); i += 4) {
            status = FLASH_ProgramWord(FLASH_WRITE_ADDR + i, *(uint32_t *) ((char *) &cfg + i));
            if (status != FLASH_COMPLETE)
                break;          // TODO: fail
        }
    }
    FLASH_Lock();

    readEEPROM();
    if (b)
        blinkLED(15, 20, 1);
}

void checkFirstTime(bool reset)
{
    // check the EEPROM integrity before resetting values
    if (!validEEPROM() || reset)
        resetConf();
}

// Default settings
static void resetConf(void)
{
    int i;
    const int8_t default_align[3][3] = { /* GYRO */ { 0, 0, 0 }, /* ACC */ { 0, 0, 0 }, /* MAG */ { -2, -3, 1 } };

    memset(&cfg, 0, sizeof(config_t));

    cfg.version = EEPROM_CONF_VERSION;
    //cfg.mixerConfiguration = MULTITYPE_QUADX;
	cfg.mixerConfiguration = MULTITYPE_AIRPLANE;
    featureClearAll();
    //featureSet(FEATURE_VBAT);		// Enable Vbat monitoring
	//featureSet(FEATURE_PPM);	  	// Enable CPPM input

    cfg.looptime = 0;
    cfg.P8[ROLL] = 20;
    //cfg.I8[ROLL] = 30;
    //cfg.D8[ROLL] = 23;
	cfg.I8[ROLL] = 0;
	cfg.D8[ROLL] = 0;
    cfg.P8[PITCH] = 20;
    //cfg.I8[PITCH] = 30;
    //cfg.D8[PITCH] = 23;
    cfg.I8[PITCH] = 0;
    cfg.D8[PITCH] = 0;
    cfg.P8[YAW] = 85;
	cfg.I8[YAW] = 0;
    //cfg.I8[YAW] = 45;
    cfg.D8[YAW] = 0;
    cfg.P8[PIDALT] = 0;
    cfg.I8[PIDALT] = 0;
    cfg.D8[PIDALT] = 0;
    cfg.P8[PIDPOS] = 0; 	// POSHOLD_P * 100;
    cfg.I8[PIDPOS] = 0; 	// POSHOLD_I * 100;
    cfg.D8[PIDPOS] = 0;
    cfg.P8[PIDPOSR] = 0; 	// POSHOLD_RATE_P * 10;
    cfg.I8[PIDPOSR] = 0; 	// POSHOLD_RATE_I * 100;
    cfg.D8[PIDPOSR] = 0; 	// POSHOLD_RATE_D * 1000;
    cfg.P8[PIDNAVR] = 0; 	// NAV_P * 10;
    cfg.I8[PIDNAVR] = 0; 	// NAV_I * 100;
    cfg.D8[PIDNAVR] = 0; 	// NAV_D * 1000;
    cfg.P8[PIDLEVEL] = 20;
    cfg.I8[PIDLEVEL] = 0;
    cfg.D8[PIDLEVEL] = 100;
    //cfg.P8[PIDLEVEL] = 70;
    //cfg.I8[PIDLEVEL] = 10;
    //cfg.D8[PIDLEVEL] = 20;
    cfg.P8[PIDMAG] = 40;
    cfg.P8[PIDVEL] = 0;
    cfg.I8[PIDVEL] = 0;
    cfg.D8[PIDVEL] = 0;
    cfg.rcRate8 = 100;
    cfg.rcExpo8 = 0;
    cfg.rollPitchRate = 0;
    cfg.yawRate = 0;
    cfg.dynThrPID = 0;
    cfg.thrMid8 = 50;
    cfg.thrExpo8 = 0;
    for (i = 0; i < CHECKBOXITEMS; i++)
        cfg.activate[i] = 0;
    cfg.angleTrim[0] = 0;
    cfg.angleTrim[1] = 0;
    cfg.accZero[0] = 0;
    cfg.accZero[1] = 0;
    cfg.accZero[2] = 0;
	// Magnetic declination: format is [sign]dddmm (degreesminutes) default is zero. 
	// +12deg 31min = 1231 Sydney, Australia 
	// -6deg 37min = -637 Southern Japan
    // cfg.mag_declination = 0;    		
	cfg.mag_declination = 1231;
	memcpy(&cfg.align, default_align, sizeof(cfg.align));
    cfg.acc_hardware = ACC_DEFAULT;     // default/autodetect
    cfg.acc_lpf_factor = 4;
	cfg.acc_lpf_for_velocity = 10;
	cfg.accz_deadband = 50;
    cfg.gyro_cmpf_factor = 400; 		// default MWC
    cfg.gyro_lpf = 42;
    cfg.mpu6050_scale = 1; // fuck invensense
	cfg.baro_tab_size = 21;
	cfg.baro_noise_lpf = 0.6f;
	cfg.baro_cf = 0.985f;
	cfg.moron_threshold = 32;
    cfg.gyro_smoothing_factor = 0x00141403;     // default factors of 20, 20, 3 for R/P/Y
    cfg.vbatscale = 110;
    cfg.vbatmaxcellvoltage = 43;
    cfg.vbatmincellvoltage = 33;
	// cfg.power_adc_channel = 0;

    // Radio
    parseRcChannels("AETR1234"); 
	//parseRcChannels("TAER1234"); // Debug - JR
    cfg.deadband = 0;
    cfg.yawdeadband = 0;
    cfg.alt_hold_throttle_neutral = 20;
    cfg.spektrum_hires = 0;
    for (i = 0; i < 8; i++)					// RC stick centers
	{
		cfg.midrc[i] = 1500;
    }
	cfg.defaultrc = 1500;
	cfg.mincheck = 1100;
    cfg.maxcheck = 1900;
    cfg.retarded_arm = 0;       			// disable arm/disarm on roll left/right

    // Failsafe Variables
    cfg.failsafe_delay = 10;    			// 1sec
    cfg.failsafe_off_delay = 200;       	// 20sec
    cfg.failsafe_throttle = 1200;       	// decent default which should always be below hover throttle for people.

    // Motor/ESC/Servo
    cfg.minthrottle = 1150;
    cfg.maxthrottle = 1850;
    cfg.mincommand = 1000;
    cfg.motor_pwm_rate = 400;
    cfg.servo_pwm_rate = 50;

    // servos
    cfg.yaw_direction = 1;
    cfg.tri_yaw_middle = 1500;
    cfg.tri_yaw_min = 1020;
    cfg.tri_yaw_max = 2000;

    // gimbal
    cfg.gimbal_pitch_gain = 10;
    cfg.gimbal_roll_gain = 10;
    cfg.gimbal_flags = GIMBAL_NORMAL;
    cfg.gimbal_pitch_min = 1020;
    cfg.gimbal_pitch_max = 2000;
    cfg.gimbal_pitch_mid = 1500;
    cfg.gimbal_roll_min = 1020;
    cfg.gimbal_roll_max = 2000;
    cfg.gimbal_roll_mid = 1500;

    // gps/nav stuff
    cfg.gps_type = GPS_NMEA;
    cfg.gps_baudrate = 115200;
    cfg.gps_wp_radius = 200;
    cfg.gps_lpf = 20;
    cfg.nav_slew_rate = 30;
    cfg.nav_controls_heading = 1;
    cfg.nav_speed_min = 100;
    cfg.nav_speed_max = 300;

    // serial (USART1) baudrate
    cfg.serial_baudrate = 115200;

	// Aeroplane stuff
	cfg.flapmode = ADV_FLAP;				// Switch for flaperon mode?
	cfg.flapchan = AUX2;					// RC channel number for simple flaps)
	cfg.aileron2 = AUX1;					// RC channel number for second aileron
	cfg.flapspeed = 10;						// Desired rate of change of flaps 
	cfg.flapstep = 3;						// Steps for each flap movement
	cfg.DynPIDchan = THROTTLE;				// Dynamic PID source channel
	cfg.DynPIDbreakpoint = 1500;			// Dynamic PID breakpoint
	cfg.rollPIDpol = 1;
	cfg.pitchPIDpol = 1;
	cfg.yawPIDpol = 1;

    for (i = 0; i < 8; i++)					// Servo limits
	{
		cfg.servoendpoint_low[i] = 1000;
		cfg.servoendpoint_high[i] = 2000;
		cfg.servoreverse[i] = 1;
		cfg.servotrim[i] = 1500;			// Set servo trim to default
	}	
				
    // custom mixer. clear by defaults.
    for (i = 0; i < MAX_MOTORS; i++)
        cfg.customMixer[i].throttle = 0.0f;

    writeParams(0);
}

bool sensors(uint32_t mask)
{
    return enabledSensors & mask;
}

void sensorsSet(uint32_t mask)
{
    enabledSensors |= mask;
}

void sensorsClear(uint32_t mask)
{
    enabledSensors &= ~(mask);
}

uint32_t sensorsMask(void)
{
    return enabledSensors;
}

bool feature(uint32_t mask)
{
    return cfg.enabledFeatures & mask;
}

void featureSet(uint32_t mask)
{
    cfg.enabledFeatures |= mask;
}

void featureClear(uint32_t mask)
{
    cfg.enabledFeatures &= ~(mask);
}

void featureClearAll()
{
    cfg.enabledFeatures = 0;
}

uint32_t featureMask(void)
{
    return cfg.enabledFeatures;
}
