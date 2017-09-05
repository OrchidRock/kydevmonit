/*
 * Copyright (C) 2013 National University of Defense Technology(NUDT) & Kylin Ltd.
 *
 * Authors:
 *  Kobe Lee   kobe24_lixiang@126.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LISTEN_DBUS_H__
#define __LISTEN_DBUS_H__

#include <glib-object.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gio/gio.h>

G_BEGIN_DECLS


enum {
    SIG_DATA_ALERT,
    SIG_LAST_SIGNAL,
};

#define LISTEN_DBUS_NAME	"com.jd.test"
#define LISTEN_DBUS_PATH	"/com/jd/test"
#define LISTEN_DBUS_INTERFACE	"com.jd.monitor.test"
#define LISTEN_DBUS_ACTION_ID	"com.jd.monitor.authentication"
#define LISTEN_TYPE_DBUS	(monitor_dbus_get_type())

#define LISTEN_DBUS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), LISTEN_TYPE_DBUS, MonitorDbus))
#define LISTEN_DBUS_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), LISTEN_TYPE_DBUS, MonitorDbusClass))
#define LISTEN_IS_DBUS(o)	(G_TYPE_CHECK_INSTANCE_TYPE((o), LISTEN_TYPE_DBUS))
#define LISTEN_IS_DBUS_CLASS(k)	(G_TYPE_CHECK_CLASS_TPYE((k), LISTEN_TYPE_DBUS))
#define LISTEN_DBUS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS((o), LISTEN_TYPE_DBUS, MonitorClass))

typedef struct _MonitorDbusPrivate MonitorDbusPrivate;
typedef struct _MonitorDbus MonitorDbus;
typedef struct _MonitorDbusClass MonitorDbusClass;
typedef struct _MonitorWarnDbusClass MonitorWarnDbusClass;

struct _MonitorDbus {
    GObject parent;
    MonitorDbusPrivate *priv;
};

struct _MonitorDbusClass {
    GObjectClass parent_class;
    void (*event)(MonitorDbus* dbus, guint type, GValue * value);

};

struct _MonitorWarnDbusClass {
    GObjectClass parent_class;
    void (*event)(MonitorDbus* dbus, guint tag ,guint type, guint subtype);

};

GType monitor_dbus_get_type(void);
MonitorDbus * monitor_dbus_new(void);
gint monitor_dbus_call_monitor_method(MonitorDbus * dbus,GValueArray * method_value ,DBusGMethodInvocation * ret_value);
gint monitor_dbus_intropect(MonitorDbus * dbus,const char* devicename,DBusGMethodInvocation * ret_value);
#define DBUS_STRUCT_INT_INT_STRING (dbus_g_type_get_struct("GValueArray", G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING))

G_END_DECLS

int dbus_init(void);


void signal_handler();
#endif

