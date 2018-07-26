/// \copyright 2017 David Jabson, Brainstorm Engineering
///
/// \file roc.c
/// \brief ROC2 Initial Data Collection Program
///
/// This program is the first version of data collection software for the ROC2 Radio
///   Occultation Instrument. It lacks the full feature set of the final instrument but
///   performs the following:
///
///   - Reads configuration file to determine commands for setting up GPS receiver
///   - Configures GPS receiver
///   - Launch data offload program
///   - Logs data received from GPS receiver to files
///   - When data file is closed, calls data processing script and open new file
///

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <libconfig.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "roc2.h"

/// \brief Main function.
///
/// Calls RocInit() to perform initialization then enters the main while loop where it
/// .... 
/// the main loop stops, then RocCleanup() is called before exiting.

int main (int argc, char *argv[])
{
	int err, acked, i, numread, j;
	unsigned char *cmd_endptr, *cmd_readptr, *data_endptr, *data_readptr;
	unsigned char term, cmd_readbuf[READBUFSIZE*2], cmd_linebuf[READBUFSIZE/2], data_readbuf[READBUFSIZE*2];
	struct termios options;
	struct timespec curtime;
	FILE *wkfp;

	struct pollfd fds[2];

	err = RocInit(argc, argv);
	if(err<0)
	{
		fprintf(stderr, "%s: Initialization error %d. Exiting\r\n",MODULE_NAME,err);
//		led(RED);
	}
	else
	{
		printf("SWVersion = %s\r\nSWDate = %s\r\n",SWVersion,SWDate);
		printf("%s: Module started.\r\n",MODULE_NAME);
//		led(GREEN);
	}

	err = GPSStartupCmds();
	if(err<0)
	{
	}
	else
	{
		if(verbose) printf("%s: Processed %d startup commands.\r\n", MODULE_NAME, err);
	}

	term=gps_cmd.term[strlen(gps_cmd.term)-1];
	cmd_readptr=cmd_readbuf;
	memset(cmd_readbuf, 0, sizeof(cmd_readbuf));
	data_readptr=data_readbuf;
	memset(data_readbuf, 0, sizeof(data_readbuf));

	fds[0].fd = gps_data.fp;
	fds[0].events = POLLIN;
	fds[1].fd = gps_cmd.fp;
	fds[1].events = POLLIN;

//	tcgetattr(serial.fp, &options);						// Get current port options
//	options.c_cflag |= CREAD;							// Enable the serial receiver
//	tcflush(serial.fp,TCIOFLUSH);						// Flush serial buffers
//	if(tcsetattr(serial.fp, TCSANOW, &options) == 0)	// Make the new settings active

    verbose = 1;
	numread = 0;
//	fp = 0;

	while(!quit)
	{
		if(time(NULL) >= nextfile.tv_sec)
		{
			nextfile.tv_sec+=file_len;
			if (numread && fp) {
				err = LogData(fp, data_readbuf, numread);
				if(verbose) printf("%s: Logged %d bytes, err=%d : 1\r\n", MODULE_NAME, numread, err);
				numread = 0;
			}
			NewFile();
		}

#if 0
		if ((wkfp = fopen("/data/roc/FLAGWAKEUP", "r")) > 0) {
			char buf[50];
			int saveSpeed;

			printf("roc: Wakeup GPS\n");
			close(gps_data.fp);
			saveSpeed = gps_data.speed;
			gps_data.speed = 4800;
			err = SerialPortInit(&gps_data);
			if(err != BCSUCCESS) 
				printf("roc: Baud rate change to 4800 unsuccessful");
			
			for (i=0;i<50;i++) buf[i] = 0;
			fclose(wkfp);
			write(gps_data.fp, buf, 1);
			usleep(100000);
			write(gps_data.fp, buf, 1);
			usleep(100000);
			write(gps_data.fp, buf, 1);
			usleep(100000);
			write(gps_data.fp, buf, 1);
			usleep(100000);
			write(gps_data.fp, buf, 1);
			usleep(100000);
			write(gps_data.fp, buf, 1);
			usleep(500000);
			close(gps_data.fp);
			gps_data.speed = saveSpeed;
			err = SerialPortInit(&gps_data);
			if(err != BCSUCCESS) 
				printf("roc: Baud rate change to %d unsuccessful", saveSpeed);
			usleep(200000);

			system("rm /data/roc/FLAGWAKEUP");
		}
#endif

		if(poll (fds, 1, 10)) {
//		if ((fds [1].revents & POLLIN) || ((data_readptr-1) > data_endptr))
		if (fds [0].revents & POLLIN)		// incoming data on GPS data port
		{
			err = ioctl(gps_data.fp, FIONREAD, &i);
			if(err)
			{
				fprintf(stderr,"%s: I/O error, exiting.",MODULE_NAME);
				quit=1;
			}

//			if(verbose) printf("%d bytes ready to read from %s\r\n",i, gps_data.port);
			if((numread + i) > READBUFSIZE)	// Serial read will overrun buffer if we do it
			{
				err = LogData(fp, data_readbuf, numread);
				if(verbose) printf("%s: Logged %d bytes, err=%d : 2\r\n", MODULE_NAME, numread, err);
				numread = 0;
			}

			numread += read(gps_data.fp, data_readbuf+numread, i);
//			if(verbose) printf("Writing %d bytes\r\n", numread);
		}

		if (fds [1].revents & POLLIN)		// incoming data on GPS command port
		{
			ioctl(gps_cmd.fp, FIONREAD, &i);
			if(verbose) printf("%d bytes ready to read from %s\r\n",i, gps_cmd.port);
			if((cmd_readptr + i - cmd_readbuf) > READBUFSIZE)	// Serial read will overrun buffer if we do it
			{
				fprintf(stderr, "Serial buffer overrun. Data lost!\r\n");
				cmd_readptr=cmd_readbuf;
				memset(cmd_readbuf, 0, sizeof(cmd_readbuf));
			}
			else
			{
				numread = read(gps_cmd.fp, cmd_readbuf, i);
//				if(verbose) printf("Writing %d bytes\r\n", numread);
//				if(verbose)
//				{
//					for(j=0;j<numread;j++)
//					{
//						printf("%02x ", cmd_readbuf[j]);
//					}
//					printf("\r\n");
//					fflush(stdout);
//				}
//				err = LogData(fp, cmd_readbuf, numread);
//				if(verbose) printf("LogData returned %d\r\n", err);
			}
		} 
		}
	}

	if(verbose) printf("%s: Cleaning up\r\n", MODULE_NAME);
	RocCleanup();
	printf("%s: ROC stopped.\r\n",MODULE_NAME);

	if (gps_cmd.port) free(gps_cmd.port);
	if (gps_data.port) free(gps_data.port);
	if (data_dir) free(data_dir);
	if (curfilename) free(curfilename);

	exit(BCSUCCESS);
}



