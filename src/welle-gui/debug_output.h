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

#ifndef CDEBUGOUTPUT_H
#define CDEBUGOUTPUT_H

#include <QString>
#include "gui_helper.h"

class CLogStringStream;

class CDebugOutput
{
public:
    static void init(void);
    static void customMessageHandler(QtMsgType type, const QMessageLogContext &, const QString & str);
    static void clogMessageHandler(std::string & str);
    static void handleMessage(QString str);
    static void setFileName(QString fileName);
    static void setCGUI(CGUIHelper *cGuiObject);

private:
    static QString fileName;
    static CGUIHelper *cGuiObject;
};

class CLogStringStream : public std::basic_streambuf<char, std::char_traits<char> > {
protected:
    int sync();
    int overflow(int c);

private:
    std::string buffer_;
};

#endif // CDEBUGOUTPUT_H
