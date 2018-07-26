25-Jul-2018
-----------

Fixed:

- Fixed bug where the GPS receiver would not shut off when switching to low power
  mode. This bug only occurred when the software was started automatically at boot
  up. Cause of the bug was the system call to the "gpio" script that now handles
  digital I/O. That system call wasn't being specified as a full path to the 
  executable. That's why it worked when logged in but not when started without a
  terminal attached to it.

Changes:
- Changed the build date and time string to "20180725,2231" for both roc and zephyr
  modules.
- Changed the software version string to "027" for both roc and zephyr modules.

Bugs:
- The initialization commands need to be sent to the GPS receiver after turning
  power back on (when switching from LP mode to FL mode). Currently no way to do
  that until some form of IPC is implemented.


22-Jul-2018
-----------

Fixed:

- Minimum delay between messages sent from ROC was increased from 300ms to 1s
  if the zephyr module. The 300ms limit was sometimes allowing messages to be
  sent to the Zephyr instrument too quickly causing message acknowledgement
  errors.

Changes:
- Changed the build date and time string to "20180722,2225" for both roc and zephyr
  modules.
- Changed the software version string to "026" for both roc and zephyr modules.

Bugs:
- The initialization commands need to be sent to the GPS receiver after turning
  power back on (when switching from LP mode to FL mode). Currently no way to do
  that until some form of IPC is implemented.
- Bing has reported some problems with the latest code:
   - switching to LP mode doesn't shut down the GPS receiver (NOTE: UNABLE TO
     REPRODUCE THIS BUG).
   - Some sort of bug related to sending a telecommand to ROC? Need clarification
     on this one. See Bing's log entry "2018-06-29_tofix.txt"


17-Jul-2018
-----------

This folder contains no software changes, it is just a snapshot of the latest
version to be used as the basis for further updates.

Fixed:

Changes:
- Changed the build date and time string to "20180717,2327" for both roc and zephyr
  modules.
- Changed the software version string to "025" for both roc and zephyr modules.

Bugs:
- The initialization commands need to be sent to the GPS receiver after turning
  power back on (when switching from LP mode to FL mode). Currently no way to do
  that until some form of IPC is implemented.
- Bing has reported some problems with the latest code:
   - Sometimes switching to SAFE mode is not acknowledged.
   - switching to LP mode doesn't shut down the GPS receiver.
   - Some sort of bug related to sending a telecommand to ROC? Need clarification
     on this one. See Bing's log entry "2018-06-29_tofix.txt"


12-Jun-2018
-----------

Fixed:

Changes:
- Created 'roc_gpio' startup script in /etc/init.d. This script configures GPIO
  pins correctly.
- Created script 'gpio' in /opt/bin. This script is called from the roc and
  zephyr modules to set the digital outputs rather than doing it manually in those
  modules. See source code for that script for details.
- Disabled script 'roc_led' in /etc/init.d since that functionality has been moved
  to script 'gpio'.
- Changed the build date and time string to "20180612,2312" for both roc and zephyr
  modules.
- Changed the software version string to "024" for both roc and zephyr modules.
- zephyr: Added 'hwversion' item to zephyr.conf file. The latest version of the
  interface board, completed June 2018, is version 3. Version 2 is the only prior
  version in use.
- zephyr: Reads hwversion from conf file and uses this to determine how to handle
  low power mode. HW version 3 allows software control of the power to the GPS
  receiver. GPS power is on at startup and only turned off when entering LP mode.
  Power turned back on when switching from LP mode to FL mode.
- zephyr: turn external LED to green if startup is successful, turn it red if not.
- roc: Added some of the code to eventually take over control of the external LED.
  Not active yet.

Bugs:
- The initialization commands need to be sent to the GPS receiver after turning
  power back on (when switching from LP mode to FL mode). Currently no way to do
  that until some form of IPC is implemented.


08-Mar-2018
-----------

Fixed:
- Fixed problem of data files growing forever and not being copied to queue.
- Fixed problem of data offload process stopping after a while when it boots normally.
  
Changes:
- Changed the build date and time string to "20180308,2301"
- Changed the software version string to "022"

Bugs:
- Sometimes the name of the file to offload becomes invalid. Either NULL or %. Code
  can't offload that file and doesn't know what to do about it, it just keeps trying.
- System crashed after running for about 10 minutes offloading data. Crash happened
  right when it received a TMAck and GPS message back to back, not sure if that was
  the cause.


07-Mar-2018
-----------

Fixed:
- ROC was sending IMR request repeatedly at startup even after it got an IM message
  putting it into SB mode. I assume it's supposed to stop sending IMR after it gets
  any IM message.
- Wasn't sending periodic housekeeping TM messages in LP mode. Now it does.
  
Changes:
- Added 'clear' telecommand that clears the data queue
- Changed the build date and time string to "20180307,1819"
- Changed the software version string to "021"

Bugs:
- Sometimes the name of the file to offload becomes invalid. Either NULL or %. Code
  can't offload that file and doesn't know what to do about it, it just keeps trying.
- System crashed after running for about 10 minutes offloading data. Crash happened
  right when it received a TMAck and GPS message back to back, not sure if that was
  the cause.
- Saw a behavior that Jen had noticed where the data files grew very large and were
  no longer being copied into the queue.
