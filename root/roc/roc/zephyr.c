/// \copyright 2018 David Jabson, Brainstorm Engineering
///
/// \file zephyr.c
/// \brief ROC2 Zephyr Interface Module
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
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <dirent.h>
#include "zephyr.h"

/// \brief Main function.
///
/// Calls ZephyrInit() to perform initialization then enters the main while loop where it
/// .... 
/// the main loop stops, then ZephyrCleanup() is called before exiting.

#define SEPTENTRIOTOGLNG 300000

int main (int argc, char *argv[])
{
	int err, acked, i, numread, j;
	unsigned char *endptr, *readptr;
	unsigned char term, readbuf[INBUFSIZE], linebuf[INBUFSIZE/2];
	struct termios options;
//	struct timespec curTime;
	time_t Timer;
	char *nextFile;
	enum msg InMsgType;
	char InMsgTypeStr[8];
	time_t nextS=0, nextTm=0;
	
	struct pollfd fds[1];

	err = ZephyrInit(argc, argv);
//	led(RED);
	sendIMRatPower = 1;
	if(err<0)
	{
		fprintf(stderr, "%s: Initialization error %d. Exiting\r\n",MODULE_NAME,err);
		if(hwversion>2) led(RED);
	}
	else 
	{
		printf("SWVersion = %s\r\nSWDate = %s\r\n",SWVersion,SWDate);
		printf("%s: Module started.\r\n",MODULE_NAME);
//		write(zephyr.fp, "ROC online\r\n", 12);
		if(hwversion>2) led(GREEN);
	}
	
	term=zephyr.term[strlen(zephyr.term)-1];
	readptr=readbuf;
	memset(readbuf, 0, sizeof(readbuf));

	fds[0].fd = zephyr.fp;
	fds[0].events = POLLIN;

//	tcgetattr(serial.fp, &options);						// Get current port options
//	options.c_cflag |= CREAD;							// Enable the serial receiver
//	tcflush(serial.fp,TCIOFLUSH);						// Flush serial buffers
//	if(tcsetattr(serial.fp, TCSANOW, &options) == 0)	// Make the new settings active

	Timer = time(NULL);

	while(!quit)
	{
		if (Timer != time(NULL)) {
			Timer = time(NULL);		// set for next second

			if (waitingAck) {
				waitingAck -= 1;
			}
			if (waitingIM) {
				waitingIM -= 1;
			}
			if(safeTimeout)	{ 
				safeTimeout -= 1;
				if (safeTimeout == 0) { // Haven't heard from Zephyr in over 2 hours, enter Standby mode
					safeTimeout = SAFETIMEOUT;
					InstMode = SB;
					safeAck = 0;
				}
			}

#if 0
// This section checks to make sure we are not receiving chars in Low Power mode
// and that we are receiving chras in non-Low Power mode
			if(lowPowerCheck)	{ 
				lowPowerCheck -= 1;
				if (lowPowerCheck == 0) { // Check to make sure we are not receiing chars in Low Power mode
					lowPowerCheck = LOWPOWERCHECK;
					if (verbose)
						printf("%s:%d: Check Mode %d Power State with %d GPS Chars: %d\r\n",
							MODULE_NAME, Timer, InstMode, gotGPSChars, lowPowerCheckWait);
					if (InstMode == LP) {
						if (lowPowerCheckWait == 0) {
							if (gotGPSChars > 100) {
								if (verbose) printf("%s: Got %d GPS Chars in LP Mode - Toggle Low Power Line\r\n", MODULE_NAME, gotGPSChars);
								system("echo \"0\"   > /sys/class/gpio/gpio202/value");			// Set value to low
								usleep(SEPTENTRIOTOGLNG);	// 300 msecs
								system("echo \"1\"   > /sys/class/gpio/gpio202/value");			// Set value to high
							}
							gotGPSChars = 0;
							lowPowerCheckWait = 5;
							discardFirstFile = 1;
						} else {
							lowPowerCheckWait -= 1;
						}
					} else {
						if (lowPowerCheckWait == 0) {
							if (gotGPSChars < 100) {
								if (verbose) printf("%s: Only %d GPS Chars in NON LP Mode - Toggle Low Power Line\r\n", MODULE_NAME, gotGPSChars);
								system("echo \"0\"   > /sys/class/gpio/gpio202/value");			// Set value to low
								usleep(SEPTENTRIOTOGLNG);	// 300 msecs
								system("echo \"1\"   > /sys/class/gpio/gpio202/value");			// Set value to high
							}
							gotGPSChars = 0;
							lowPowerCheckWait = 8;
						} else {
							lowPowerCheckWait -= 1;
						}
					}
				}
			}
#endif
		}
		
//		if(time(NULL) == nextfile.tv_sec)
//		{
//			nextfile.tv_sec+=file_len;
//			NewFile();
//		}
		usleep(200000);		// sleep for 0.2s
		
//		if(time(NULL) > ackTimeout) waitingAck=0;
		
		switch(InstMode)
		{
			case FL:		// Flight mode. Check for data files to offload
		//	    if (verbose) printf("%s: FL Set Safe Output to Low\r\n", MODULE_NAME);
				sendIMRatPower = 0;
				switch(hwversion)
				{
					case 3:
						set_safe(OFF);
						break;
					default:
						system("echo \"0\"   > /sys/class/gpio/gpio193/value");			// Set SAFE digital output to lo
						system("echo \"1\"   > /sys/class/gpio/gpio233/value");			// Set 5V On
						break;
				}
				if(modeSwitch)	// coming out of low power mode, reset GPS
				{
					modeSwitch=0;
// Sepentrio Wakeup
					if (wakeUpFlag) {
						wakeUpFlag = 0;
						switch(hwversion)
						{
							case 3:
								if (verbose) printf("%s: Entering FL mode. GPS power on.\r\n", MODULE_NAME);
								gps_power(ON);
								break;
							default:
								if (verbose) printf("%s: Toggle Wakeup to Septentrio - Flight Mode\r\n", MODULE_NAME);
								system("echo \"0\"   > /sys/class/gpio/gpio202/value");			// Set value to low
								usleep(SEPTENTRIOTOGLNG);	// 300 msecs
								system("echo \"1\"   > /sys/class/gpio/gpio202/value");			// Set value to high
								lowPowerCheckWait = 16;
								lowPowerCheck = LOWPOWERCHECK;
								gotGPSChars = 0;
								break;
						}
					}
				}

				if(waitingAck==0)
				if(nextFile = GetNextFile())
				{
					if (strstr(nextFile,"list.txt")) {
						TCflag = 1;
						if(TClastPart==TCnumParts)		// we finished the previous file, starting a new one
						{
							strcpy(TCfileOffload, nextFile);
						}
						TCSendFile(TCfileOffload);
					} else {
						TCflag = 0;
						if(lastPart==numParts)		// we finished the previous file, starting a new one
						{
							strcpy(fileOffload, nextFile);
						}
						SendFile(fileOffload);
					}
				}
				break;

			case SB:		// Standby mode. Send IMR msg once per minute until we get an IM msg that forces a mode change
				if (modeSwitch) {
					modeSwitch = 0;
					if (wakeUpFlag && poweroffgps) {
						switch(hwversion)
						{
							case 3:
//								if (verbose) printf("%s: Entering SB mode. GPS power off.\r\n", MODULE_NAME);
//								gps_power(OFF);
								break;
							default:
								if (verbose) printf("%s: Toggle Wakeup to Septentrio - Flight Mode\r\n", MODULE_NAME);
								system("echo \"0\"   > /sys/class/gpio/gpio202/value");			// Set value to low
								usleep(SEPTENTRIOTOGLNG);	// 300 msecs
								system("echo \"1\"   > /sys/class/gpio/gpio202/value");			// Set value to high
								break;
						}
						wakeUpFlag = 0;
						poweroffgps = 0;
						}
					nextImr = time(NULL)+5;
				}
		//	    if (verbose) printf("%s: SB Set Safe Output to Low\r\n", MODULE_NAME);
				switch(hwversion)
				{
					case 3:
						set_safe(OFF);
						break;
					default:
						system("echo \"0\"   > /sys/class/gpio/gpio193/value");			// Set SAFE digital output to lo
						system("echo \"1\"   > /sys/class/gpio/gpio233/value");			// Set 5V On
						break;
				}
				if(time(NULL) > nextImr)
				{
					if (sendIMRatPower) {
						if(verbose) printf("%s: Sending IMR message\r\n", MODULE_NAME);
						SendMsg(IMR, "", 0);
						waitingIM = WAITFORIM;
					}
					nextImr = time(NULL)+60;
				}

//				if(time(NULL) > nextTm)
//				{
//					if(verbose) printf("%s: Sending TM housekeeping message\r\n", MODULE_NAME);
//					dataTm = 0;
//					SendMsg(TM, "", 0);
//					nextTm = time(NULL)+60;
//				}

				if((nextFile = GetNextFile()) && !waitingAck && !waitingIM)
				{
					if (strstr(nextFile,"list.txt")) {
						TCflag = 1;
						if(TClastPart==TCnumParts)		// we finished the previous file, starting a new one
						{
							strcpy(TCfileOffload, nextFile);
						}
						TCSendFile(TCfileOffload);
					} else {
						TCflag = 0;
						if(lastPart==numParts)	{	// we finished the previous file, starting a new one
							dataTm = 0;
							SendMsg(TM, "", 0);
							waitingAck=WAITFORACK;
							RemoveFile(nextFile);
						} else {
							SendFile(fileOffload);
						}
					}
				}
				break;

			case LP:		// Low power mode. Put GPS in standby.
		//	    if (verbose) printf("%s: LP Set Safe Output to Low\r\n", MODULE_NAME);
				switch(hwversion)
				{
					case 3:
						set_safe(OFF);
						break;
					default:
						system("echo \"0\"   > /sys/class/gpio/gpio193/value");			// Set SAFE digital output to lo
						system("echo \"0\"   > /sys/class/gpio/gpio233/value");			// Set 5V off
						break;
				}
				if(modeSwitch)
				{
					modeSwitch=0;
//					system("echo \"exePowerMode StandBy\" > /dev/ttyS3");		// Ugly hack for now (use this one for VM testing)
//					if (verbose) printf("%s: Send Low Power Command to Septentrio\r\n", MODULE_NAME);
//					system("echo \"exePowerMode, StandBy\" > /dev/ttyAPP2");		// Ugly hack for now (use this one for flight HW)
					switch(hwversion)
					{
						case 3:
							if (verbose) printf("%s: Entering LP mode. GPS off.\r\n", MODULE_NAME);
							gps_power(OFF);
							break;
						default:
							if (verbose) printf("%s: Toggle Power Down to Septentrio - Flight Mode\r\n", MODULE_NAME);
							system("echo \"0\"   > /sys/class/gpio/gpio202/value");			// Set value to low
							usleep(SEPTENTRIOTOGLNG);	// 300 msecs
							system("echo \"1\"   > /sys/class/gpio/gpio202/value");			// Set value to high
							break;
					}
					lowPowerCheckWait = 5;
					gotGPSChars = 0;
					poweroffgps = 1;
					discardFirstFile = 1;											// Flag that power off command to GPS sent
				}
//				nextImr = time(NULL) + 10000000;
//				nextTm = time(NULL)+60;

				if((nextFile = GetNextFile()) && !waitingAck && !waitingIM)
				{
					if (strstr(nextFile,"list.txt")) {
						TCflag = 1;
						if(TClastPart==TCnumParts)		// we finished the previous file, starting a new one
						{
							strcpy(TCfileOffload, nextFile);
						}
						TCSendFile(TCfileOffload);
					} else {
						TCflag = 0;
						if(lastPart==numParts)	{	// we finished the previous file, starting a new one
							dataTm = 0;
							SendMsg(TM, "", 0);
							waitingAck=WAITFORACK;
							RemoveFile(nextFile);
						} else {
							SendFile(fileOffload);
						}
					}
				}
				break;

			case SA:		// Safety mode.
				if(hwversion<3) system("echo \"1\"   > /sys/class/gpio/gpio233/value");			// Set 5V On
				if (modeSwitch) {
					nextS = time(NULL) + 60;
					safeAck = 0;
					SendMsg(S, "", 0);		// We need to send an S msg every time Zephyr sends us IM.S					
					modeSwitch = 0;
#if 0
					if (wakeUpFlag && poweroffgps) {
						if (verbose) printf("%s: Toggle Wakeup to Septentrio - Flight Mode\r\n", MODULE_NAME);
						system("echo \"0\"   > /sys/class/gpio/gpio200/value");			// Set value to low
						usleep(500000);	// 500 msecs
						system("echo \"1\"   > /sys/class/gpio/gpio200/value");			// Set value to high
						wakeUpFlag = 0;
						poweroffgps = 0;
					}
#endif
				}

				if((time(NULL) > nextS) /*&& !safeAck*/)
				{
					SendMsg(S, "", 0);
					nextS = time(NULL)+60;
					safeAck = 0;
				}

//	    		if (verbose) printf("%s: SA Set Safe Output to High\r\n", MODULE_NAME);
				switch(hwversion)
				{
					case 3:
						set_safe(ON);
						break;
					default:
						system("echo \"1\"   > /sys/class/gpio/gpio193/value");			// Set SAFE digital output to high
						break;
				}
//				nextImr = time(NULL) + 10000000;

				if ((nextFile = GetNextFile()) && !waitingAck)
				{
					if (strstr(nextFile,"list.txt")) {
						TCflag = 1;
						if(TClastPart==TCnumParts)		// we finished the previous file, starting a new one
						{
							strcpy(TCfileOffload, nextFile);
						}
						TCSendFile(TCfileOffload);
					} else {
						TCflag = 0;
						if(lastPart==numParts)	{	// we finished the previous file, starting a new one
							dataTm = 0;
							SendMsg(TM, "", 0);
							waitingAck=WAITFORACK;
							RemoveFile(nextFile);
						} else {
							SendFile(fileOffload);
						}
					}
				}

				break;
				
			case EF:		// End flight mode. Shutdown, stop all Zephyr comms.
//				system("echo \"0\"   > /sys/class/gpio/gpio233/value");			// Set 5V Off
//				shutdown = TRUE;	// I thought we were supposed to shut down here but that's apparently not correct.
//				quit=1;				// Just do nothing for now.
				if (modeSwitch) {
					modeSwitch = 0;
//					system("echo \" \" > /dev/ttyAPP2");		// wakeup
				}

//				nextImr = time(NULL) + 10000000;
				if((nextFile = GetNextFile()) && (waitingAck==0))
				{
					RemoveFile(nextFile);
				}

				safeTimeout = SAFETIMEOUT;
				break;
			default:
				break;
		}
		
		if(poll (fds, 1, 10)) {
//			if ((fds [1].revents & POLLIN) || ((data_readptr-1) > data_endptr))
			if (fds [0].revents & POLLIN)		// incoming data on Zephyr serial port
			{
				err = ioctl(zephyr.fp, FIONREAD, &i);
				if(err)
				{
					fprintf(stderr,"%s: I/O error, exiting.",MODULE_NAME);
					shutdown = FALSE;
					quit=TRUE;
				}

	//			if(verbose) printf("%d bytes ready to read from %s\r\n",i, gps_data.port);
				if((readptr + i - readbuf) > INBUFSIZE)	// Serial read will overrun buffer if we do it
				{
					fprintf(stderr, "%s: Serial buffer overrun. Data lost!\r\n", MODULE_NAME);
					readptr=readbuf;
					memset(readbuf, 0, sizeof(readbuf));
				}
				else
				{
					safeTimeout = SAFETIMEOUT;	// Enter safety mode if no data from Zephyr in 10 minutes
	//				printf("readbuf=%x  readptr=%x\r\n",readbuf,readptr);
					numread = read(zephyr.fp, readptr, i);
					readptr += numread;
	//				printf("first char = 0x%d\r\n",readbuf[0]);
					if(strstr(readbuf, "</CRC>\n"))			// Found end of a message
					{
						readptr=readbuf;
						if (verbose) printf(readbuf);
						fflush(stdout);
						while(endptr=strstr(readptr, "</CRC>\n"))
						{
							sscanf(readptr,"<%s>",InMsgTypeStr);
							InMsgTypeStr[strlen(InMsgTypeStr)-1]=0;		// Remove trailing >
							InMsgType=MsgType(InMsgTypeStr);
							if(verbose) printf("%s: Received message type %d\r\n",MODULE_NAME, InMsgType);
							MsgHandler(InMsgType, readbuf);
							readptr=endptr+strlen("</CRC>\n");

							nextMsgTime.tv_nsec += 300000000;						// next message (1 sec from now)
							if (nextMsgTime.tv_nsec > 1000000000) {
								nextMsgTime.tv_nsec -= 1000000000;						// next message (1 sec from now)
								nextMsgTime.tv_sec += 1;						// next message (1 sec from now)
							}
						}
						readptr=readbuf;
						memset(readbuf, 0, sizeof(readbuf));
					}
					else
					{
	//					printf("read %d bytes\r\n",numread);
	//					printf(readbuf);
	//					fflush(stdout);
					}
						
				}
			}
		} 
//		else
//		{
//			if(time(NULL) > safeTimeout)	// Haven't heard from Zephyr in over 2 hours, enter Safety mode
//			{
//				InstMode = SA;
//			}
//		}
	}

	ZephyrCleanup();
	printf("%s: Module stopped.\r\n",MODULE_NAME);

	exit(BCSUCCESS);
}


