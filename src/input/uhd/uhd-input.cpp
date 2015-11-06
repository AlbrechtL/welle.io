/*
 *
 *    Copyright (C) 2015
 *    Sebastian Held <sebastian.held@imst.de>
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"uhd-input.h"

#include <uhd/types/tune_request.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

	uhd_streamer::uhd_streamer (uhdInput *d) {
	m_theStick		= d;
	m_stop_signal_called	= false;
//create a receive streamer
	uhd::stream_args_t stream_args( "fc32", "sc16" );
	m_theStick -> m_rx_stream =
	          m_theStick -> m_usrp -> get_rx_stream (stream_args);
//setup streaming
	uhd::stream_cmd_t stream_cmd (uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS );
	stream_cmd.num_samps	= 0;
	stream_cmd.stream_now	= true;
	stream_cmd.time_spec	= uhd::time_spec_t();
	m_theStick -> m_rx_stream -> issue_stream_cmd (stream_cmd);

	start();
}

void	uhd_streamer::stop (void) {
	m_stop_signal_called = true;
	while (isRunning ())
	   wait(1);
}

void	uhd_streamer::run (void) {
	while (!m_stop_signal_called) {
//	get write position, ignore data2 and size2
	   int32_t size1, size2;
	   void *data1, *data2;
	   m_theStick -> theBuffer -> GetRingBufferWriteRegions (10000,
	                                                         &data1,
	                                                         &size1,
	                                                         &data2,
	                                                         &size2);

	   if (size1 == 0) {
// no room in ring buffer, wait for main thread to process the data
	      usleep (100); // wait 100 us
	      continue;
	   }

	   uhd::rx_metadata_t md;
	   size_t num_rx_samps =
	         m_theStick -> m_rx_stream -> recv (data1, size1, md, 1.0);
	   m_theStick -> theBuffer -> AdvanceRingBufferWriteIndex (num_rx_samps);

	   if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
	      std::cout << boost::format ("Timeout while streaming") << std::endl;
	      continue;
	   }

	   if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
	      std::cerr << boost::format ("Got an overflow indication") << std::endl;
	      continue;
	   }

//	   if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
//	      std::cerr << boost::format("Receiver error: %s") % md.strerror() << std::endl;
//	      continue;
//	   }
	}
}

	uhdInput::uhdInput (QSettings *s, bool *success ) {
	this	-> uhdSettings	= s;
	this	-> myFrame	= new QFrame (NULL);
	setupUi (this	-> myFrame);
	this	-> myFrame	-> show ();
	this	-> inputRate	= Khz (2048);
	this	-> ringbufferSize	= 1024;	// blocks of 1024 complexes
	this	-> theBuffer	= NULL;	// also indicates good init or not
	*success		= false;
	lastFrequency		= 100000;
	m_workerHandle		= 0;
//	create a usrp device.
	std::string args;
	std::cout << std::endl;
	std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
	try {
	   m_usrp = uhd::usrp::multi_usrp::make (args);
//	Lock mboard clocks

	   std::string ref("internal");
	   m_usrp -> set_clock_source (ref);

	   std::cout << boost::format("Using Device: %s") % m_usrp->get_pp_string() << std::endl;
//	set sample rate
	   m_usrp -> set_rx_rate (inputRate);
	   inputRate = m_usrp -> get_rx_rate ();
	   std::cout << boost::format("Actual RX Rate: %f Msps...") % (inputRate/1e6) << std::endl << std::endl;

//	allocate the rx buffer
	   theBuffer	= new RingBuffer<std::complex<float> >(ringbufferSize * 1024);
	}
	catch (...) {
	   fprintf (stderr, "No luck with uhd\n");
	   return;
	}
//	some housekeeping for the local frame
	externalGain		-> setMaximum (maxGain ());
	uhdSettings		-> beginGroup ("uhdSettings");
	externalGain 		-> setValue (
	            uhdSettings -> value ("externalGain", 40). toInt ());
	f_correction		-> setValue (
	            uhdSettings -> value ("f_correction", 0). toInt ());
	KhzOffset		-> setValue (
	            uhdSettings -> value ("KhzOffset", 0). toInt ());
	uhdSettings	-> endGroup ();

	setExternalGain	(externalGain	-> value ());
	set_KhzOffset	(KhzOffset	-> value ());
	connect (externalGain, SIGNAL (valueChanged (int)),
	         this, SLOT (setExternalGain (int)));
	connect (KhzOffset, SIGNAL (valueChanged (int)),
	         this, SLOT (set_KhzOffset (int)));
	*success 		= true;
}

	uhdInput::~uhdInput (void) {
	if (theBuffer != NULL) {
	   stopReader();
	   uhdSettings	-> beginGroup ("uhdSettings");
	   uhdSettings	-> setValue ("externalGain", 
	                                      externalGain -> value ());
	   uhdSettings	-> setValue ("f_correction",
	                                      f_correction -> value ());
	   uhdSettings	-> setValue ("KhzOffset",
	                                      KhzOffset	-> value ());
	   uhdSettings	-> endGroup ();
	   delete theBuffer;
	}
	delete myFrame;
}

void	uhdInput::setVFOFrequency (int32_t freq) {
	std::cout << boost::format ("Setting RX Freq: %f MHz...") % (freq/1e6) << std::endl;
	uhd::tune_request_t tune_request (freq);
	m_usrp->set_rx_freq (tune_request);
}

int32_t	uhdInput::getVFOFrequency	(void) {
int32_t freq = m_usrp -> get_rx_freq ();
	std::cout << boost::format("Actual RX Freq: %f MHz...") % (freq/1e6) << std::endl << std::endl;
	return freq;
}

bool	uhdInput::restartReader	(void) {
	if (m_workerHandle != 0)
	   return true;

	theBuffer -> FlushRingBuffer ();
	m_workerHandle = new uhd_streamer (this);
	return true;
}

void	uhdInput::stopReader	(void) {
	if (m_workerHandle == 0)
	   return;

	m_workerHandle -> stop ();

	delete m_workerHandle;
	m_workerHandle = 0;
}
//
//	not used:
uint8_t	uhdInput::myIdentity	(void) {
	return DAB_STICK;
}

int32_t	uhdInput::getSamples	(DSPCOMPLEX *v, int32_t size) {
	size = std::min ((uint32_t)size,
	                 (uint32_t)(theBuffer -> GetRingBufferReadAvailable ()));
	theBuffer -> getDataFromBuffer (v, size);
	return size;
}

int32_t	uhdInput::Samples		(void) {
	return theBuffer -> GetRingBufferReadAvailable();
}

void	uhdInput::resetBuffer	(void) {
	theBuffer -> FlushRingBuffer();
}

void	uhdInput::set_fCorrection	(int32_t f) {
	(void)f;
}

int16_t	uhdInput::maxGain		(void) {
	uhd::gain_range_t range = m_usrp->get_rx_gain_range();
	return	range.stop();
}

void	uhdInput::setExternalGain	(int32_t gain) {
	std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
	m_usrp -> set_rx_gain (gain);
	double gain_f = m_usrp -> get_rx_gain ();
	std::cout << boost::format("Actual RX Gain: %f dB...") % gain_f << std::endl << std::endl;
}

void	uhdInput::set_KhzOffset	(int32_t o) {
	vfoOffset	= o;
}

int16_t	uhdInput::bitDepth	(void) {
	return 16;
}