/// \brief Performs module initialization
///
/// Calls LoadConfigFileSettings() to load the configuration file settings for this module and calls
/// CmdLineHandler() to handle any command line options. Finally configures a ZMQ REP
/// socket to listen for messages from other modules.
///
/// \param argc Number of command line arguments passed to main().
/// \param argv[] List of command line arguments passed to main().
/// \return
/// - BCSUCCESS if successful.
/// - BCCONFIGREADERROR indicates an error in LoadConfigFileSettings().
/// - BCCMDLINEERROR indicates an error in CmdLineHandler().
/// - BCZMQOPTERROR indicates an error setting the options of a ZMQ socket.
/// - BCMODLISTERROR indicates an error in PopulateModules().
/// - BCZMQBINDERROR indicates an error binding the ZMQ REP socket.

int RocInit(int argc, char *argv[])
{
	int err, port;

	verbose = FALSE;
	quit = FALSE;

	fclose(stdin);

	// Read config file
	if(LoadConfigFileSettings(&cfgRoc, pathCfgRoc) != BCSUCCESS)
		return(BCCONFIGREADERROR);		

	// Parse command line options
	if(CmdLineHandler(argc, argv) != BCSUCCESS)
	{
		Usage();
		return(BCCMDLINEERROR);
	}

	// Set up serial ports
	err = SerialPortInit(&gps_data);
	if(err != BCSUCCESS)
		return(err);
	err = SerialPortInit(&gps_cmd);
	if(err != BCSUCCESS)
		return(err);
	
	// Set up data logging
	CreateDayDirectory();
	OpenLogFile(MODULE_NAME);
	clock_gettime(CLOCK_REALTIME, &now);	// use internal clock for now. Eventually get times from data blocks
	nextfile.tv_sec = now.tv_sec + file_len - ((now.tv_sec + file_len) % file_len);
	printf("now = %ld  len = %d  next = %ld\r\n", now.tv_sec, file_len, nextfile.tv_sec);
	nextfile.tv_nsec = now.tv_nsec;
	
	return(BCSUCCESS);
}


/// \brief Closes sockets and frees memory.
///
/// There will be some serial port clean up stuff in here.
///
/// \return Always returns BCSUCCESS

int RocCleanup(void)
{

	close(gps_cmd.fp);
	close(gps_data.fp);

	return(BCSUCCESS);
}


/// \brief Parses command line arguments.
///
/// \param argc Number of command line arguments passed to main().
/// \param argv[] List of command line arguments passed to main().
/// \return
/// - BCSUCCESS if successful
/// - BCERROR if there was an error parsing any of the command line options