- System stops offloading data after some number of minutes when it boots on its
  own (doesn't do it when run from the cmd line)
  

06-Mar-2018
-----------

Fixed:
- REALLY think I fixed the unreliable comms now. Had to increase gap between ROC
  messages from 1 second to 2 seconds. Seems to work well now so far.
- Fixed a bug that caused ROC to miss the 2nd message if two messages arrived
  simultaneously.
  
Changes:
- Re-added reboot command via TC
- Increased max size of TM payload to 3800 bytes. The maximum amount of data that
  linux will send in a single write is ~4100 bytes. If our packets are bigger than
  that we need to transmit them using multiple write commands. But when I do this
  the Zephyr simulator crashes. I'm probably not splitting up the buffer properly
  when I do it in multiple parts.
- Changed the build date and time string to "20180306,1859"
- Changed the software version string to "020"

Bugs:
- Sometimes the name of the file to offload becomes invalid. Either NULL or %. Code
  can't offload that file and doesn't know what to do about it, it just keeps trying.
- System crashed after running for about 10 minutes offloading data. Crash happened
  right when it received a TMAck and GPS message back to back, not sure if that was
  the cause.

  
01-Mar-2018
-----------

Fixed:
- Probably fix for the 5 second delay between sending of TM messages during
  data transfer. This should increase our throughput dramatically. Need to test
  though.

Changes:
- Changed the contents of the TM packet header. If the packet contains a data payload
  (we're transmitting a data file) the StateMess1 field of the header now contains
  the name of the file being transferred, the current part number, and the total number
  of parts. The format is name,part,totalparts. This information is needed to 
  recombine the individual parts back into the original files on the receiving end.
- Removed some debug messages added previously.
- Changed the build date and time string to "20180301,2316"
- Changed the software version string to "019"

Bugs:
- Sometimes the name of the file to offload becomes invalid. Either NULL or %. Code
  can't offload that file and doesn't know what to do about it, it just keeps trying.
- System crashed after running for about 10 minutes offloading data. Crash happened
  right when it received a TMAck and GPS message back to back, not sure if that was
  the cause.

  
  
27-Feb-2018
-----------

Fixed:
- Fixed bug in the setting of the SAFE output signal
- Definitely fixed the back-to-back msgs
- Fixed a bug causing problems when Zephyr acknowledged the receipt of a housekeeping
  TM message. 

Changes:
- No longer try to reset GPS receiver when coming out of LP mode since it doesn't work
- Changed the only TC command that we listen to. If the command contains "ls" in it, the
  system will do "ls -l" on the download queue and put the results into a file called
  "~list.txt" and place that file into the queue directory. The ~ in the file name should
  cause it to be the next file offloaded.
- Data files after the 1st file are started/stopped on even boundaries of the file length
- Changed the build date and time string to "20180227,2222"
- Changed the software version string to "018"

Bugs:
- Sometimes the name of the file to offload becomes invalid. Either NULL or %. Code
  can't offload that file and doesn't know what to do about it, it just keeps trying.
- System crashed after running for about 10 minutes offloading data. Crash happened
  right when it received a TMAck and GPS message back to back, not sure if that was
  the cause.
  

26-Feb-2018
-----------

Fixed:
- Maybe fixed the timing so no more-back-to-back msgs?

Changes:
- Removed .conf files from /root/roc
- SWVersion and SWDate printed at startup.
- ROC now sends an "S" msg every time Zephyrs sends us IM-S message as required.
- Changed the build date and time string to "20180226,1159"
- Changed the software version string to "017"


20-Feb-2018
-----------

Fixed:
  
Changes:
- No longer shuts down when it receives the command to change to EF mode. It just
  does nothing now. The program remains running so that it can listen for further
  mode change commands.
- SAFE digital output now set to HI when in safe mode.
- Housekeeping TM messages sent once/min when in SB mode.
- Changed the build date and time string to "20180220,2127"
- Changed the software version string to "016"


14-Feb-2018
-----------

Fixed:
- Fixed bug causing ROC to enter Safe mode at startup.
- Got rid of compiler warnings.
- Fixed crash caused by sending TC message to ROC.
- Fixed GPS receiver sleep when entering LP mode.

Changes:
- Can no longer send messages faster than once/second. Currently an inefficient
  solution.
- Now correctly sends TCAck message in response to receiving TC message.
- A TC message that contains the string "reboot" (without the quotes) anywhere in
  the payload will initiate a reboot.
- Changed the build date and time string to "20180214,0016"
- Changed the software version string to "015"


13-Feb-2018
-----------

Fixed:
- Fixed bug in how GPS receiver was being put into standby mode. Had the
  wrong serial port specified.
  
Changes:
- No longer starts sending IMR messages when entering SB mode after being
  in another mode. Only sends IMR messages at startup.
- Switching to EF mode now causes system to shutdown.
- Changed the build date and time string to "20180213,0001"
- Changed the software version string to "014"


12-Feb-2018
-----------

Changes:
- Sends command to put GPS receiver into StandBy mode when Zephyr tells
  ROC to enter Low Power mode.
- Toggled nRESET pin on GPS receiver (pin #10) when switching from Low
  Power mode to Flight Mode.
- Changed the build date and time string to "20180212,0001"
- Changed the software version string to "013"


11-Feb-2018
-----------

Changes:
- Now responds correctly to End-of-FLight mode change. Stops all comms
  with Zephyr. GPS data collection continues.
- Should now correctly enter Safety mode if no comms have been received
  from Zephyr in over 2 hours. Does not toggle SAFE digital output yet
  though when entering Safety mode, either by 2 hour timeout or by IM
  change request.
- Changed the build date and time string to "20180211,2324"
- Changed the software version string to "012"