int MsgHandler(enum msg msgtype, char *msgdata)
{
	switch(msgtype)
	{
		case IM:
			return(HandleIM(msgdata));
			break;
		case SAck:
			return(HandleSAck(msgdata));
			break;
		case SW:
			return(HandleSW(msgdata));
			break;
		case TMAck:
			if(HandleTMAck(msgdata))
			{
				waitingAck=0;
				if(dataTm)
				{
					if (TCflag) {
						TClastPart++;
						if(verbose) printf("%s: Rcvd TMAck. File %s Part %d/%d\r\n",MODULE_NAME,TCfileOffload,TClastPart,TCnumParts);
						if(TClastPart==TCnumParts)		// done with file transfer
						{
							RemoveFile(TCfileOffload);
							TCflag = 0;
						}
					} else {
						lastPart++;
						if(verbose) printf("%s: Rcvd TMAck. File %s Part %d/%d\r\n",MODULE_NAME,fileOffload,lastPart,numParts);
						if(lastPart==numParts)		// done with file transfer
						{
							RemoveFile(fileOffload);
						}						
					}
					return(1);
				}
			}
			else
			{
				return(0);
			}
			break;
		case TC:
			return(HandleTC(msgdata));
			break;
		case GPS:
			return(HandleGPS(msgdata));
			break;
		default:
			return(-1);
			break;
	}
}