int CmdLineHandler(int argc, char *argv[])
{
	int i;

	opterr=0;
	optind=0;	// Reinitialize getopt()

	while ((i = getopt (argc, argv, cloptions)) != -1)
    	switch (i)
		{
			case 'v':		// 'v' option turns on verbose mode
				verbose = TRUE;
				break;
			case 'l':		// 'l' option specifies data file length
//				file_len=get_baud(atoi(optarg));
				file_len=atoi(optarg);
				if((file_len < 0) || (file_len > 3600))
				{
					fprintf(stderr, "%s: Invalid file length setting %s\r\n", MODULE_NAME, optarg);
					return(BCERROR);
				}
				printf("File length = %d\r\n",file_len);
				break;
			case 'p':		// 'p' option specifies serial port to use
				free(gps_cmd.port);
				gps_cmd.port = malloc(strlen(optarg)+1);
				strcpy(gps_cmd.port, optarg);
				if(gps_cmd.port == NULL)
				{
					fprintf(stderr, "%s: Error setting serial port to %s\r\n",MODULE_NAME,optarg);
					return(BCERROR);
				}
				printf("Port = %s\r\n",gps_cmd.port);
				break;
			case 's':		// 's' option specifies serial port speed
				gps_cmd.speed=get_baud(atoi(optarg));
				gps_data.speed=get_baud(atoi(optarg));
				if(gps_cmd.speed == BCERROR)
				{
					fprintf(stderr, "%s: Invalid bits setting %s\r\n", MODULE_NAME, optarg);
					return(BCERROR);
				}
				printf("Speed = %d\r\n",gps_cmd.speed);
				break;
			case 'b':		// 'b' option specifies serial port bit settings
				if(verify_bits(optarg))
				{
					gps_cmd.bits = optarg[0]-'0';
					gps_cmd.parity = optarg[1];
					gps_cmd.stop = optarg[2]-'0';
					gps_data.bits = optarg[0]-'0';
					gps_data.parity = optarg[1];
					gps_data.stop = optarg[2]-'0';
				}
				else
				{
					fprintf(stderr, "%s: Invalid bits setting %s\r\n", MODULE_NAME, optarg);
					return(BCERROR);
				}
				printf("Bits = %d, Par = %c, Stop = %d\r\n",gps_cmd.bits, gps_cmd.parity,gps_cmd.stop);
				break;
			case 'f':		// 'f' option specifies serial port flow control
				if(verify_flow(optarg))
				{
					gps_cmd.flow = optarg[0];
					gps_data.flow = optarg[0];
				}
				else
				{
					fprintf(stderr, "%s: Invalid flow control setting %s\r\n", MODULE_NAME, optarg);
					return(BCERROR);
				}
				printf("Flow = %c\r\n",gps_cmd.flow);
				break;
			case '?':
				if ((optopt == 'f') || (optopt == 'p') || (optopt == 's') || (optopt == 'b') || (optopt == 'l'))
					fprintf (stderr, "%s: Option -%c requires an argument.\n", MODULE_NAME,optopt);
				else if (isprint (optopt))
					fprintf (stderr, "%s: Unknown option `-%c'.\n", MODULE_NAME,optopt);
				else
					fprintf (stderr,"%s: Unknown option character `\\x%x'.\n",MODULE_NAME,optopt);
				return(BCERROR);
			default:
				return(BCERROR);
		}
	return(BCSUCCESS);
}


/// \brief Prints the usage summary showing valid command line options.

void Usage(void)
{

	printf("Usage:\r\n roc [OPTIONS]\r\n\r\n");
	printf("Options:\r\n -b <bit settings>\tUse <bit settings> for the serial port (default = %s).\r\n",BITS_DEFAULT);
	printf(" -f [N | H | S]\tSpecifies what type of flow control to use (N=None, H=Hardware, S=Software) (default=%c).\r\n",FLOW_DEFAULT);
	printf(" -s <speed>\tConfigure the serial ports for <speed> bits per second (default = %d).\r\n",BAUD_DEFAULT);
	printf(" -l <length>\tCreate data files of length <length> seconds (default = %d).\r\n",FILE_LEN_DEFAULT);
	printf(" -v\tVerbose mode.\r\n");
}


/// \brief Reads the program configuration file
///
/// This function opens the module's configuration file (whose path is defined in the
/// pathCfgRoc global) and reads the contents. The currently supported settings that can
/// be placed in the config file are:
/// - (Optional) Port number to use for incoming socket connections (socket_in)
///
/// \param cfg Pointer to config_t object for storing state of config file operations.
/// \param path String containing full path of the config file to read.
/// \return
/// - BCSUCCESS if successful
/// - BCCONFIGREADERROR indicates an error reading the configuration file

