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
#ifndef	__SPECTROGRAM
#define	__SPECTROGRAM

#include	<stdio.h>
#include	<stdlib.h>
#include	<qwt_interval.h>
#include	<qwt_raster_data.h>

class	SpectrogramData: public QwtRasterData {
public:
	double	*data;		// pointer to actual data
	int	left;		// index of left most element in raster
	int	width;		// raster width
	int	height;		// rasterheigth
	int	datawidth;	// width of matrix
	int	dataheight;	// for now == rasterheigth
	double	max;

	SpectrogramData (double *data, int left, int width, int height,
	                 int datawidth, double max):
        QwtRasterData () {
	this	-> data		= data;
	this	-> left		= left;
	this	-> width	= width;
	this	-> height	= height;
	this	-> datawidth	= datawidth;
	this	-> dataheight	= height;
	this	-> max		= max;

	setInterval (Qt::XAxis, QwtInterval (left, left + width));
	setInterval (Qt::YAxis, QwtInterval (0, height));
	setInterval (Qt::ZAxis, QwtInterval (0, max));
}

void	initRaster (const QRectF &x, const QSize &raster) {
	(void)x;
	(void)raster;
}

QwtInterval Interval (Qt::Axis x)const {
	if (x == Qt::XAxis)
	   return QwtInterval (left, left + width);
	return QwtInterval (0, max);
}

	~SpectrogramData () {
}

double value (double x, double y) const {
//fprintf (stderr, "x = %f, y = %f\n", x, y);
	   x = x - left;
	   x = x / width * datawidth;
	   return data [(int)y * datawidth + (int)x];
}

};

#endif