int HandleIM(char *msgdata)
{
	char modestr[3]="";
	char *loc;
	enum mode newmode;

	waitingIM = 0;
	
	loc=strstr(msgdata, "<Mode>");
	if (loc == 0) {
		if(verbose) printf("%s: HandleIM <Mode> not found, ERROR!!!\r\n",MODULE_NAME);
		return;
	}
	strncpy(modestr,loc+strlen("<Mode>"),2);
	if(verbose) printf("%s: Switching to mode %s\r\n",MODULE_NAME, modestr);
	LastMode = InstMode;		// Save current inst mode
	if(strncmp(modestr, "FL", 2)==0)	///< Switch to flight mode
	{
		newmode=FL;
	}
	else if(strncmp(modestr, "SB", 2)==0)	///< Switch to Standby mode
	{
		newmode=SB;
	}
	else if(strncmp(modestr, "LP", 2)==0)	///< Switch to Low Power mode
	{
		newmode=LP;
	}
	else if(strncmp(modestr, "SA", 2)==0)	///< Switch to Safety mode
	{
		newmode=SA;
	}
	else if(strncmp(modestr, "EF", 2)==0)	///< Switch to End Flight mode
	{
		newmode=EF;
	}
	else
	{
		fprintf(stderr, "%s: Unknown mode change request: %s\r\n", MODULE_NAME, modestr);
		return(-1);
	}

	if (verbose) printf("%s: Now in mode %d\r\n", MODULE_NAME, newmode);
	if ((InstMode != newmode) || (newmode==SA)) {
		modeSwitch = 1;
// Do wake up when coming out of LP,SA or EF since we may have been gone to SA or EF from LP
		if (InstMode == LP) {
//			if ((newmode == FL) || (newmode == SB)) {
				wakeUpFlag = 1;	// only toggle wake up after low power mode
//			}
		}

// Do wake up when coming out of SA or EF since we may have been gone to SA or EF from LP
//		if ((InstMode == SA) || (InstMode == EF)) {
//			wakeUpFlag = 1;	// only toggle wake up after low power mode
//		}
	} 
	InstMode = newmode;
	SendMsg(IMAck, "", 0);
	//if(newmode==SA)
	//{
	//	SendMsg(S, "", 0);		// We need to send an S msg every time Zephyr sends us IM.S
	//}
//	nextImr = time(NULL) + 10000000;		// Set timer way into future so no more IMR msgs go out
//	return(newmode);
}