int LoadConfigFileSettings(config_t *cfg, const char *path)
{
	const char *str;
	int count, i;

	config_init(cfg);

	if(config_read_file(cfg, path) == CONFIG_FALSE)
	{
		fprintf(stderr,"%s: Error %s in config file %s, line %d\r\n",MODULE_NAME, config_error_text(cfg),config_error_file(cfg),config_error_line(cfg));
		config_destroy(cfg);
		return(BCCONFIGREADERROR);
	}

	setting = config_lookup(cfg, "data_dir");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(str != NULL)
		{
			data_dir = malloc(strlen(str)+1);
			strcpy(data_dir, str);
			if(verbose) printf("%s: Data logged in %s.\r\n",MODULE_NAME, data_dir);
		}
		else
		{
			fprintf(stderr, "%s: Invalid directory name %s in config file. Using default location %s\r\n",MODULE_NAME, str, DIR_DEFAULT);
			data_dir = DIR_DEFAULT;
		}
	}
	else
	{
		data_dir = DIR_DEFAULT;
	}

	setting = config_lookup(cfg, "gps_cmd_port");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(str != NULL)
		{
			gps_cmd.port = malloc(strlen(str)+1);
			strcpy(gps_cmd.port, str);
			if(verbose) printf("%s: GPS commands on %s.\r\n",MODULE_NAME, gps_cmd.port);
		}
		else
		{
			fprintf(stderr, "%s: Invalid device name %s in config file.\r\n",MODULE_NAME, str);
			gps_cmd.port = NULL;
		}
	}
	else
	{
		gps_cmd.port = NULL;
	}

	setting = config_lookup(cfg, "gps_data_port");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(str != NULL)
		{
			gps_data.port = malloc(strlen(str)+1);
			strcpy(gps_data.port, str);
			if(verbose) printf("%s: GPS data on %s.\r\n",MODULE_NAME, gps_data.port);
		}
		else
		{
			fprintf(stderr, "%s: Invalid device name %s in config file.\r\n",MODULE_NAME, str);
			gps_data.port = NULL;
		}
	}
	else
	{
		gps_data.port = NULL;
	}

	setting = config_lookup(cfg, "gps_cmd_speed");
	if(setting != NULL)
	{
		gps_cmd.speed = get_baud(config_setting_get_int(setting));
		if(gps_cmd.speed == BCERROR)
		{
			fprintf(stderr, "%s: Invalid baud rate %d in config file %s. Using default vaule %d.\r\n",MODULE_NAME, gps_cmd.speed, path, BAUD_DEFAULT);
			gps_cmd.speed = BAUD_DEFAULT;
		}
		else
		{
			if(verbose) printf("%s: Using baud rate %d for serial port %s from config file %s.\r\n",MODULE_NAME, gps_cmd.speed, gps_cmd.port, path);
		}
	} 
	else
	{
		gps_cmd.speed = BAUD_DEFAULT;
	}

	setting = config_lookup(cfg, "gps_data_speed");
	if(setting != NULL)
	{
		gps_data.speed = get_baud(config_setting_get_int(setting));
		if(gps_data.speed == BCERROR)
		{
			fprintf(stderr, "%s: Invalid baud rate %d in config file %s. Using default vaule %d.\r\n",MODULE_NAME, gps_data.speed, path, BAUD_DEFAULT);
			gps_data.speed = BAUD_DEFAULT;
		}
		else
		{
			if(verbose) printf("%s: Using baud rate %d for serial port %s from config file %s.\r\n",MODULE_NAME, gps_data.speed, gps_data.port, path);
		}
	} 
	else
	{
		gps_data.speed = BAUD_DEFAULT;
	}

	setting = config_lookup(cfg, "bits");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(verify_bits(str))
		{
			gps_cmd.bits = str[0]-'0';
			gps_data.bits = str[0]-'0';
			gps_cmd.parity = str[1];
			gps_data.parity = str[1];
			gps_cmd.stop = str[2]-'0';
			gps_data.stop = str[2]-'0';
			if(verbose) printf("%s: Using serial bit settings %s.\r\n",MODULE_NAME, str);
		}
		else
		{
			fprintf(stderr, "%s: Invalid bits setting %s in config file. Using default value %s\r\n", MODULE_NAME, str, BITS_DEFAULT);
			gps_cmd.bits = 8;
			gps_cmd.parity = 'N';
			gps_cmd.stop = 1;
			gps_data.bits = 8;
			gps_data.parity = 'N';
			gps_data.stop = 1;
		}
	}
	else
	{
			gps_cmd.bits = 8;
			gps_cmd.parity = 'N';
			gps_cmd.stop = 1;
			gps_data.bits = 8;
			gps_data.parity = 'N';
			gps_data.stop = 1;
	}

	setting = config_lookup(cfg, "flow");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(verify_flow(str))
		{
			gps_cmd.flow = str[0];
			gps_data.flow = str[0];
			if(verbose) printf("%s: Using %c flow control.\r\n",MODULE_NAME, gps_cmd.flow);
		}
		else
		{
			fprintf(stderr, "%s: Invalid flow control setting %c in config file. Using default value %s\r\n", MODULE_NAME, str[0], FLOW_DEFAULT);
			gps_cmd.flow = FLOW_DEFAULT;
			gps_data.flow = FLOW_DEFAULT;
		}
	}
	else
	{
			gps_cmd.flow = FLOW_DEFAULT;
			gps_data.flow = FLOW_DEFAULT;
	}

	setting = config_lookup(cfg, "terminator");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(str != NULL)
		{
			strcpy(gps_cmd.term, str);
			strcpy(gps_data.term, str);
		}
		else
		{
			fprintf(stderr, "%s: Invalid line terminator specified. Using default value\r\n", MODULE_NAME);
			strcpy(gps_cmd.term, TERM_DEFAULT);
			strcpy(gps_data.term, TERM_DEFAULT);
		}
	}
	else
	{
		strcpy(gps_cmd.term, TERM_DEFAULT);
		strcpy(gps_data.term, TERM_DEFAULT);
	}

	setting = config_lookup(cfg, "file_len");
	if(setting != NULL)
	{
		file_len = config_setting_get_int(setting);
		if((file_len < 0) || (file_len > 3600))
		{
			fprintf(stderr, "%s: Invalid file length %d. Using default value %d\r\n", MODULE_NAME, FILE_LEN_DEFAULT);
			file_len = FILE_LEN_DEFAULT;
		}
		else
		{
			if(verbose) printf("%s: Using file length %d.\r\n",MODULE_NAME, file_len);
		}
	}
	else
	{
		file_len = FILE_LEN_DEFAULT;
	}

	config_destroy(cfg);
	return(BCSUCCESS);

}



