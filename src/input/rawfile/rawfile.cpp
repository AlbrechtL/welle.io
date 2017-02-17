/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    Bases on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include	"rawfile.h"
#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<fcntl.h>
//
#include	<sys/time.h>
#include	<time.h>

static inline int64_t getMyTime(void)
{
    struct timeval	tv;

	gettimeofday (&tv, NULL);
	return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}

#define	INPUT_FRAMEBUFFERSIZE	8 * 32768

rawFile::rawFile(QSettings *settings, bool *success)
{
    settings	-> beginGroup("rawfile");
    fileName = settings -> value ("RAW_file", "").toString();
    settings -> endGroup();

    fileName	= fileName;
	*success	= false;

	readerOK	= false;
    filePointer	= fopen (fileName. toLatin1 (). data (), "r");
    if (filePointer == NULL)
    {
       fprintf (stderr, "file %s cannot open\n", fileName. toLatin1 (). data ());
	   *success = false;
	   return;
	}

    SampleBuffer	= new RingBuffer<uint8_t>(INPUT_FRAMEBUFFERSIZE);
    SpectrumSampleBuffer = new RingBuffer<uint8_t>(8192);
	readerOK	= true;
	readerPausing	= true;
	*success	= true;
	currPos		= 0;
	start	();
}

rawFile::~rawFile(void)
{
	ExitCondition = true;
    if (readerOK)
    {
	   while (isRunning ())
	      usleep (100);
	   fclose (filePointer);
       delete SampleBuffer;
       delete SpectrumSampleBuffer;
	}
}

bool rawFile::restartReader(void)
{
	if (readerOK)
	   readerPausing = false;
	return readerOK;
}

void rawFile::stopReader(void)
{
	if (readerOK)
	   readerPausing = true;
}

uint8_t	rawFile::myIdentity(void)
{
	return FILEREADER;
}

//	size is in I/Q pairs, file contains 8 bits values
int32_t	rawFile::getSamples(DSPCOMPLEX *V, int32_t size)
{
    int32_t	amount, i;
    uint8_t	*temp = (uint8_t *)alloca (2 * size * sizeof (uint8_t));

	if (filePointer == NULL)
	   return 0;

    while ((int32_t)(SampleBuffer -> GetRingBufferReadAvailable ()) < 2 * size)
	   if (readerPausing)
	      usleep (100000);
	   else
	      msleep (100);

    amount = SampleBuffer	-> getDataFromBuffer (temp, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	   V [i] = DSPCOMPLEX (float (temp [2 * i] - 128) / 128.0,
	                       float (temp [2 * i + 1] - 128) / 128.0);
	return amount / 2;
}

int32_t	rawFile::getSamplesFromShadowBuffer (DSPCOMPLEX *V, int32_t size)
{
    int32_t	amount, i;
    uint8_t	*temp = (uint8_t *)alloca (2 * size * sizeof (uint8_t));

    amount = SpectrumSampleBuffer	-> getDataFromBuffer (temp, 2 * size);
    for (i = 0; i < amount / 2; i ++)
       V [i] = DSPCOMPLEX (float (temp [2 * i] - 128) / 128.0,
                           float (temp [2 * i + 1] - 128) / 128.0);
    return amount / 2;
}

int32_t	rawFile::Samples(void)
{
    return SampleBuffer -> GetRingBufferReadAvailable () / 2;
}

void rawFile::run(void)
{
    int32_t	t, i;
    uint8_t	*bi;
    int32_t	bufferSize	= 32768;
    int64_t	period;
    int64_t	nextStop;

	if (!readerOK)
	   return;

	ExitCondition = false;

	period		= (32768 * 1000) / (2 * 2048);	// full IQś read
	fprintf (stderr, "Period = %ld\n", period);
	bi		= new uint8_t [bufferSize];
	nextStop	= getMyTime ();
    while (!ExitCondition)
    {
       if (readerPausing)
       {
	      usleep (1000);
	      nextStop = getMyTime ();
	      continue;
	   }

       while (SampleBuffer -> WriteSpace () < bufferSize + 10)
       {
	      if (ExitCondition)
	         break;
	      usleep (100);
	   }

	   nextStop += period;
	   t = readBuffer (bi, bufferSize);
       if (t <= 0)
       {
	      for (i = 0; i < bufferSize; i ++)
	          bi [i] = 0;
	      t = bufferSize;
	   }
       SampleBuffer -> putDataIntoBuffer (bi, t);
       SpectrumSampleBuffer->putDataIntoBuffer(bi, t);
	   if (nextStop - getMyTime () > 0)
	      usleep (nextStop - getMyTime ());
	}
	fprintf (stderr, "taak voor replay eindigt hier\n");
}

/*
 *	length is number of uints that we read.
 */
int32_t	rawFile::readBuffer (uint8_t *data, int32_t length)
{
    int32_t	n;

	n = fread (data, sizeof (uint8_t), length, filePointer);
	currPos		+= n;
	if (n < length) {
	   fseek (filePointer, 0, SEEK_SET);
	   fprintf (stderr, "End of file, restarting\n");
	}
	return	n & ~01;
}