int HandleSAck(char *msgdata)
{
	char ackstr[4]="";
	char *loc;
	
	loc=strstr(msgdata, "<Ack>");
	if (loc == 0) {
		if(verbose) printf("%s: HandleSAck <Ack> not found, ERROR!!!\r\n", MODULE_NAME);
		return (0);
	}
	strncpy(ackstr,loc+strlen("<Ack>"),3);
	if(verbose) printf("%s: Ack is %s\r\n", MODULE_NAME, ackstr);
	if(!strncmp(ackstr,"ACK",3)) 
	{
		safeAck = 1;
	}
	else 
	{
		safeAck = 0;
	}
	
	return(safeAck);

}


int HandleSW(char *msgdata)
{
	if(verbose) printf("%s: Received shutdown warning.\r\n", MODULE_NAME);

}


int HandleTMAck(char *msgdata)
{
	char ackstr[4]="";
	char *loc;
	
	loc=strstr(msgdata, "<Ack>");
	if (loc == 0) {
		if(verbose) printf("%s: Handle TMAck <Ack> not found, ERROR!!!\r\n", MODULE_NAME);
		return (0);
	}
	strncpy(ackstr,loc+strlen("<Ack>"),3);
	if(verbose) printf("%s: Ack is %s\r\n", MODULE_NAME, ackstr);
	if(!strncmp(ackstr,"ACK",3)) return(1);
	else return(0);
}


int HandleTC(char *msgdata)
{
	char lenstr[8]="";
	char cmdstr[128];
	char *loc, *payload, *filename;
	int len, doit;
	FILE *f;
	
	loc=strstr(msgdata, "<Length>");
	if (loc == 0) {
		if(verbose) printf("%s: HandleTC <Length> not found, ERROR!!!\r\n", MODULE_NAME);
		return (0);
	}
	sscanf(loc+strlen("<Length>"),"%d",&len);
	if(verbose) printf("%s: Rcvd TC msg. Payload is %d bytes\r\n", MODULE_NAME, len);
	payload=malloc(len);
	
	loc=strstr(msgdata, "</CRC>\n")+strlen("</CRC>\n");		// point to beginning of binary section
	if (loc == 0) {
		if(verbose) printf("%s: HandleTC </CRC> not found, ERROR!!!\r\n", MODULE_NAME);
		free(payload);
		return (0);
	}
	//printf("LOC:%c", loc[0]);
	loc+=strlen("START");									// skip over binary section header
	memcpy(payload, loc, len);								// copy payload into buffer
	//printf(":%c:%2.2s\n", loc[0], payload);
	
/*	loc=strstr(payload,"!!");								// search for !! that separates filename from data
	filename=malloc(strlen(pathUpload)+(loc-payload));		// allocate space for name of upload file
	memcpy(filename, pathUpload, strlen(pathUpload));		// copy upload directory into filename
	memcpy(filename+strlen(pathUpload), payload, loc-payload);				// copy filename
	loc+=2;													// point to beginning of data
	
	f = fopen(filename, "w");
	printf("Writing %s to file %s\r\n", loc, filename);
	fwrite(loc, 1, len-(loc-payload), f);
	fclose(f); */
	
	SendMsg(TCAck, "", 0);
	
	if(memmem(payload, len, "ls", strlen("ls")))	// do "ls -1" on queue and put results in file for offload
	{
		sprintf(cmdstr,"ls -l /data/roc/queue > /data/roc/queue/list.txt");
		doit=1;
	}
	else if(memmem(payload, len, "reboot", strlen("reboot")))	// reboot ROC
	{
		sprintf(cmdstr,"reboot");	
		doit=1;
	}
	else if(memmem(payload, len, "clear", strlen("clear")))	// clear download queue
	{
		sprintf(cmdstr,"rm %s/*",queueDir);
		doit=0;
	}
	if(verbose) printf("%s: Executing command '%s'\r\n", MODULE_NAME, cmdstr);
	if(doit) system(cmdstr);

	
	free(payload);
//	free(filename);
}


