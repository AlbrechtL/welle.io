/****************** Warning Warning HACK HACK HACK **************
 *
 * This file is only to declare gstreamer types and functions to
 * use sdrdab headers without patching it. In welle.io gstreamer
 * is replaced by QMultimedia!
 *
 ****************** Warning Warning HACK HACK HACK **************/

/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *
 * gst.h: Main header for GStreamer, apps should include this
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef __GST_H__
#define __GST_H__

typedef int gboolean;
typedef int GOptionGroup;
typedef int GError;
typedef int guint;
typedef int gchar;
typedef int GOptionGroup;
typedef int GstTagList;
typedef int gint;
typedef int gpointer;
typedef int GstBus;
typedef int GstMessage;
typedef int GstElement;
typedef int GMainLoop;
typedef int GstPad;
typedef int GstPadProbeInfo;
typedef int GstPadProbeReturn;
void		gst_init			(int *argc, char **argv[]);
gboolean	gst_init_check			(int *argc, char **argv[],						 GError ** err);
gboolean        gst_is_initialized              (void);
GOptionGroup *	gst_init_get_option_group	(void);
void		gst_deinit			(void);
void		gst_version			(guint *major, guint *minor,						 guint *micro, guint *nano);
gchar *		gst_version_string		(void);
gboolean        gst_segtrap_is_enabled          (void);
void            gst_segtrap_set_enabled         (gboolean enabled);
gboolean        gst_registry_fork_is_enabled    (void);
void            gst_registry_fork_set_enabled   (gboolean enabled);
gboolean gst_update_registry (void);

#endif /* __GST_H__ */
