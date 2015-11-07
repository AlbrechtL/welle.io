#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
 *	Communication via network to a DAB receiver to 
 *	have the sound locally
 */

#ifndef	__SOUND_CLIENT__
#define	__SOUND_CLIENT__
#include	"sound-constants.h"
#include	<QDialog>
#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QTcpSocket>
#include	<QHostAddress>
#include	<QTimer>
#include	"ui_soundwidget.h"
//

class	audioSink;
class	soundClient:public QDialog,
	            public Ui_soundwidget {
Q_OBJECT
public:
		soundClient	(QSettings *, QWidget *parent = NULL);
		~soundClient	(void);

	QSettings	*remoteSettings;
	bool		connected;
private	slots:
	void		wantConnect	(void);
	void		setConnection	(void);
	void		readData	(void);
	void		toBuffer	(QByteArray);
	void		terminate	(void);
	void	        setGain		(int);
	void		setStreamOutSelector	(int);
	void		timerTick	(void);
private:
	QTcpSocket	streamer;
	audioSink	*our_audioSink;
	int16_t		*outTable;
	bool		setupSoundOut	(QComboBox *,
	                                 audioSink *,
	                                 int32_t,
	                                 int16_t	*);
	QTimer		*connectionTimer;
};
#endif


