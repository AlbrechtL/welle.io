#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J (JSDR).
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
 *
 */
#ifndef	IP_DATAHANDLER
#define	IP_DATAHANDLER
#include	"dab-constants.h"
#include	"virtual-datahandler.h"
#include	<QByteArray>

class	RadioInterface;

class	ip_dataHandler:public virtual_dataHandler {
Q_OBJECT
public:
		ip_dataHandler		(RadioInterface *, bool);
		~ip_dataHandler		(void);
	void	add_mscDatagroup	(QByteArray &);
private:
	void	process_ipVector	(QByteArray &);
	void	process_udpVector	(uint8_t *, int16_t);
	bool	show_crcErrors;
	int16_t	handledPackets;
	int16_t	crcErrors;
signals:
	void	writeDatagram		(char *, int);
	void	show_ipErrors		(int);
};

#endif



