/// \copyright 2017 David Jabson, Brainstorm Engineering
///
/// \file roc.h
/// \brief Header file for the initial ROC2 data collection software.
///    

#ifndef __ROC__
#define __ROC__

#define SWDate					"20180725,2231"		///< Build date and time
#define SWVersion				"027"				///< Software version number

// Definitions
#define TRUE					1
#define FALSE					0
#define ON						1
#define OFF						0
#define RED						1
#define GREEN					2
#define ORANGE					3
#define MODULE_NAME				"roc"

#define BAUD_DEFAULT			115200				///< Default serial port baud rate
#define BITS_DEFAULT			"8N1"				///< Default serial data bits
#define FLOW_DEFAULT			'N'					///< Default flow control setting
#define TERM_DEFAULT			"\r\n"				///< Default line terminator
#define READBUFSIZE				4096				///< Size of serial input buffer
#define FILE_LEN_DEFAULT		30					///< Default length of data files (seconds)
#define DIR_DEFAULT				"/data/roc"			///< Default location to store data files

//Error Codes
#define BCSUCCESS				0		///< Return code for functions that complete successfully
#define BCERROR					-1		///< Indicates general error
#define BCINVALID				-2		///< Indicates invalid parameter on command line
#define BCCONFIGREADERROR		-3		///< Indicates error reading configuration file
#define BCCONFIGLOOKUPERROR		-4		///< Indicates error finding setting in configuration file
#define BCCMDLINEERROR			-5		///< Indicates error on command line
#define BCFILEOPENERROR			-6		///< Indicates error opening data file
#define BCZMQBINDERROR			-7		///< Indicates error binding ZMQ REP socket
#define BCZMQOPTERROR			-8		///< Indicates error setting option on ZMQ socket
#define BCZMQCONNERROR			-9		///< Indicates error connecting ZMQ REQ socket
#define BCMODLISTERROR			-10		///< Indicates error populating the list of BuoyCom modules
#define BCMEMERROR				-11		///< Indicates error allocating memory
#define BCFORKERROR				-12		///< Indicates error launching module
#define BCDISPATCHERROR			-13		///< Indicates error registering module with dispatch
#define BCINVALIDMSG			-14		///< Indicates unsupported message received by module
#define BCMSGTOOLARGE			-15		///< Attempt to send a message longer than MAX_MSG_LEN bytes
#define BCSERIALERROR			-16		///< Indicates error initializing serial port
#define BCINVALIDBITS			-17		///< Indicates unsupported number of serial data bits
#define BCSEROPENERROR			-18		///< Error opening serial port
#define BCINVALIDPAR			-19		///< Indicates unsupported serial parity
#define BCINVALIDSTOP			-20		///< Indicates unsupported number of serial stop bits
#define BCINVALIDFLOW			-21		///< Indicates unsupported flow control specifier
#define BCSETATTRERROR			-22		///< Error setting serial attributes
#define BCMAXMODULES			-23		///< Indicates maximum number of modules has been reached
#define BCMODNOTFOUND			-24		///< Cannot find specified module
#define BCFILEERROR				-25		///< General file error
#define BCNODATAFILES			-26		///< No data files to transfer
#define BCNOCARRIER				-27		///< Modem unable to connect
#define BCNOREPLY				-28		///< No reply from modem


// Global Variables
const char cloptions[16]="vl:b:s:f:p:\0";				///< List of valid command line options. A : indicates parameter required for that option
const char *pathCfgRoc = "/etc/opt/roc/roc.conf";		///< Full path to ROC configuration file
config_t cfgRoc;										///< Pointer to structure holding data from config file
const char *pathCfgGps = "/etc/opt/roc/gps.conf";		///< Full path to GPS configuration file
config_t cfgGps;										///< Pointer to structure holding data from config file
const char *ipcPipe = "/tmp/rocfifo";
config_setting_t *setting;								///< Pointer to a setting used by libconfig.
const char *queueDir = "/data/roc/queue";				///< hard coded for now, move to config file

char todaysDirectoryName[100];
char todaysDirectoryPath[100];
char logFileName[100];

int verbose;			///< Print diagnostic messages to stdout
int quit;				///< Tells the main loop when to exit
int file_len;			///< Length of data files (seconds)
char *data_dir;			///< Top level directory for logged data files
FILE *fp;				///< Pointer to open data file
FILE *pipefp;				///< Pointer to named pipe for communicating with Zephyr process

char *curfilename;			///< Stores the name of the current data file
struct timespec nextfile;		///< Stores the time when a new data files should be started
struct timespec now;			///< Stores current time

typedef struct {
	char *port;				///< Serial device name
	int speed;				///< Serial baud rate
	int bits;				///< Bits per character (7 or 8)
	char parity;			///< Parity bit ('N', 'O', or 'E')
	int stop;				///< Number of stop bits (1 or 2)
	char flow;				///< Flow control ('N', 'H', or 'S')
	char term[3];			///< Line terminator
	int fp;					///< File descriptor for open serial port
} serial;

serial gps_cmd, gps_data;

// Function Prototypes
int RocInit(int argc, char *argv[]);
int RocCleanup(void);
int CmdLineHandler(int argc, char *argv[]);
void Usage(void);
int LoadConfigFileSettings(config_t *cfg, const char *path);
int SerialPortInit(serial *port);
int GPSStartupCmds(void);
int get_baud(int baud);
int verify_bits(const char *bits);
int verify_flow(const char *bits);
char *TodaysDirectoryName(void);
char *TodaysDirectoryPath(void);
int TodaysDirectoryExists(void);
int CreateDayDirectory(void);
char *LogFileName(char *name);
int OpenLogFile(char *name);
int CloseLogFile(void);
int NewFile(void);
int LogData(FILE *fp, char *data, int n);
int CopyFile(char *name);
int SendGpsCmd(char *cmdstr, int timeout);
int GetGpsReply(char *replybuf, int timeout);
void gps_power(int state);
void gps_reset(int state);
void gps_power_toggle(int state);
void set_safe(int state);
void led(int state);
void set_gpio0(int state);
void led_blink(int color, int num, int duration);

#endif  //  __ROC__