int HandleGPS(char *msgdata)
{
	if(verbose) printf("%s: Received GPS message.\r\n", MODULE_NAME);
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

int ZephyrInit(int argc, char *argv[])
{
	int err, port;

	verbose = FALSE;
	quit = FALSE;

	fclose(stdin);

	// Read config file
	if(LoadConfigFileSettings(&cfgZephyr, pathCfgZephyr) != BCSUCCESS)
		return(BCCONFIGREADERROR);		

	// Parse command line options
	if(CmdLineHandler(argc, argv) != BCSUCCESS)
	{
		Usage();
		return(BCCMDLINEERROR);
	}

	// Set up serial ports
	err = SerialPortInit(zephyr);
	if(err != BCSUCCESS)
		return(err);
	
	// Set up data logging
//	CreateDayDirectory();
//	OpenLogFile(MODULE_NAME);
//	clock_gettime(CLOCK_REALTIME, &now);	// use internal clock for now. Eventually get times from data blocks
//	nextfile.tv_sec = now.tv_sec + file_len;
//	nextfile.tv_nsec = now.tv_nsec;
	
	poweroffgps = 0;
	MsgID = 0;
	InstMode = SB;
	modeSwitch = 0;
	wakeUpFlag = 0;
	safeAck = 0;
	numParts = TCnumParts = 0;
	lastPart = TClastPart = 0;
	waitingAck = 0;
	waitingIM = 0;
	shutdown = FALSE;
	safeTimeout = SAFETIMEOUT;		// Enter Safe mode if we don't hear from Zephyr in 10 minutes
	lowPowerCheck = LOWPOWERCHECK;	// Time Interval to check for low power state
	lowPowerCheckWait = 5;			// wait an extra cycle before checking
	discardFirstFile = 0;
	clock_gettime(CLOCK_REALTIME, &nextMsgTime);	// OK to send msg at any time
	nextImr = time(NULL)+5;

	// Set up named pipe
	printf("%s: mkfifo returned %d\r\n", MODULE_NAME, mkfifo(ipcPipe, 0666));
	pipefp = fopen(ipcPipe, "r+");
	if(pipefp)
	{
		if(verbose) printf("%s: Opened named pipe successfully\r\n", MODULE_NAME);
	}
	else
	{
		fprintf(stderr, "%s: Attempt to open named pipe returned %d\r\n", MODULE_NAME, errno);
	}
	
	switch(hwversion)
	{
		case 3:
			gps_power(ON);	// Note: this also turns on power to LED's
			break;
		default:
			if (verbose) printf("%s: Init: Set Safe Export\r\n", MODULE_NAME);
			system("echo \"193\" > /sys/class/gpio/export");				// Prepare SAFE digital output
			if (verbose) printf("%s: Init: Set Safe Direction\r\n", MODULE_NAME);
			system("echo \"out\" > /sys/class/gpio/gpio193/direction");		// Set as output
			if (verbose) printf("%s: Init: Set Safe Low\r\n", MODULE_NAME);
			system("echo \"0\"   > /sys/class/gpio/gpio193/value");			// Set value to low
	
			if (verbose) printf("%s: Init: Set nRST Export\r\n", MODULE_NAME);
			system("echo \"200\" > /sys/class/gpio/export");				// Prepare SAFE digital output
			if (verbose) printf("%s: Init: Set nRST Direction\r\n", MODULE_NAME);
			system("echo \"out\" > /sys/class/gpio/gpio200/direction");		// Set as output
			if (verbose) printf("%s: Init: Set nRST Low\r\n", MODULE_NAME);
			system("echo \"1\"   > /sys/class/gpio/gpio200/value");			// Set value to high

			if (verbose) printf("%s: Init: Set TogglePwr Export\r\n", MODULE_NAME);
			system("echo \"202\" > /sys/class/gpio/export");				// Prepare SAFE digital output
			if (verbose) printf("%s: Init: Set nRST Direction\r\n", MODULE_NAME);
			system("echo \"out\" > /sys/class/gpio/gpio202/direction");		// Set as output
			if (verbose) printf("%s: Init: Set nRST Low\r\n", MODULE_NAME);
			system("echo \"1\"   > /sys/class/gpio/gpio202/value");			// Set value to high

			if (verbose) printf("%s: Init: Set 5VPwr Export\r\n", MODULE_NAME);
			system("echo \"233\" > /sys/class/gpio/export");				// Prepare SAFE digital output
			if (verbose) printf("%s: Init: Set 5VPwr Direction\r\n", MODULE_NAME);
			system("echo \"out\" > /sys/class/gpio/gpio233/direction");		// Set as output
			if (verbose) printf("%s: Init: Set 5VPwr High(on)\r\n", MODULE_NAME);
			system("echo \"1\"   > /sys/class/gpio/gpio233/value");			// Set value to high
			break;
	}
	
	return(BCSUCCESS);
}


/// \brief Closes sockets and frees memory.
///
/// There will be some serial port clean up stuff in here.
///
/// \return Always returns BCSUCCESS

int ZephyrCleanup(void)
{

	if (zephyr.port) free(zephyr.port);
	if (data_dir) free(data_dir);

	close(zephyr.fp);
	if(shutdown) system("shutdown now");
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
//			case 'l':		// 'l' option specifies data file length
//				file_len=get_baud(atoi(optarg));
//				if((file_len < 0) || (file_len > 3600))
//				{
//					fprintf(stderr, "%s: Invalid file length setting %s\r\n", MODULE_NAME, optarg);
//					return(BCERROR);
//				}
//				printf("File length = %d\r\n",file_len);
//				break;
			case 'p':		// 'p' option specifies serial port to use
				free(zephyr.port);
				zephyr.port = malloc(strlen(optarg)+1);
				strcpy(zephyr.port, optarg);
				if(zephyr.port == NULL)
				{
					fprintf(stderr, "%s: Error setting serial port to %s\r\n",MODULE_NAME,optarg);
					return(BCERROR);
				}
				printf("Port = %s\r\n",zephyr.port);
				break;
			case 's':		// 's' option specifies serial port speed
				zephyr.speed=get_baud(atoi(optarg));
				zephyr.speed=get_baud(atoi(optarg));
				if(zephyr.speed == BCERROR)
				{
					fprintf(stderr, "%s: Invalid bits setting %s\r\n", MODULE_NAME, optarg);
					return(BCERROR);
				}
				printf("Speed = %d\r\n",zephyr.speed);
				break;
			case 'b':		// 'b' option specifies serial port bit settings
				if(verify_bits(optarg))
				{
					zephyr.bits = optarg[0]-'0';
					zephyr.parity = optarg[1];
					zephyr.stop = optarg[2]-'0';
					zephyr.bits = optarg[0]-'0';
					zephyr.parity = optarg[1];
					zephyr.stop = optarg[2]-'0';
				}
				else
				{
					fprintf(stderr, "%s: Invalid bits setting %s\r\n", MODULE_NAME, optarg);
					return(BCERROR);
				}
				printf("Bits = %d, Par = %c, Stop = %d\r\n",zephyr.bits, zephyr.parity,zephyr.stop);
				break;
			case 'f':		// 'f' option specifies serial port flow control
				if(verify_flow(optarg))
				{
					zephyr.flow = optarg[0];
					zephyr.flow = optarg[0];
				}
				else
				{
					fprintf(stderr, "%s: Invalid flow control setting %s\r\n", MODULE_NAME, optarg);
					return(BCERROR);
				}
				printf("Flow = %c\r\n",zephyr.flow);
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

	printf("Usage:\r\n zephyr [OPTIONS]\r\n\r\n");
	printf("Options:\r\n -b <bit settings>\tUse <bit settings> for the serial port (default = %s).\r\n",BITS_DEFAULT);
	printf(" -f [N | H | S]\tSpecifies what type of flow control to use (N=None, H=Hardware, S=Software) (default=%c).\r\n",FLOW_DEFAULT);
	printf(" -s <speed>\tConfigure the serial ports for <speed> bits per second (default = %d).\r\n",BAUD_DEFAULT);
//	printf(" -l <length>\tCreate data files of length <length> seconds (default = %d).\r\n",FILE_LEN_DEFAULT);
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

	setting = config_lookup(cfg, "zeph_port");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(str != NULL)
		{
			zephyr.port = malloc(strlen(str)+1);
			strcpy(zephyr.port, str);
			if(verbose) printf("%s: GPS commands on %s.\r\n",MODULE_NAME, zephyr.port);
		}
		else
		{
			fprintf(stderr, "%s: Invalid device name %s in config file.\r\n",MODULE_NAME, str);
			zephyr.port = NULL;
		}
	}
	else
	{
		zephyr.port = NULL;
	}


	setting = config_lookup(cfg, "zeph_speed");
	if(setting != NULL)
	{
		zephyr.speed = get_baud(config_setting_get_int(setting));
		if(zephyr.speed == BCERROR)
		{
			fprintf(stderr, "%s: Invalid baud rate %d in config file %s. Using default vaule %d.\r\n",MODULE_NAME, zephyr.speed, path, BAUD_DEFAULT);
			zephyr.speed = BAUD_DEFAULT;
		}
		else
		{
			if(verbose) printf("%s: Using baud rate %d for serial port %s from config file %s.\r\n",MODULE_NAME, zephyr.speed, zephyr.port, path);
		}
	} 
	else
	{
		zephyr.speed = BAUD_DEFAULT;
	}

	setting = config_lookup(cfg, "bits");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(verify_bits(str))
		{
			zephyr.bits = str[0]-'0';
			zephyr.parity = str[1];
			zephyr.stop = str[2]-'0';
			if(verbose) printf("%s: Using serial bit settings %s.\r\n",MODULE_NAME, str);
		}
		else
		{
			fprintf(stderr, "%s: Invalid bits setting %s in config file. Using default value %s\r\n", MODULE_NAME, str, BITS_DEFAULT);
			zephyr.bits = 8;
			zephyr.parity = 'N';
			zephyr.stop = 1;
		}
	}
	else
	{
			zephyr.bits = 8;
			zephyr.parity = 'N';
			zephyr.stop = 1;
	}

	setting = config_lookup(cfg, "flow");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(verify_flow(str))
		{
			zephyr.flow = str[0];
			if(verbose) printf("%s: Using %c flow control.\r\n",MODULE_NAME, zephyr.flow);
		}
		else
		{
			fprintf(stderr, "%s: Invalid flow control setting %c in config file. Using default value %s\r\n", MODULE_NAME, str[0], FLOW_DEFAULT);
			zephyr.flow = FLOW_DEFAULT;
		}
	}
	else
	{
			zephyr.flow = FLOW_DEFAULT;
	}

	setting = config_lookup(cfg, "terminator");
	if(setting != NULL)
	{
		str = config_setting_get_string(setting);
		if(str != NULL)
		{
			strcpy(zephyr.term, str);
		}
		else
		{
			fprintf(stderr, "%s: Invalid line terminator specified. Using default value\r\n", MODULE_NAME);
			strcpy(zephyr.term, TERM_DEFAULT);
		}
	}
	else
	{
		strcpy(zephyr.term, TERM_DEFAULT);
	}

	setting = config_lookup(cfg, "lp_mode");

	setting = config_lookup(cfg, "hwversion");
	if(setting != NULL)
	{
		hwversion = config_setting_get_int(setting);
		printf("%s: Interface board version %d.\r\n",MODULE_NAME, hwversion);
	} 
	else
	{
		hwversion = HWVERSION_DEFAULT;
		printf("%s: No interface board version number found. Using default %d.\r\n",MODULE_NAME, hwversion);
	}
	
	config_destroy(cfg);
	return(BCSUCCESS);

}