int SerialPortInit(serial *port)
{
	struct termios options;

	printf("Opening port %s\r\n",port->port);
	port->fp = open(port->port, O_RDWR | O_NOCTTY | O_NDELAY);	// Open serial port
	if(port->fp == BCERROR)
		return(BCSEROPENERROR);

	tcgetattr(port->fp, &options);						// Get current port options

	cfsetispeed(&options, port->speed);				// Set input baud rate
	cfsetospeed(&options, port->speed);				// Set output baud rate

	options.c_cflag &= ~CSIZE; 							// Clear data bit fields
	switch(port->bits)
	{
		case 7:
			options.c_cflag |= CS7;						// Select 7 data bits
			break;
		case 8:
			options.c_cflag |= CS8;						// Select 8 data bits
			break;
		default:
			return(BCINVALIDBITS);
	}

	switch(port->parity)
	{
		case 'N':
			options.c_cflag &= ~PARENB;					// Clear parity enable bit
			options.c_iflag &= ~INPCK;					// Disable parity checking of incoming data
			break;
		case 'O':
			options.c_cflag |= PARENB;					// Set parity enable bit
			options.c_cflag |= PARODD;					// Set odd parity bit
			options.c_iflag |= (INPCK | ISTRIP);		// Enable parity checking of incoming data
			break;
		case 'E':
			options.c_cflag |= PARENB;					// Set parity enable bit
			options.c_cflag &= ~PARODD;					// Clear odd parity bit (which means use even parity)
			options.c_iflag |= (INPCK | ISTRIP);		// Enable parity checking of incoming data
			break;
		default:
			return(BCINVALIDPAR);
	}

	switch(port->stop)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;					// Clear stop bits bit (which means use 1 stop bit)
			break;			
		case 2:
			options.c_cflag |= CSTOPB;					// Set stop bits bit (which means use 2 stop bits)
			break;			
		default:
			return(BCINVALIDSTOP);
	}

	switch(port->flow)
	{
		case 'N':
			options.c_cflag &= ~CRTSCTS;				// Disable hardware flow control
			options.c_iflag &= ~(IXON | IXOFF | IXANY);	// Disable software flow control
			break;
		case 'H':
			options.c_cflag |= CRTSCTS;					// Enable hardware flow control
			options.c_iflag &= ~(IXON | IXOFF | IXANY);	// Disable software flow control
			break;
		case 'S':
			options.c_cflag &= ~CRTSCTS;				// Disable hardware flow control
			options.c_iflag |= (IXON | IXOFF | IXANY);	// Enable software flow control
			break;
		default:
			return(BCINVALIDFLOW);
	}

	options.c_iflag &= ~ICRNL;							// disable mapping of CR to NL
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOCTL | ECHOPRT | ECHOKE | ISIG);	// Turn off line read (canonical) mode and character echoes
	options.c_cflag |= CLOCAL;							// Set local mode
