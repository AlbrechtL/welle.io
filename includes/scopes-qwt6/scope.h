#
/*
 *    Copyright (C) 2008, 2009, 2010
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
 */

#ifndef	__SCOPES
#define	__SCOPES

#include	<QObject>
#include	<QStackedWidget>
#include	<qwt.h>
#include	<qwt_plot.h>
#include	<qwt_plot_curve.h>
#include	<qwt_plot_marker.h>
#include	<qwt_plot_grid.h>
#include	<qwt_color_map.h>
#include	<qwt_plot_spectrogram.h>
#include	<qwt_plot_zoomer.h>
#include	<qwt_plot_panner.h>
#include	<qwt_plot_layout.h>
#include	<qwt_scale_widget.h>
#include	<QBrush>
#include	<QTimer>
#include	<stdint.h>
#include	"spectrogramdata.h"

class	SpectrumViewer;
class	WaterfallViewer;
class	QwtLinearColorMap;
class	QwtScaleWidget;
/*
 *	The stacked object is called the scope, it is built as a wrapper
 *	around the actual display mechanisms the waterfall and the
 *	spectrumviewer.
 */

#define	WATERFALL_MODE	0
#define	SPECTRUM_MODE	1

class Scope: public QObject {
Q_OBJECT
public:
	Scope (QwtPlot *, uint16_t, uint16_t);
	~Scope (void);
void	Display 	(double *, double *, double, int32_t);
void	SelectView	(uint8_t);
void	setBitDepth	(int16_t);
private:
	QwtPlot		*Plotter;
	uint16_t	Displaysize;
	uint16_t	Rastersize;
	uint8_t		CurrentWidget;
	SpectrumViewer	*Spectrum;
	WaterfallViewer	*Waterfall;
	int16_t		bitDepth;
	float		get_db		(float);
private slots:
	void		leftClicked	(int);
	void		rightClicked	(int);
signals:
	void		clickedwithLeft	(int);
	void		clickedwithRight	(int);
};

/*
 *	for the waterfall display
 */
class WaterfallViewer: public QObject, public QwtPlotSpectrogram {
Q_OBJECT
public:
	WaterfallViewer		(QwtPlot *, uint16_t, uint16_t);
	~WaterfallViewer	();
void	ViewWaterfall 		(double *,
	                         double *,
	                         double,
	                         int32_t);
private:
	SpectrogramData	*WaterfallData;
	QwtPlot		*plotgrid;
	uint16_t	Displaysize;
	uint16_t	Rastersize;
	double		*plotData;
	QwtPlotMarker	*Marker;
	uint16_t	IndexforMarker;
	QwtPlotPicker	*lm_picker;
	uint8_t		OneofTwo;
	QwtLinearColorMap	*colorMap;
	QwtScaleWidget		*rightAxis;
private slots:
	void	leftMouseClick (const QPointF &);
signals:
	void	leftClicked (int);
};
/*
 *	The spectrumDisplay
 */
class	SpectrumViewer: public QObject {
Q_OBJECT
public:
	SpectrumViewer 	(QwtPlot *, uint16_t);
	~SpectrumViewer	(void);
void	ViewSpectrum	(double *, double *, double, int32_t);
void	setBitDepth	(int16_t);
private:
	QwtPlot		*plotgrid;
	uint16_t	Displaysize;
	QwtPlotGrid	*grid;
	QwtPlotCurve	*SpectrumCurve;
	QwtPlotMarker	*Marker;
	uint32_t	IndexforMarker;
	QwtPlotPicker	*lm_picker;
	QwtPlotPicker	*rm_picker;
	QBrush		*ourBrush;
	int16_t		bitDepth;
	int32_t		normalizer;
	float		get_db		(float);
private slots:
	void	leftMouseClick (const QPointF &);
	void	rightMouseClick (const QPointF &);
signals:
	void	leftClicked	(int);
	void	rightClicked	(int);
};
#endif