int SerialPortInit(serial port)
{
	struct termios options;

	if(verbose) printf("%s: Opening port %s\r\n", MODULE_NAME, port.port);
	port.fp = open(port.port, O_RDWR | O_NOCTTY | O_NDELAY);	// Open serial port
	if(port.fp == BCERROR)
		return(BCSEROPENERROR);

	tcgetattr(port.fp, &options);						// Get current port options

	cfsetispeed(&options, port.speed);				// Set input baud rate
	cfsetospeed(&options, port.speed);				// Set output baud rate

	options.c_cflag &= ~CSIZE; 							// Clear data bit fields
	switch(port.bits)
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

	switch(port.parity)
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

	switch(port.stop)
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

	switch(port.flow)
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

	if(tcsetattr(port.fp, TCSANOW, &options) == 0)	// Make the new settings active
	{
//		sleep(2);
		tcflush(port.fp,TCIOFLUSH);
		return(BCSUCCESS);
	}
	else
		return(BCSETATTRERROR);
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
	char *rootname, *fullpath;

	epoch = time(NULL);
//	epoch = ts.tv_sec;
	now = gmtime(&epoch);
	sprintf(todaysDirectoryName,"%04d%02d%02d", (now->tm_year)+1900, (now->tm_mon)+1, now->tm_mday);
	return(todaysDirectoryName);
}


char *TodaysDirectoryPath(void)
{
	int len;
	char *dirname, *fullpath;

	dirname = TodaysDirectoryName();

	fullpath = &todaysDirectoryPath[0];
	len = strlen(data_dir);
	strcpy(fullpath, data_dir);
	strcpy(fullpath + len, "/");
	strcpy(fullpath + len + 1, dirname);
	strcpy(fullpath + len + 1 + strlen(dirname), "/");
	fullpath[len + strlen(dirname) + 2] = 0;	// Null term
	return(fullpath);
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

	char *filename = malloc(14 + 1 + strlen(name) + 4 + 1);	// format is YYYYMMDDhhmmss_name.txt
	if(filename == NULL)
	{
		return(NULL);
	}

	epoch = time(NULL);
//	epoch = ts.tv_sec;
	now = gmtime(&epoch);
	sprintf(filename,"%s_%04d%02d%02d%02d%02d%02d.sbf", name, (now->tm_year)+1900, (now->tm_mon)+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

	if((rootname = TodaysDirectoryPath()) == NULL)
	{
		return(NULL);
	}

	fullpath = &logFileName[0];
	strcpy(fullpath, rootname);
//	strcpy(fullpath + strlen(rootname), "/");
	strcpy(fullpath + strlen(rootname), filename);
	fullpath[strlen(rootname) + strlen(filename)] = 0;

	free (filename);

	return(fullpath);
}


int OpenLogFile(char *name)
{
	int i, index, ret;
	char *strName;

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
		if(verbose) printf("%s: Closed log files.\r\n", MODULE_NAME);
	}

	return(ret);
}