//	options.c_cflag &= ~CREAD;							// Disable the receiver for now
	options.c_cflag |= CREAD;							// Enable the receiver
	options.c_oflag &= ~OPOST;							// Disable all processed output settings
	options.c_cc[VMIN] = 0;								// Forces the read() function to return immediately with however many characters are available
	options.c_cc[VTIME] = 0;							// Tells the read() function not to wait for any data

	if(tcsetattr(port->fp, TCSANOW, &options) == 0)	// Make the new settings active
	{
		sleep(2);
		tcflush(port->fp,TCIOFLUSH);
		return(BCSUCCESS);
	}
	else
		return(BCSETATTRERROR);
}


int GPSStartupCmds(void)
{
	int count, i, ret, skipped;
	char *cmdstr=NULL, *confstr, *configfile;
	const char *str;
	FILE *cmdfile;
	ssize_t numread;
	size_t len=0;
	char reply[256];
	
	config_init(&cfgGps);

	if(config_read_file(&cfgGps, pathCfgGps)==CONFIG_FALSE)
	{
		config_destroy(&cfgGps);
		return(BCCONFIGREADERROR);
	}

	setting = config_lookup(&cfgGps, "startup");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(str != NULL)
		{
			configfile = malloc(strlen(str)+1);
			strcpy(configfile, str);
			if(verbose) printf("%s: Using GPS config file %s.\r\n",MODULE_NAME, configfile);
		}
		else
		{
			fprintf(stderr, "%s: Invalid GPS config file %s\r\n",MODULE_NAME, str);
			return(BCERROR);
		}
		if(configfile)
		{
			cmdfile=fopen(configfile, "r");
			if(!cmdfile)
			{
				fprintf(stderr, "%s: Error opening GPS config file %s\r\n", MODULE_NAME, configfile);
				return(BCERROR);
			}

			free(configfile);
		}
	}
	else
	{
		return(0);
	}

// getline allocates memory when cmdstr = NULL, to allow unknown length strings
	i = 0;
	while((numread = getline(&cmdstr, &len, cmdfile)) != -1)
	{
		if(ret = SendGpsCmd(cmdstr, 12000))
		{
			fprintf(stderr, "%s: Error %d sending command %s to GPS receiver.\r\n", MODULE_NAME, ret, cmdstr);
		}
		else
		{
			if (cmdstr) free(cmdstr);
			cmdstr = NULL;
			if(GetGpsReply(reply, 3000) == -1) fprintf(stderr, "%s: Timeout error waiting for GPS reply\r\n", MODULE_NAME);
			i++;
		}
	}
	fclose(cmdfile);
	if (cmdstr) free(cmdstr);
	config_destroy(&cfgGps);
	return(i);
}


int get_baud(int baud)
{
	switch (baud) {
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
	case 2500000:
		return B2500000;
	case 3000000:
		return B3000000;
	case 3500000:
		return B3500000;
	case 4000000:
		return B4000000;
	default: 
		return BCERROR;
	}
}


int verify_bits(const char *bits)
{
	if(((bits[0]=='7') || (bits[0]=='8')) && ((toupper(bits[1])=='N') || (toupper(bits[1])=='O') || (toupper(bits[1])=='E')) && ((bits[2]=='1') || (bits[2]=='2')))
		return(TRUE);
	else
		return(FALSE);
}


int verify_flow(const char *bits)
{
	if((toupper(bits[0])=='N') || (toupper(bits[0])=='H') || (toupper(bits[0])=='S'))
		return(TRUE);
	else
		return(FALSE);
}


char *TodaysDirectoryName(void)
{
	time_t epoch;
	struct tm *now;
	struct timespec ts;
	char *dirname;

	dirname = &todaysDirectoryName[0];
	epoch = time(NULL);
//	epoch = ts.tv_sec;
	now = gmtime(&epoch);
	sprintf(dirname,"%04d%02d%02d", (now->tm_year)+1900, (now->tm_mon)+1, now->tm_mday);
	return(dirname);
}


char *TodaysDirectoryPath(void)
{
	int len;
	char *dirname, *fullpath;

	dirname = TodaysDirectoryName();

	len = strlen(data_dir);
	fullpath = &todaysDirectoryPath[0];	// +3 is for 2 '/'s that we're adding plus the null terminator
	if(fullpath == NULL) return (NULL);
	strcpy(fullpath, data_dir);
	strcpy(fullpath + len, "/");
	strcpy(fullpath + len + 1, dirname);
	strcpy(fullpath + len + 1 + strlen(dirname), "/");
	fullpath[len + strlen(dirname) + 2] = 0;	// Null term
	return(todaysDirectoryPath);
}


