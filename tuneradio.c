/* 
 *  
 *  TUNERADIO
 *  
 *  Opens the the tuner device (ie. /dev/tuner) sets the frequency,
 *  stereo/mono and AFC modes.  It then sits in a while loop to
 *  keep the device open.
 *
 *  I needed this program to run on remote computers that stream 
 *  MP3 streams out to the net of radio stations.  This program 
 *  assumes the Brooktree 848 card and interface for *BSD.
 *
 *  Thanks to Jason Andresen for the patches to compile this on
 *  FreeBSD 5.1 and cleaning up my code. :-)
 *
 *  Copyright (C) 2000-2003 Timothy Pozar pozar@lns.com
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *  
 *  $Id: tuneradio.c,v 1.3 2003/10/31 05:53:37 pozar Exp $
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <machine/ioctl_bt848.h>

#define TRUE 1
#define FALSE 0
#define BUF_SIZE 4096

char device[BUF_SIZE] = "/dev/tuner";
int mono = FALSE;
int afc = FALSE;
int frequency = 8850;	/* 8850 = 88.5Mhz */
int devfh;
int setaudio;
static void cleanup (int);
static void cleanvoid (void);
static void cntlc_handler (int);  
void banner(void);

int main(argc,argv)
int argc;
char *argv[];
{
int gotmask, setmask, i;

   /* scan command line arguments, and look for files to work on. */
   for (i = 1; i < argc; i++) {
      switch (argv[i][1]){   /* be case indepenent... */
         case 'A':   /* Run with AFC... */
         case 'a':
            afc = TRUE;
            break;  
         case 'D':   /* dsp file name... */
         case 'd':
            i++;
            strncpy(device,argv[i],BUF_SIZE);
            break;
         case 'F':   /* Frequency in 10 KHz... */
         case 'f':
            i++;
            frequency = atoi(argv[i]);
            if((frequency < 8800) || (frequency > 10800)){
               printf("Frequency %i is out of range of 8800 to 10800\n",frequency);
               exit(1);
            }
            break;
         case 'H':   /* Help... */
         case 'h':
            banner();
            exit(0);
         case 'M':   /* Run in mono... */
         case 'm':
            mono = TRUE;
            break;  
         default:
            printf("I don't know the meaning of the command line argument: \"%s\".\n",argv[i]);
            banner();
            exit(1);
      }
   }

   if (atexit (cleanvoid) == -1)
      perror ("atexit");

   if (signal (SIGINT, cntlc_handler) == SIG_ERR) {
      perror ("Signal INT...");
      exit (1);
   }
   if (signal (SIGHUP, cntlc_handler) == SIG_ERR) {
      perror ("Signal HUP...");
      exit (1);
   }   

   if((devfh = open(device, O_RDONLY)) == -1){
      perror("opening dsp device");
      exit(1);
   }

   setaudio = AUDIO_INTERN; 
   if(ioctl(devfh, BT848_SAUDIO, &setaudio) == -1){
      perror("set internal audio ");
      exit(1);
   }

   if(ioctl(devfh, RADIO_GETMODE, &gotmask) == -1){
      perror("get mode");
      exit(1);
   }
   setmask = gotmask;

   if(mono)
      setmask |= RADIO_MONO;    
   else
      setmask &= ~RADIO_MONO;

   if(afc)
      setmask |= RADIO_AFC;    
   else
      setmask &= ~RADIO_AFC;

   if(ioctl(devfh, RADIO_SETMODE, &setmask) == -1){
      perror("set mode - mono/afc");
      exit(1);
   }

   if(ioctl(devfh, RADIO_SETFREQ, &frequency) == -1){
      perror("set frequency");
      exit(1);
   }

   while(1){
      sleep(1);
   }

   exit(0);
}

void banner()
{
   printf("tuneradio: Set \"%s\" frequency, AFC and stereo/mono mode,\n",device);
   printf("           and then keep the tuner open.\n");
   printf("   -a            Sets tuner to run with AFC.  Default is off.\n");
   printf("   -d device     Sets device name for the tuner.  Default is \"%s\".\n",device);
   printf("   -f frequency  Sets tuner frequency (10Khz ie 88.1Mhz = 8810).\n");
   printf("                 Default is \"%i\".\n",frequency);
   printf("   -m            Sets tuner to run in mono.  Default is stereo.\n");
   return;
}

/*
 * cntlc_handler()
 *
 * runs when a user does a CNTL-C...
 *
 */

void
cntlc_handler (int whatever)
{
  exit (0);
}

/*
 * cleanup()
 *
 * runs on exit...
 *
 */
void cleanvoid ()
{
	cleanup(0);
}

void
cleanup (int whatever)
{
   printf ("Shutting down...\n");
   setaudio = AUDIO_EXTERN; 
   if(ioctl(devfh, BT848_SAUDIO, &setaudio) == -1){
      perror("set internal audio ");
   }
   setaudio = AUDIO_UNMUTE; 
   if(ioctl(devfh, BT848_SAUDIO, &setaudio) == -1){
      perror("set unmute audio ");
   }
  close (devfh);
}  