int NewFile(void)
{
	CloseLogFile();
	OpenLogFile(MODULE_NAME);

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


char *GetNextFile(void)
{
	DIR *dir;
	struct dirent *entry, *lastentry;
	int found;
	char *fullPath;
	
	found = FALSE;
	dir = opendir(queueDir);
	if(dir != NULL)
	{
		while((!found) && (entry = readdir(dir)))
		{
			if(strcmp(entry->d_name,"list.txt")==0)
				{
				found = TRUE;
				}
		}
		closedir(dir);
		if (!found) {
			dir = opendir(queueDir);
			while((!found) && (entry = readdir(dir)))
			{
				if(entry->d_name[0] != '.')
					{
					found = TRUE;
					}
			}
			closedir(dir);
		}
		if(found)
		{
			struct stat st;

			fullPath= &getNextFile[0];
			strcpy(fullPath,queueDir);
			strcpy(fullPath+strlen(queueDir), "/");
			strcpy(fullPath+strlen(queueDir)+1, entry->d_name);
			stat(fullPath, &st);
			if (strstr(fullPath,"list.txt")==0) {
				if (!discardFirstFile) {
					if (!lastPart) gotGPSChars += st.st_size;
					if (verbose) printf("%s: File Size = %d, Discard=%d\n",	MODULE_NAME, st.st_size, discardFirstFile);
				}
				discardFirstFile = 0;
			}
			return(fullPath);
		}
		else
		{
			return(NULL);
		}
	}
}



int SendFile(char *name)
{
	char *buf;
	struct stat st;
	int err, n;
	FILE *ptr;

	err=stat(name, &st);
	if(verbose) printf("%s: Opening file %s\r\n", MODULE_NAME, name);
	ptr = fopen(name, "r");
	if(ptr==NULL)
	{
		fprintf(stderr,"%s: Error getting file size\r\n", MODULE_NAME);
		return(BCFILEERROR);
	}

	if(numParts>lastPart)						// In the middle of a file transfer
	{
		if(verbose) printf("%s: Continuing offload of %s %d/%d\r\n", MODULE_NAME, name, lastPart+1, numParts);
		buf=malloc(MAX_DATA);
		if(buf==NULL)
		{
			fprintf(stderr,"%s: Error allocating buffer for file read\r\n", MODULE_NAME);
			return(BCFILEERROR);
		}
		fseek(ptr, MAX_DATA*lastPart, SEEK_SET);	// move to proper place in the data file
		n = fread(buf, 1, MAX_DATA, ptr);
		dataTm = 1;
		SendMsg(TM, buf, n);
	}
	else										// Starting new transfer
	{
		numParts = (st.st_size / MAX_DATA) + 1;		// calculate how many parts the file needs to be broken into
		if (verbose) printf("%s: size = %d parts = %d\r\n",MODULE_NAME,st.st_size,numParts);
		if(numParts>1)								// multipart file
		{
			if(verbose) printf("%s: Starting offload of %s 1/%d\r\n", MODULE_NAME, name, numParts);
			buf=malloc(MAX_DATA);
			if(buf==NULL)
			{
				fprintf(stderr,"%s: Error allocating buffer for file read\r\n", MODULE_NAME);
				return(BCFILEERROR);
			}
			n = fread(buf, 1, MAX_DATA, ptr);
			dataTm = 1;
			SendMsg(TM, buf, n);
		}
		else
		{											// single part file
			if(verbose) printf("%s:  Offloading %s 1/1\r\n", MODULE_NAME, name);
			buf=malloc(st.st_size);
			if(buf==NULL)
			{
				fprintf(stderr,"%s: Error allocating buffer for file read\r\n", MODULE_NAME);
				return(BCFILEERROR);
			}
			n = fread(buf, 1, st.st_size, ptr);
			dataTm = 1;
			SendMsg(TM, buf, st.st_size);
//			write(zephyr.fp, buf, n);
		}
	}
	waitingAck=WAITFORACK;
//	ackTimeout=time(NULL) + 5;
	fclose(ptr);
	if (buf) free(buf);
}

int TCSendFile(char *name)
{
	char *buf;
	struct stat st;
	int err, n;
	FILE *ptr;

	err=stat(name, &st);
	if(verbose) printf("%s: Opening file %s\r\n", MODULE_NAME, name);
	ptr = fopen(name, "r");
	if(ptr==NULL)
	{
		fprintf(stderr,"%s: Error getting file size\r\n", MODULE_NAME);
		return(BCFILEERROR);
	}

	if(TCnumParts>TClastPart)						// In the middle of a file transfer
	{
		if(verbose) printf("%s: Continuing offload of %s %d/%d\r\n", MODULE_NAME, name, TClastPart+1, TCnumParts);
		buf=malloc(MAX_DATA);
		if(buf==NULL)
		{
			fprintf(stderr,"%s: Error allocating buffer for file read\r\n", MODULE_NAME);
			return(BCFILEERROR);
		}
		fseek(ptr, MAX_DATA*TClastPart, SEEK_SET);	// move to proper place in the data file
		n = fread(buf, 1, MAX_DATA, ptr);
		dataTm = 1;
		SendMsg(TM, buf, n);
	}
	else										// Starting new transfer
	{
		TCnumParts = (st.st_size / MAX_DATA) + 1;		// calculate how many parts the file needs to be broken into
		if (verbose) printf("%s: size = %d parts = %d\r\n",MODULE_NAME,st.st_size,TCnumParts);
		if(TCnumParts>1)								// multipart file
		{
			if(verbose) printf("%s: Starting offload of %s 1/%d\r\n", MODULE_NAME, name, TCnumParts);
			buf=malloc(MAX_DATA);
			if(buf==NULL)
			{
				fprintf(stderr,"%s: Error allocating buffer for file read\r\n", MODULE_NAME);
				return(BCFILEERROR);
			}
			n = fread(buf, 1, MAX_DATA, ptr);
			dataTm = 1;
			SendMsg(TM, buf, n);
		}
		else
		{											// single part file
			if(verbose) printf("%s:  Offloading %s 1/1\r\n", MODULE_NAME, name);
			buf=malloc(st.st_size);
			if(buf==NULL)
			{
				fprintf(stderr,"%s: Error allocating buffer for file read\r\n", MODULE_NAME);
				return(BCFILEERROR);
			}
			n = fread(buf, 1, st.st_size, ptr);
			dataTm = 1;
			SendMsg(TM, buf, st.st_size);
//			write(zephyr.fp, buf, n);
		}
	}
	waitingAck=WAITFORACK;
//	ackTimeout=time(NULL) + 5;
	fclose(ptr);
	if (buf) free(buf);
}


int RemoveFile(char *name)
{
	if (strstr(name,"list.txt")) {	
		TClastPart=0;
		TCnumParts=0;
	} else {
		lastPart=0;
		numParts=0;
	}
	return(remove(name));
}


void SendMsg(enum msg msgtype, char *payload, int payload_size)
{
	unsigned short cs;
	int ptr;
	unsigned char byte;
	struct timespec curTime;
	char hdr_msg[128];
	size_t	wrote;
	
	clock_gettime(CLOCK_REALTIME, &curTime);
	while((curTime.tv_sec < nextMsgTime.tv_sec) || ((curTime.tv_sec == nextMsgTime.tv_sec) && (curTime.tv_nsec < nextMsgTime.tv_nsec)))	// Wait until it's ok to send next msg
	{
		clock_gettime(CLOCK_REALTIME, &curTime);
		usleep(50000);		// sleep for 50ms
	}
	if(verbose) printf("%s: Sending message at time %ld.%03ld\r\n", MODULE_NAME, curTime.tv_sec, curTime.tv_nsec/1000000);
	switch(msgtype)
	{
		case IMR:			///< Tells Zephyr we're done booting and ready for mode change
			sprintf(outBuf,"<IMR>\n\t<Msg>%d</Msg>\n\t<Inst>%s</Inst>\n\t<SWDate>%s</SWDate>\n\t<SWVersion>%s</SWVersion>\n\t<ZProtocolVersion>%s</ZProtocolVersion>\n</IMR>\n",MsgID,INSTID,SWDate,SWVersion,ZephVersion);
			crc_calc=ComputeCRC(outBuf,strlen(outBuf));
			sprintf(outBuf+strlen(outBuf),"<CRC>%d</CRC>\n",crc_calc);
			if (verbose) printf(outBuf);
			write(zephyr.fp, outBuf, strlen(outBuf));
			break;
		case IMAck:			///< Tells Zephyr we got the mode change command
			sprintf(outBuf,"<IMAck>\n\t<Msg>%d</Msg>\n\t<Inst>%s</Inst>\n\t<Ack>%s</Ack>\n</IMAck>\n",MsgID,INSTID,"ACK");		///< always sending ACK for now, change later
			crc_calc=ComputeCRC(outBuf,strlen(outBuf));
			sprintf(outBuf+strlen(outBuf),"<CRC>%d</CRC>\n",crc_calc);
			if(verbose) printf(outBuf);
			write(zephyr.fp, outBuf, strlen(outBuf));
			break;
		case S:				///< Tells Zephyr we're in Safety mode now, ok to shut off power
			sprintf(outBuf,"<S>\n\t<Msg>%d</Msg>\n\t<Inst>%s</Inst>\n</S>\n",MsgID,INSTID);		
			crc_calc=ComputeCRC(outBuf,strlen(outBuf));
			sprintf(outBuf+strlen(outBuf),"<CRC>%d</CRC>\n",crc_calc);
			if(verbose) printf(outBuf);
			write(zephyr.fp, outBuf, strlen(outBuf));
			break;
		case TM:			///< Telemetry message. This is how we send data to Zephyr
			if (TCflag) {
				sprintf(hdr_msg, "%s,%d,%d",TCfileOffload,TClastPart,TCnumParts);
			}
			else if(dataTm)		// This message contains a data payload
			{
				sprintf(hdr_msg, "%s,%d,%d",fileOffload,lastPart,numParts);
			}
			else			// This is just a housekeeping message, no data payload
			{
				sprintf(hdr_msg,"000");		// This should eventually contain # satellites tracked
			}
			sprintf(outBuf,"<TM>\n\t<Msg>%d</Msg>\n\t<Inst>%s</Inst>\n\t<StateFlag1>FINE</StateFlag1>\n\t<StateMess1>%s</StateMess1>\n\t<Length>%d</Length>\n</TM>\n",MsgID,INSTID,hdr_msg,payload_size);
			crc_calc=ComputeCRC(outBuf,strlen(outBuf));
			sprintf(outBuf+strlen(outBuf),"<CRC>%d</CRC>\n",crc_calc);
			sprintf(outBuf+strlen(outBuf),"START");
			if(verbose) printf("%s:Payload %d bytes:", outBuf, payload_size);
			ptr=strlen(outBuf);
			memcpy(outBuf+ptr,payload,payload_size);
			ptr+=payload_size;
			cs = ComputeCRC(payload,payload_size);
			byte = ((cs & 0xFF00) >> 8) & 0xFF;
			if(verbose) printf("%02X", byte);
			memcpy(outBuf+ptr, &byte, 1);
			ptr++;
			byte = cs & 0xFF;
			if(verbose) printf("%02X", byte);
			memcpy(outBuf+ptr, &byte, 1);
			ptr++;
			sprintf(outBuf+ptr,"END");
			ptr+=3;
			if(verbose) printf(":END\n");
			wrote=0;
//			printf("data to write = %d\r\n", ptr);
			while(ptr > 0)
			{
				wrote = write(zephyr.fp, outBuf+wrote, ptr);
				ptr -= wrote;
//				printf("wrote = %d  ptr = %d\r\n", wrote, ptr);
//				usleep(300000);		// sleep for 300ms
			}
			break;
		case TCAck:			///< Tells Zephyr we got the telecommand message (uplink data)
			sprintf(outBuf,"<TCAck>\n\t<Msg>%d</Msg>\n\t<Inst>%s</Inst>\n\t<Ack>%s</Ack>\n</TCAck>\n",MsgID,INSTID,"ACK");		///< always sending ACK for now, change later
			crc_calc=ComputeCRC(outBuf,strlen(outBuf));
			sprintf(outBuf+strlen(outBuf),"<CRC>%d</CRC>\n",crc_calc);
			if(verbose) printf(outBuf);
			write(zephyr.fp, outBuf, strlen(outBuf));
			break;
		default:
			break;
	}
	
	MsgID++;
//	nextMsgTime = curTime;	// set earliest time for sending
	clock_gettime(CLOCK_REALTIME, &nextMsgTime);	// set earliest time for sending
//	if (msgtype != TM) {
//		nextMsgTime.tv_nsec += 300000000;					// next message (300 msec from now)
//		if (nextMsgTime.tv_nsec > 1000000000) {
//			nextMsgTime.tv_nsec -= 1000000000;
//			nextMsgTime.tv_sec += 1;
//		}
//	} else	nextMsgTime.tv_sec += 1;						// next message (1 sec from now)
	nextMsgTime.tv_sec += 1;						// next message (1 sec from now)
}



int CRCComp(char* data, int len)
{
	if(ComputeCRC(data, len)==crc_msg) return(1);
	else return(0);
}


unsigned short CRC(unsigned short last, char next)
{
	unsigned short x, y;

	x = ((last>>8) ^ next) & 0xff;
	x ^= x>>4;

	y = (last << 8) ^ (x << 12) ^ (x <<5) ^ x;

	y &= 0xffff;
	return(y);
}


unsigned short ComputeCRC(char *data, int len)
{
	int i;
	unsigned short result;

	result = 0x1021;
	for(i=0;i<len;i++)
	{
		result=CRC(result, data[i]);
	}
	return(result);
}


enum msg MsgType(char *typestr)
{
	if(strncmp(typestr, "IM", 2)==0)	///< Got an IM message
	{
		return(IM);
	}
	else if(strncmp(typestr, "SAck", 4)==0)	///< Got an SAck message
	{
		return(SAck);
	}
	else if(strncmp(typestr, "SW", 2)==0)	///< Got an SW message
	{
		return(SW);
	}
	else if(strncmp(typestr, "TMAck", 5)==0)	///< Got a TMAck message
	{
		return(TMAck);
	}
	else if(strncmp(typestr, "TC", 2)==0)	///< Got a TC message
	{
		return(TC);
	}
	else if(strncmp(typestr, "GPS", 3)==0)	///< Got a GPS message
	{
		return(GPS);
	}
	else
	{
		return(Error);
	}
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
		sprintf(cmdstr,"%s gps_power 0", GPIO_PATH);
	}
	else
	{
		sprintf(cmdstr,"%s gps_power 1", GPIO_PATH);
	}

	system(cmdstr);
}


