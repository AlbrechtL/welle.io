#
/*
 *    Copyright (C) 2016
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
#ifndef	__DAB_CLIENT
#define	__DAB_CLIENT

#include	<QWidget>
#include	<QDialog>
#include	<QObject>
#include	<stdio.h>
#include	<stdint.h>
#include	<QSettings>
#include        <QLineEdit>
#include        <QHostAddress>
#include        <QByteArray>
#include        <QTcpSocket>
#include	<QStringList>
#include	<QStringListModel>
#include	"ui_dab-client.h"
#include	"client-constants.h"
class	audioSink;
class	dabClient:public QDialog,
	             private Ui_remote_control_dab_rpi {
Q_OBJECT
public:
		dabClient	(QSettings *, QWidget *parent = (QWidget *)0);
	       ~dabClient	(void);
private:
	bool            connected;
        QHostAddress    serverAddress;
        QTcpSocket	controlServer;
        QTcpSocket	soundServer;
        qint64          basePort;
	QSettings	*clientSettings;

	QStringListModel	ensemble;
	QStringList	Services;
	QString		ensembleLabel;

	void		setStreanOutSelector	(int);
	audioSink       *our_audioSink;
        int16_t         *outTable;
        bool            setupSoundOut   (QComboBox *,
                                         audioSink *,
                                         int32_t,
                                         int16_t        *);
	void		toBuffer	(QByteArray);
private slots:
        void            readMessages	(void);
	void		readSound	(void);
        void            setConnection   (void);
        void            wantConnect     (void);
	void		setDisconnect	(void);
	void		setStart	(void);
	void		setQuit		(void);
	void		setChannel	(const QString &);
	void		selectService	(QModelIndex);
	void		setAttenuation	(int);
	void		setStreamOutSelector (int);
};
#endif

