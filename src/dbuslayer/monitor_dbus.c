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
 * You should have receiups a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "monitor_dbus.h"
#include "monitor_dbus_glue.h"
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "marshal.h"
#include <sys/timeb.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <signal.h>
#include <malloc.h>
#include <unistd.h>

#include "../interlayer/interlayer.h"

static GMainLoop *loop = NULL;
MonitorDbus * dbus;
static guint mnsignals[SIG_LAST_SIGNAL] = { 0 };

typedef void*(*PFUNC)(void*);

G_DEFINE_TYPE(MonitorDbus, monitor_dbus, G_TYPE_OBJECT);

typedef struct
{
    GMainLoop *loop;
    gboolean auth_ok;
    gint auth_rest; /* 0:successed, -1:failed, -2:challenge, -3:not authorized */
} AuthReturn;

MonitorDbus * monitor_dbus_new(void)
{
    return (MonitorDbus*) g_object_new(LISTEN_TYPE_DBUS, NULL);
}


void monitor_dbus_init(MonitorDbus *dbus)
{

}

void monitor_dbus_class_init(MonitorDbusClass * kclass)
{
    mnsignals[SIG_DATA_ALERT] = g_signal_new("monitor_alert",
                                           G_TYPE_FROM_CLASS(kclass),
                                           G_SIGNAL_RUN_FIRST,
                                           G_STRUCT_OFFSET(MonitorWarnDbusClass, event),
                                           NULL,
                                           NULL,
                                           monitor_marshal_VOID__UINT_BOXED,
                                           G_TYPE_NONE,
                                           3,
                                           G_TYPE_UINT,
                                           G_TYPE_UINT,
                       G_TYPE_UINT);
}



//客户端-监控统一入口
gint monitor_dbus_call_monitor_method(MonitorDbus * dbus,GValueArray * method_value ,DBusGMethodInvocation * ret_value)
{
    GValue * input[5] = {{0}};
    for(int i =0; i < 5;i++){
        input[i] = g_value_array_get_nth(method_value, i);
    }
    gint value1 = g_value_get_int(input[0]);
    gint value2 = g_value_get_int(input[1]);
    gint value3 = g_value_get_int(input[2]);
    gint value4 = g_value_get_int(input[3]);
    gchar * value5 = g_value_dup_string(input[4]);
    printf("input value1: %d\n", value1);
    printf("input value2: %d\n", value2);
    printf("input value3: %d\n", value3);
    printf("input value4: %d\n", value4);
    printf("input value5: %s\n", value5);
    

    gint statuscode = -1;
    gint count = -1;
    gchar statusdesc[1024] = {0};

    interlayer_call_device_method(value1, value2,value5, &statuscode,&count,statusdesc); 

    GValueArray *struct_return;
    GValue struct_container[3] = {{0}};
    
    g_value_init (&struct_container[0], G_TYPE_INT);
    g_value_set_int (&struct_container[0], statuscode);
    g_value_init (&struct_container[1], G_TYPE_INT);
    g_value_set_int (&struct_container[1], count);
    g_value_init (&struct_container[2], G_TYPE_STRING);
    g_value_set_string (&struct_container[2], statusdesc);

    struct_return = g_value_array_new(0);
    for(int j =0; j <3; j++){
        struct_return = g_value_array_append(struct_return, &struct_container[j]);
    }

	dbus_g_method_return(ret_value,struct_return);	
	return 1;
}
gint monitor_dbus_intropect(MonitorDbus* dbus, const char* devicename,DBusGMethodInvocation * ret_value){
    //char* message = "hello";
    //GValue data = g_value_set_string("hello");
    //printf("devicename= %s\n", devicename);
    char desc[1024];
    memset(desc, 0, 1024);
    interlayer_get_devices_intropect(LISTEN_DBUS_NAME, LISTEN_DBUS_PATH, devicename,desc); 
    printf("Desc: %s\n", desc);
    dbus_g_method_return(ret_value, desc);
    return 0;
}
int send_signal(MonitorDbus * dbus, gint s1){
    g_signal_emit(dbus, mnsignals[SIG_DATA_ALERT], 0, s1);
    return 0;
}

int send_mnsignals_timer_handler(MonitorDbus * dbus){
    printf("send signal\n");
    send_signal(dbus, 1);
    return 1;
}

int dbus_init(void)
{
    DBusGConnection * connection = NULL;
    DBusConnection *connect;
    DBusGProxy * dbus_proxy = NULL;
    GError * error = NULL;
    guint request_name_result;
    gint ret;
    dbus_g_thread_init();

    dbus = (MonitorDbus*)monitor_dbus_new();
    loop = g_main_loop_new(NULL, FALSE);

    connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
    if(connection == NULL){
        g_error("%s", error->message);
        goto out;
    }

    dbus_proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
                                           DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

    ret = dbus_g_proxy_call(dbus_proxy, "RequestName", &error,
                            G_TYPE_STRING, LISTEN_DBUS_NAME,
                            G_TYPE_UINT,
                            DBUS_NAME_FLAG_DO_NOT_QUEUE,
                            G_TYPE_INVALID,
                            G_TYPE_UINT, &request_name_result,
                            G_TYPE_INVALID);
    if(!ret){
        g_error("RequestName failed:%s", error->message);
        g_error_free(error);
        exit(EXIT_FAILURE);
    }

    g_object_unref(G_OBJECT(dbus_proxy));

    if(request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
        exit(EXIT_FAILURE);

    dbus_g_object_type_install_info(LISTEN_TYPE_DBUS,
                                    &dbus_glib_monitor_dbus_object_info);
    dbus_g_connection_register_g_object(connection, LISTEN_DBUS_PATH,G_OBJECT(dbus));

    // 定时发送signal
    //g_timeout_add_seconds(5, (GSourceFunc)send_mnsignals_timer_handler, dbus);

    interlayer_load_devices(LISTEN_DBUS_NAME, LISTEN_DBUS_PATH);

    g_main_loop_run(loop);
out:
    g_main_loop_unref(loop);
    g_object_unref(dbus);

    return 1;
}


void signal_handler(){
    g_main_loop_unref(loop);
    g_object_unref(dbus);
}