void gps_reset(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"%s gps_reset 0", GPIO_PATH);
	}
	else
	{
		sprintf(cmdstr,"%s gps_reset 1", GPIO_PATH);
	}

	system(cmdstr);
}


void gps_power_toggle(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"%s gps_pow_tog 0", GPIO_PATH);
	}
	else
	{
		sprintf(cmdstr,"%s gps_pow_tog 1", GPIO_PATH);
	}

	system(cmdstr);
}


void set_safe(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"%s safe 1", GPIO_PATH);
	}
	else
	{
		sprintf(cmdstr,"%s safe 0", GPIO_PATH);
	}

	system(cmdstr);
}


void led(int state)
{
	char cmdstr[32];
	
	switch(state)
	{
		case RED:
			sprintf(cmdstr,"%s led1 0", GPIO_PATH);
			system(cmdstr);
			sprintf(cmdstr,"%s led0 1", GPIO_PATH);
			system(cmdstr);
			break;
		case GREEN:
			sprintf(cmdstr,"%s led0 0", GPIO_PATH);
			system(cmdstr);
			sprintf(cmdstr,"%s led1 1", GPIO_PATH);
			system(cmdstr);
			break;
		case ORANGE:
			sprintf(cmdstr,"%s led0 1", GPIO_PATH);
			system(cmdstr);
			sprintf(cmdstr,"%s led1 1", GPIO_PATH);
			system(cmdstr);
			break;
		case OFF:
			sprintf(cmdstr,"%s led0 0", GPIO_PATH);
			system(cmdstr);
			sprintf(cmdstr,"%s led1 0", GPIO_PATH);
			system(cmdstr);
			break;
	}		
}


void set_gpio0(int state)
{
	char cmdstr[32];
	
	if(state)
	{
		sprintf(cmdstr,"%s gpio0 1", GPIO_PATH);
	}
	else
	{
		sprintf(cmdstr,"%s gpio0 0", GPIO_PATH);
	}

	system(cmdstr);
}