int TodaysDirectoryExists(void)
{
	int i, found;
	DIR *dir;

	found = TRUE;

	dir = opendir(TodaysDirectoryPath());
	if(!dir)
		if(ENOENT == errno)		// Directory not found
		{
			found = FALSE;
		}
		else					// Some other error occurred
		{
			return(BCFILEOPENERROR);
		}
			
	if(verbose)
	{
		if(found) printf("%s: Found %s\r\n", MODULE_NAME, TodaysDirectoryName());
		else printf("%s: Did not find %s\r\n", MODULE_NAME, TodaysDirectoryName());
	}
	return(found);
}


int CreateDayDirectory(void)
{
	int i, ret;

	ret = mkdir(TodaysDirectoryPath(), 0777);
	if(ret)
		if(errno != EEXIST)		// Existing directories not an error, anything else is
		{
			return(errno);
		}

	return(BCSUCCESS);
}


char *LogFileName(char *name)
{
	time_t epoch;
	struct tm *now;
	char *rootname, *fullpath;
	struct timespec ts;
	char filename[50];	// format is YYYYMMDDhhmmss_name.txt

	epoch = time(NULL);
//	epoch = ts.tv_sec;
	now = gmtime(&epoch);
	sprintf(filename,"%s_%04d%02d%02d%02d%02d%02d.sbf", name, (now->tm_year)+1900, (now->tm_mon)+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

	if((rootname = TodaysDirectoryPath()) == NULL)
	{
		return(NULL);
	}

	fullpath = &logFileName[0];		// +1's are for the '/' and null term
	strcpy(fullpath, rootname);
//	strcpy(fullpath + strlen(rootname), "/");
	strcpy(fullpath + strlen(rootname), filename);
	fullpath[strlen(rootname) + strlen(filename)] = 0;

	return(logFileName);
}


int OpenLogFile(char *name)
{
	int i, index, ret;
	char *strName;
	char reply[256];

	SendGpsCmd("enoc, COM2, RMC\r", 1000);
	GetGpsReply(reply, 3000);
	
	if(!TodaysDirectoryExists())
	{
		if(CreateDayDirectory()) return(BCFILEERROR);
	}

	ret = BCSUCCESS;

	if((strName = LogFileName(name)) == NULL)
	{
		fprintf(stderr, "%s: Error generating log file name.\r\n", MODULE_NAME);
		return(BCFILEERROR);
	}

	if((fp = fopen(strName, "a+")) == NULL)
	{
		fprintf(stderr, "%s: Error Opening log file %s.\r\n", MODULE_NAME, strName);
		ret = BCFILEOPENERROR;
	}
	else
	{
		if(verbose) 
		{
			printf("%s: Opened log file %s.\r\n", MODULE_NAME, strName);
			fflush(stdout);
		}
		curfilename = malloc(strlen(strName));
		strcpy(curfilename, strName);
	}

	return(ret);
}


int CloseLogFile(void)
{
	int i, index, err, ret;

	ret = BCSUCCESS;

	if(err = fclose(fp))
	{
		fprintf(stderr, "%s: Error closing log file.\r\n", MODULE_NAME);
		ret = BCFILEERROR;
	}

	if(ret == BCSUCCESS)
	{
		if(verbose) printf("%s: Closed log file.\r\n", MODULE_NAME);
	}

	return(ret);
}


int NewFile(void)
{
//	led(ORANGE);
	CloseLogFile();
	CopyFile(curfilename);
	OpenLogFile(MODULE_NAME);
//	led(OFF);

	return(BCSUCCESS);
}


int LogData(FILE *fp, char *data, int n)
{
	int ret;
	
	if(fp==NULL)
	{
		fprintf(stderr,"%s: Error. Invalid log file stream\r\n", MODULE_NAME);
		return(BCFILEERROR);
	}
	ret = fwrite(data, 1, n, fp);
	fflush(fp);
	return(ret);
}


int CopyFile(char *name)
{
	char cmd[128];
	int err;
	
	sprintf(cmd,"cp %s %s", name, queueDir);
//	sprintf(cmd,"cp data.txt %s", queueDir);
	if((err==system(cmd)) < 0)
	{
		fprintf(stderr, "%s: Error copying data %s file to queue directory %s\r\n", MODULE_NAME, name, queueDir);
	} else {
		fprintf(stderr, "%s: data.txt copied\n", cmd);
	}
	return(err);
}


int SendGpsCmd(char *cmdstr, int timeout)
{
	size_t n;
	struct timespec spec;
	int prompt, i, numread, ptr;
	char inbuf[256];	// needs to be big enough to hold replies from commands
	double nowtime, endtime;
	
/* Get prompt to make sure GPS is ready to receive command.
	Return an error if prompt not received 
*/
	if(verbose) printf("%s: Sending GPS command %s", MODULE_NAME, cmdstr);
	clock_gettime(CLOCK_MONOTONIC, &spec);
	nowtime = spec.tv_sec + (spec.tv_nsec / 1000000000);
	endtime = nowtime + (timeout/1000);
	
	write(gps_cmd.fp, "\n", 1);		// send LF to get command prompt
	prompt=0;
	numread=0;
	inbuf[0]=0;
	ptr=0;
	
	while(!prompt && (nowtime < endtime))		// Wait until prompt (COMx>) is received
	{
		ioctl(gps_cmd.fp, FIONREAD, &i);
		if(i>0)
		{
			numread = read(gps_cmd.fp, inbuf+ptr, i);
			ptr += numread;
			inbuf[ptr] = 0;
			if(strstr(inbuf, "COM") && strstr(inbuf, ">"))
			{
				prompt = 1;
			}
		}
		clock_gettime(CLOCK_MONOTONIC, &spec);
		nowtime = spec.tv_sec + (spec.tv_nsec / 1000000000);
	}
	if(nowtime >= endtime)	return(-1);		// Timed out without receiving prompt

/* Send command
*/	
	n = write(gps_cmd.fp, cmdstr, strlen(cmdstr));		// Send command
	if(n != strlen(cmdstr)) return(-2);					// Some sort of write error occurred
	
	return(BCSUCCESS);

}


int GetGpsReply(char *replybuf, int timeout)
{
	size_t n;
	int endreply, i, numread, ptr;
	struct timespec spec;
	double nowtime, endtime;

	replybuf[0]=0;
	
	clock_gettime(CLOCK_MONOTONIC, &spec);
	nowtime = spec.tv_sec + (spec.tv_nsec / 1000000000);
	endtime = nowtime + (timeout/1000);
	
	endreply = 0;
	ptr = 0;
	while(!endreply && (nowtime < endtime))
	{
		ioctl(gps_cmd.fp, FIONREAD, &i);
		if(i>0)
		{
			numread = read(gps_cmd.fp, replybuf+ptr, 1);
			ptr += numread;
			replybuf[ptr] = 0;
			if(strstr(replybuf, "COM") && strstr(replybuf, ">"))
			{
				endreply = 1;
			}
		}
		clock_gettime(CLOCK_MONOTONIC, &spec);
		nowtime = spec.tv_sec + (spec.tv_nsec / 1000000000);
	}
	if(nowtime >= endtime)	return(-1);		// Timed out without receiving prompt
	
	if(verbose) printf("%s: GPS Reply = %s",MODULE_NAME, replybuf);
	return(strlen(replybuf));
}


/**********************************
*
* gps_power(), gps_reset(), gps_power_toggle(), set_safe(), led(), set_gpio0()
*
* Description: These functions control the digital outputs of the TS-7680.
*
* Parameter: int state
*
* Return value: None
*
* These functions hide the active hi/lo nature of the signals so they can be
*   called as gps_power(ON), etc. without worrying about whether that's a
*   logic hi or lo. The argument to led() should be one of: GREEN, RED, ORANGE,
*   or OFF.
*
************************************/


void gps_power(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"gpio gps_power 0");
	}
	else
	{
		sprintf(cmdstr,"gpio gps_power 1");
	}

	system(cmdstr);
}


