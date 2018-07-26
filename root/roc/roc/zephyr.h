/// \copyright 2017 David Jabson, Brainstorm Engineering
///
/// \file zephyr.h
/// \brief Header file for the initial ROC2 Zephyr interface module.
///    

#ifndef __ZEPHYR__
#define __ZEPHYR__


// Definitions
#define INSTID					"ROC"				///< Instrument ID string in message headers
#define SWDate					"20180725,2231"		///< Build date and time
#define SWVersion				"027"				///< Software version number
#define ZephVersion				"100"				///< Zephyr protocol version
#define MAX_DATA				3800				///< Maximum size of TM packet payload
#define SAFETIMEOUT				600
#define LOWPOWERCHECK			15

#define TRUE					1
#define FALSE					0
#define ON						1
#define OFF						0
#define RED						1
#define GREEN					2
#define ORANGE					3
#define MODULE_NAME				"zephyr"
#define GPIO_PATH				"/opt/roc/gpio"		///< Full path to script that controls digital IO

#define BAUD_DEFAULT			115200				///< Default serial port baud rate
#define BITS_DEFAULT			"8N1"				///< Default serial data bits
#define FLOW_DEFAULT			'N'					///< Default flow control setting
#define TERM_DEFAULT			"\r\n"				///< Default line terminator
#define INBUFSIZE				2048				///< Size of incoming serial buffer
#define OUTBUFSIZE				16348				///< Size of outgoing serial buffer
#define DIR_DEFAULT				"/data/roc"			///< Default location to store data files
#define HWVERSION_DEFAULT		2					///< Version number of interface board

#define WAITFORACK				5
#define WAITFORIM				5
  
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
const char *pathCfgZephyr = "/etc/opt/roc/zephyr.conf";		///< Full path to zephyr configuration file
const char *pathUpload = "/etc/opt/roc/";				///< Directory where telecommand payload files will be stored
config_t cfgZephyr;										///< Pointer to structure holding data from config file
config_setting_t *setting;								///< Pointer to a setting used by libconfig.
const char *queueDir="/data/roc/queue";					///< hard coded for now, move to config file
const char *ipcPipe = "/tmp/rocfifo";

int verbose;			///< Print diagnostic messages to stdout
int quit;				///< Tells the main loop when to exit
char *data_dir;			///< Top level directory for logged data files
FILE *fp;				///< Pointer to open data file
FILE *pipefp;				///< Pointer to named pipe for communicating with ROC process
struct timespec nextfile;		///< Stores the time when a new data files should be started
struct timespec now;			///< Stores current time
int sendIMRatPower;		// Flag to send IMR in SB mode at power on, cleared by Flight Mode
int poweroffgps;		///< flags that we have sent power command to GPS
int modeSwitch;			///< Notifies main loop when instrument mode is changed
int wakeUpFlag;			///< Notifies main loop to toggle wake up line
int safeAck;			///< Flag that indicates the Safety message has been acknowledged
char fileOffload[100];		///< Stores the name of the file being currently offloaded
char TCfileOffload[100];		///< Stores the name of the TC file being currently offloaded
int numParts, lastPart;	///< Stores progress of multi-part data file offload
int TCnumParts, TClastPart;	///< Saves Stored progress of multi-part data file offload
int waitingAck;			///< Indicates we're waiting for acknowledgement from Zephyr
int waitingIM;			///< Indicates we're waiting for acknowledgement from Zephyr
//time_t ackTimeout;		///< Time at which to stop waiting for response from Zephyr
int safeTimeout;		///< Time at which ROC enters Safe mode if it hasn't heard from Zephyr
int lowPowerCheck;		///< Time interval at which we check the low power state
int lowPowerCheckWait;	///< Flag to hold off low power check
int gotGPSChars;		///< Flag to indicate GPS chars were received
int discardFirstFile;	// Discard first GPS file.
struct timespec nextMsgTime;		///< Earliest time that is OK to send the next message (no more than once/sec)
int shutdown;			///< Indicates whether system should shutdown after this process ends
int dataTm;				///< Indicates whether the last TM message sent contains offload data
int TCflag;				///< Flag TC data transfer
time_t nextImr;			///< Timer that keeps track of when to send out IMR messages until it gets a response
int hwversion;			///< Version number of interface board. As of 6/10/18 ROC2.1 and 2.2 have version 2, ROC2.3 has version 3.

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

serial zephyr;

enum mode {
	FL, 		///< Flight Mode (normal operation)
	SB, 		///< StandBy Mode
	LP, 		///< Low Power mode
	SA, 		///< Enter Safety Mode
	EF			///< End Flight
};

enum msg {
	IMR, 		///< Instrument Mode Request (sent from ROC to Zephyr)
	IM, 		///< Instrument Mode (sent from Zephyr to ROC)
	IMAck, 		///< Instrument Mode Acknowledge (sent from ROC to Zephyr)
	S, 			///< Safety mode reached (sent from ROC to Zephyr)
	SAck, 		///< Safety Acknowledge (sent from Zehpyr to ROC)
	SW,			///< Shutdown Warning (sent from Zehpyr to ROC)
	RA,			///< RACHuTS Authorization (ignore)
	RAAck,		///< RACHuTS Auth Ack (ignore)
	TM,			///< Telemetry Message (sent from ROC to Zephyr)
	TMAck,		///< Telemetry Message Acknowlege (sent from Zephyr to ROC)
	TC,			///< Telecommand (sent from Zephyr to ROC)
	TCAck,		///< Telecommand Acknowledge (sent from ROC to Zephyr)
	GPS,		///< GPS Data (sent from Zephyr to ROC)
	Error		///< Unrecognized message
};

enum mode InstMode, LastMode;

char inBuf[INBUFSIZE], outBuf[OUTBUFSIZE];	///< incoming and outgoing serial buffers
char *inPtr, *outPtr;						///< Buffer pointers
int MsgID;								///< counter that increments with each sent message
unsigned short crc_calc;							///< Calculated CRC value
unsigned short crc_msg;								///< CRC value from message

// Function Prototypes
int ZephyrInit(int argc, char *argv[]);
int ZephyrCleanup(void);
int CmdLineHandler(int argc, char *argv[]);
void Usage(void);
int LoadConfigFileSettings(config_t *cfg, const char *path);
int SerialPortInit(serial port);
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
char *GetNextFile(void);
int SendFile(char *name);
int RemoveFile(char *name);
int CRCComp(char* data, int len);
unsigned short CRC(unsigned short last, char next);
unsigned short ComputeCRC(char *data, int len);
void SendMsg(enum msg msgtype, char *payload, int payload_size);
enum msg MsgType(char *typestr);
int MsgHandler(enum msg msgtype, char *msgdata);
int HandleIM(char *msgdata);
int HandleSAck(char *msgdata);
int HandleSW(char *msgdata);
int HandleTMAck(char *msgdata);
int HandleTC(char *msgdata);
int HandleGPS(char *msgdata);
void gps_power(int state);
void gps_reset(int state);
void gps_power_toggle(int state);
void set_safe(int state);
void led(int state);
void set_gpio0(int state);

char todaysDirectoryName[10];
char todaysDirectoryPath[100];
char logFileName[100];
char getNextFile[100];

#endif  //  __ZEPHYR__


