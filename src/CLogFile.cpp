/*
 *    Copyright (C) 2018
 *    Albrecht Lohofener (albrechtloh@gmx.de)
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

#include "CLogFile.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

QString CLogFile::FileName;

void CLogFile::CustomMessageHandler(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    QString txt;
    QString DateTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    switch (type)
    {
    case QtInfoMsg: txt = DateTime + " " + QString("Info: %1").arg(str); break;
    case QtDebugMsg: txt = DateTime + " " + QString("Debug: %1").arg(str); break;
    case QtWarningMsg: txt = DateTime + " " + QString("Warning: %1").arg(str); break;
    case QtCriticalMsg: txt = DateTime + " " + QString("Critical: %1").arg(str); break;
    case QtFatalMsg: txt = DateTime + " " + QString("Fatal: %1").arg(str); abort();
    }

    // Write only of file name is set
    if(FileName != "")
    {
        QFile outFile(FileName);
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        ts << txt << endl;
    }
}

void CLogFile::SetFileName(QString FileName)
{
    CLogFile::FileName = FileName;
}
