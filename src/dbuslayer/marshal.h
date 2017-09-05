
#ifndef __monitor_marshal_MARSHAL_H__
#define __monitor_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:UINT,BOXED (marshal.list:1) */
extern void monitor_marshal_VOID__UINT_BOXED (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

G_END_DECLS

#endif /* __monitor_marshal_MARSHAL_H__ */