void gps_reset(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"gpio gps_reset 0");
	}
	else
	{
		sprintf(cmdstr,"gpio gps_reset 1");
	}

	system(cmdstr);
}


void gps_power_toggle(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"gpio gps_pow_tog 0");
	}
	else
	{
		sprintf(cmdstr,"gpio gps_pow_tog 1");
	}

	system(cmdstr);
}


void set_safe(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"gpio safe 1");
	}
	else
	{
		sprintf(cmdstr,"gpio safe 0");
	}

	system(cmdstr);
}


void led(int state)
{
	char cmdstr[32];
	
	switch(state)
	{
		case RED:
			sprintf(cmdstr,"gpio led1 0");
			system(cmdstr);
			sprintf(cmdstr,"gpio led0 1");
			system(cmdstr);
			break;
		case GREEN:
			sprintf(cmdstr,"gpio led0 0");
			system(cmdstr);
			sprintf(cmdstr,"gpio led1 1");
			system(cmdstr);
			break;
		case ORANGE:
			sprintf(cmdstr,"gpio led0 1");
			system(cmdstr);
			sprintf(cmdstr,"gpio led1 1");
			system(cmdstr);
			break;
		case OFF:
			sprintf(cmdstr,"gpio led0 0");
			system(cmdstr);
			sprintf(cmdstr,"gpio led1 0");
			system(cmdstr);
			break;
	}		
}


void set_gpio0(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"gpio gpio0 1");
	}
	else
	{
		sprintf(cmdstr,"gpio gpio0 0");
	}

	system(cmdstr);
}


void led_blink(int color, int num, int duration)
{
	
}



