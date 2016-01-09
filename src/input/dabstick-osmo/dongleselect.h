#
/*
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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
 *
 */
#
#ifndef	__DONGLE_SELECT
#define	__DONGLE_SELECT
#
#include	<QDialog>
#include	<QLabel>
#include	<QListView>
#include	<QStringListModel>
#include	<QStringList>
#include	<stdint.h>

class	dongleSelect: public QDialog {
Q_OBJECT
public:
	dongleSelect	(void);
	~dongleSelect	(void);
void	addtoDongleList	(const char *);
private:
	QLabel		*toptext;
	QListView	*selectorDisplay;
	QStringListModel	dongleList;
	QStringList	Dongles;
	int16_t		selectedItem;
private slots:
void	selectDongle	(QModelIndex);
};

#endif

