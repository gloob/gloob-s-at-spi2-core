/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2002 Ximian Inc.
 * Copyright 2002 Sun Microsystems, Inc.
 * Copyright 2010, 2011 Novell, Inc.
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "atspi-private.h"
#include <string.h>
#include <ctype.h>

typedef struct
{
  AtspiEventListenerCB callback;
  void *user_data;
  GDestroyNotify callback_destroyed;
  char *category;
  char *name;
  char *detail;
} EventListenerEntry;

G_DEFINE_TYPE (AtspiEventListener, atspi_event_listener, G_TYPE_OBJECT)

void
atspi_event_listener_init (AtspiEventListener *listener)
{
}

void
atspi_event_listener_class_init (AtspiEventListenerClass *klass)
{
}

static void
remove_datum (const AtspiEvent *event, void *user_data)
{
  AtspiEventListenerSimpleCB cb = user_data;
  cb (event);
}

typedef struct
{
  gpointer callback;
  GDestroyNotify callback_destroyed;
  gint ref_count;
} CallbackInfo;
static GHashTable *callbacks;

void
callback_ref (void *callback, GDestroyNotify callback_destroyed)
{
  CallbackInfo *info;

  if (!callbacks)
  {
    callbacks = g_hash_table_new (g_direct_hash, g_direct_equal);
    if (!callbacks)
      return;
  }

  info = g_hash_table_lookup (callbacks, callback);
  if (!info)
  {
    info = g_new (CallbackInfo, 1);
    info->callback = callback;
    info->callback_destroyed = callback_destroyed;
    info->ref_count = 1;
    g_hash_table_insert (callbacks, callback, info);
  }
  else
    info->ref_count++;
}

void
callback_unref (gpointer callback)
{
  CallbackInfo *info;

  if (!callbacks)
    return;
  info = g_hash_table_lookup (callbacks, callback);
  if (!info)
  {
    g_warning ("Atspi: Dereferencing invalid callback %p\n", callback);
    return;
  }
  info->ref_count--;
  if (info->ref_count == 0)
  {
#if 0
    /* TODO: Figure out why this seg faults from Python */
    if (info->callback_destroyed)
      (*info->callback_destroyed) (info->callback);
#endif
    g_free (info);
    g_hash_table_remove (callbacks, callback);
  }
}

/**
 * atspi_event_listener_new:
 * @callback: (scope notified): An #AtspiEventListenerSimpleCB to be called
 * when an event is fired.
 * @user_data: (closure): data to pass to the callback.
 * @callback_destroyed: A #GDestroyNotify called when the listener is freed
 * and data associated with the callback should be freed.  Can be NULL.
 *
 * Creates a new #AtspiEventListener associated with a specified @callback.
 *
 * Returns: (transfer full): A new #AtspiEventListener.
 */
AtspiEventListener *
atspi_event_listener_new (AtspiEventListenerCB callback,
                                 gpointer user_data,
                                 GDestroyNotify callback_destroyed)
{
  AtspiEventListener *listener = g_object_new (ATSPI_TYPE_EVENT_LISTENER, NULL);
  listener->callback = callback;
  callback_ref (callback, callback_destroyed);
  listener->user_data = user_data;
  listener->cb_destroyed = callback_destroyed;
  return listener;
}

/**
 * atspi_event_listener_new_simple:
 * @callback: (scope notified): An #AtspiEventListenerSimpleCB to be called
 * when an event is fired.
 * @callback_destroyed: A #GDestroyNotify called when the listener is freed
 * and data associated with the callback should be freed.  Can be NULL.
 *
 * Creates a new #AtspiEventListener associated with a specified @callback.
 * Returns: (transfer full): A new #AtspiEventListener.
 **/
AtspiEventListener *
atspi_event_listener_new_simple (AtspiEventListenerSimpleCB callback,
                                 GDestroyNotify callback_destroyed)
{
  AtspiEventListener *listener = g_object_new (ATSPI_TYPE_EVENT_LISTENER, NULL);
  listener->callback = remove_datum;
  callback_ref (remove_datum, callback_destroyed);
  listener->user_data = callback;
  listener->cb_destroyed = callback_destroyed;
  return listener;
}

static GList *event_listeners = NULL;

static gchar *
convert_name_from_dbus (const char *name)
{
  gchar *ret = g_malloc (g_utf8_strlen (name, -1) * 2 + 1);
  const char *p = name;
  gchar *q = ret;

  if (!ret)
    return NULL;

  while (*p)
  {
    if (isupper (*p))
    {
      if (q > ret)
        *q++ = '-';
      *q++ = tolower (*p++);
    }
    else
      *q++ = *p++;
  }
  *q = '\0';
  return ret;
}

static void
cache_process_children_changed (AtspiEvent *event)
{
  AtspiAccessible *child;

  if (!G_VALUE_HOLDS (&event->any_data, ATSPI_TYPE_ACCESSIBLE) ||
      !(event->source->cached_properties & ATSPI_CACHE_CHILDREN) ||
      atspi_state_set_contains (event->source->states, ATSPI_STATE_MANAGES_DESCENDANTS))
    return;

  child = g_value_get_object (&event->any_data);

  if (!strncmp (event->type, "object:children-changed:add", 27))
  {
    if (g_list_find (event->source->children, child))
      return;
    GList *new_list = g_list_insert (event->source->children, g_object_ref (child), event->detail1);
    if (new_list)
      event->source->children = new_list;
  }
  else if (g_list_find (event->source->children, child))
  {
    event->source->children = g_list_remove (event->source->children, child);
    if (child == child->parent.app->root)
      g_object_run_dispose (G_OBJECT (child->parent.app));
    g_object_unref (child);
  }
}

static void
cache_process_property_change (AtspiEvent *event)
{
  if (!strcmp (event->type, "object:property-change:accessible-parent"))
  {
    if (event->source->accessible_parent)
      g_object_unref (event->source->accessible_parent);
    if (G_VALUE_HOLDS (&event->any_data, ATSPI_TYPE_ACCESSIBLE))
    {
      event->source->accessible_parent = g_value_dup_object (&event->any_data);
      _atspi_accessible_add_cache (event->source, ATSPI_CACHE_PARENT);
    }
    else
    {
      event->source->accessible_parent = NULL;
      event->source->cached_properties &= ~ATSPI_CACHE_PARENT;
    }
  }
  else if (!strcmp (event->type, "object:property-change:accessible-name"))
  {
    if (event->source->name)
      g_free (event->source->name);
    if (G_VALUE_HOLDS_STRING (&event->any_data))
    {
      event->source->name = g_value_dup_string (&event->any_data);
      _atspi_accessible_add_cache (event->source, ATSPI_CACHE_NAME);
    }
    else
    {
      event->source->name = NULL;
      event->source->cached_properties &= ~ATSPI_CACHE_NAME;
    }
  }
  else if (!strcmp (event->type, "object:property-change:accessible-description"))
  {
    if (event->source->description)
      g_free (event->source->description);
    if (G_VALUE_HOLDS_STRING (&event->any_data))
    {
      event->source->description = g_value_dup_string (&event->any_data);
      _atspi_accessible_add_cache (event->source, ATSPI_CACHE_DESCRIPTION);
    }
    else
    {
      event->source->description = NULL;
      event->source->cached_properties &= ~ATSPI_CACHE_DESCRIPTION;
    }
  }
}

static void
cache_process_state_changed (AtspiEvent *event)
{
  if (event->source->states)
    atspi_state_set_set_by_name (event->source->states, event->type + 21,
                                 event->detail1);
}

static dbus_bool_t
demarshal_rect (DBusMessageIter *iter, AtspiRect *rect)
{
  dbus_int32_t x, y, width, height;
  DBusMessageIter iter_struct;

  dbus_message_iter_recurse (iter, &iter_struct);
  if (dbus_message_iter_get_arg_type (&iter_struct) != DBUS_TYPE_INT32) return FALSE;
  dbus_message_iter_get_basic (&iter_struct, &x);
  dbus_message_iter_next (&iter_struct);
  if (dbus_message_iter_get_arg_type (&iter_struct) != DBUS_TYPE_INT32) return FALSE;
  dbus_message_iter_get_basic (&iter_struct, &y);
  dbus_message_iter_next (&iter_struct);
  if (dbus_message_iter_get_arg_type (&iter_struct) != DBUS_TYPE_INT32) return FALSE;
  dbus_message_iter_get_basic (&iter_struct, &width);
  dbus_message_iter_next (&iter_struct);
  if (dbus_message_iter_get_arg_type (&iter_struct) != DBUS_TYPE_INT32) return FALSE;
  dbus_message_iter_get_basic (&iter_struct, &height);
  rect->x = x;
  rect->y = y;
  rect->width = width;
  rect->height = height;
  return TRUE;
}

static gchar *
strdup_and_adjust_for_dbus (const char *s)
{
  gchar *d = g_strdup (s);
  gchar *p;
  int parts = 0;

  if (!d)
    return NULL;

  for (p = d; *p; p++)
  {
    if (*p == '-')
    {
      memmove (p, p + 1, g_utf8_strlen (p, -1));
      *p = toupper (*p);
    }
    else if (*p == ':')
    {
      parts++;
      if (parts == 2)
        break;
      p [1] = toupper (p [1]);
    }
  }

  d [0] = toupper (d [0]);
  return d;
}

static gboolean
convert_event_type_to_dbus (const char *eventType, char **categoryp, char **namep, char **detailp, char **matchrule)
{
  gchar *tmp = strdup_and_adjust_for_dbus (eventType);
  char *category = NULL, *name = NULL, *detail = NULL;
  char *saveptr = NULL;

  if (tmp == NULL) return FALSE;
  category = strtok_r (tmp, ":", &saveptr);
  if (category) category = g_strdup (category);
  if (!category) goto oom;
  name = strtok_r (NULL, ":", &saveptr);
  if (name)
  {
    name = g_strdup (name);
    if (!name) goto oom;
    detail = strtok_r (NULL, ":", &saveptr);
    if (detail) detail = g_strdup (detail);
  }
  if (matchrule)
  {
    *matchrule = g_strdup_printf ("type='signal',interface='org.a11y.atspi.Event.%s'", category);
    if (!*matchrule) goto oom;
    if (name && name [0])
    {
      gchar *new_str = g_strconcat (*matchrule, ",member='", name, "'", NULL);
      g_free (*matchrule);
      *matchrule = new_str;
    }
    if (detail && detail [0])
    {
      gchar *new_str = g_strconcat (*matchrule, ",arg0='", detail, "'", NULL);
      g_free (*matchrule);
      *matchrule = new_str;
    }
  }
  if (categoryp) *categoryp = category;
  else g_free (category);
  if (namep) *namep = name;
  else if (name) g_free (name);
  if (detailp) *detailp = detail;
  else if (detail) g_free (detail);
  g_free (tmp);
  return TRUE;
oom:
  if (tmp) g_free (tmp);
  if (category) g_free (category);
  if (name) g_free (name);
  if (detail) g_free (detail);
  return FALSE;
}

static void
listener_entry_free (EventListenerEntry *e)
{
  gpointer callback = (e->callback == remove_datum ? (gpointer)e->user_data : (gpointer)e->callback);
  g_free (e->category);
  g_free (e->name);
  if (e->detail) g_free (e->detail);
  callback_unref (callback);
  g_free (e);
}

/**
 * atspi_event_listener_register:
 * @listener: The #AtspiEventListener to register against an event type.
 * @event_type: a character string indicating the type of events for which
 *            notification is requested.  Format is
 *            EventClass:major_type:minor_type:detail
 *            where all subfields other than EventClass are optional.
 *            EventClasses include "object", "window", "mouse",
 *            and toolkit events (e.g. "Gtk", "AWT").
 *            Examples: "focus:", "Gtk:GtkWidget:button_press_event".
 *
 * Adds an in-process callback function to an existing #AtspiEventListener.
 *
 * Legal object event types:
 *
 *    (property change events)
 *
 *            object:property-change
 *            object:property-change:accessible-name
 *            object:property-change:accessible-description
 *            object:property-change:accessible-parent
 *            object:property-change:accessible-value
 *            object:property-change:accessible-role
 *            object:property-change:accessible-table-caption
 *            object:property-change:accessible-table-column-description
 *            object:property-change:accessible-table-column-header
 *            object:property-change:accessible-table-row-description
 *            object:property-change:accessible-table-row-header
 *            object:property-change:accessible-table-summary
 *
 *    (other object events)
 *
 *            object:state-changed 
 *            object:children-changed
 *            object:visible-data-changed
 *            object:selection-changed
 *            object:text-selection-changed
 *            object:text-changed
 *            object:text-caret-moved
 *            object:row-inserted
 *            object:row-reordered
 *            object:row-deleted
 *            object:column-inserted
 *            object:column-reordered
 *            object:column-deleted
 *            object:model-changed
 *            object:active-descendant-changed
 *
 *  (window events)
 *
 *            window:minimize
 *            window:maximize
 *            window:restore
 *            window:close
 *            window:create
 *            window:reparent
 *            window:desktop-create
 *            window:desktop-destroy
 *            window:activate
 *            window:deactivate
 *            window:raise
 *            window:lower
 *            window:move
 *            window:resize
 *            window:shade
 *            window:unshade
 *            window:restyle
 *
 *  (other events)
 *
 *            focus:
 *            mouse:abs
 *            mouse:rel
 *            mouse:b1p
 *            mouse:b1r
 *            mouse:b2p
 *            mouse:b2r
 *            mouse:b3p
 *            mouse:b3r
 *
 * NOTE: this character string may be UTF-8, but should not contain byte 
 * value 56
 *            (ascii ':'), except as a delimiter, since non-UTF-8 string
 *            delimiting functions are used internally.
 *            In general, listening to
 *            toolkit-specific events is not recommended.
 *
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_register (AtspiEventListener *listener,
				             const gchar              *event_type,
				             GError **error)
{
  /* TODO: Keep track of which events have been registered, so that we
 * deregister all of them when the event listener is destroyed */

  return atspi_event_listener_register_from_callback (listener->callback,
                                                      listener->user_data,
                                                      listener->cb_destroyed,
                                                      event_type, error);
}

/**
 * atspi_event_listener_register_from_callback:
 * @callback: (scope notified): the #AtspiEventListenerCB to be registered 
 * against an event type.
 * @user_data: (closure): User data to be passed to the callback.
 * @callback_destroyed: A #GDestroyNotify called when the callback is destroyed.
 * @event_type: a character string indicating the type of events for which
 *            notification is requested.  See #atspi_event_listener_register
 * for a description of the format.
 * 
 * Registers an #AtspiEventListenerCB against an @event_type.
 *
 * Returns: #TRUE if successfull, otherwise #FALSE.
 *
 **/
gboolean
atspi_event_listener_register_from_callback (AtspiEventListenerCB callback,
				             void *user_data,
				             GDestroyNotify callback_destroyed,
				             const gchar              *event_type,
				             GError **error)
{
  EventListenerEntry *e;
  char *matchrule;
  DBusError d_error;
  GList *new_list;
  DBusMessage *message, *reply;

  if (!callback)
    {
      return FALSE;
    }

  if (!event_type)
  {
    g_warning ("called atspi_event_listener_register_from_callback with a NULL event_type");
    return FALSE;
  }

  e = g_new (EventListenerEntry, 1);
  e->callback = callback;
  e->user_data = user_data;
  e->callback_destroyed = callback_destroyed;
  callback_ref (callback == remove_datum ? (gpointer)user_data : (gpointer)callback,
                callback_destroyed);
  if (!convert_event_type_to_dbus (event_type, &e->category, &e->name, &e->detail, &matchrule))
  {
    g_free (e);
    return FALSE;
  }
  new_list = g_list_prepend (event_listeners, e);
  if (!new_list)
  {
    listener_entry_free (e);
    return FALSE;
  }
  event_listeners = new_list;
  dbus_error_init (&d_error);
  dbus_bus_add_match (_atspi_bus(), matchrule, &d_error);
  if (d_error.message)
  {
    g_warning ("Atspi: Adding match: %s", d_error.message);
    /* TODO: Set error */
  }

  dbus_error_init (&d_error);
  message = dbus_message_new_method_call (atspi_bus_registry,
	atspi_path_registry,
	atspi_interface_registry,
	"RegisterEvent");
  if (!message)
    return FALSE;
  dbus_message_append_args (message, DBUS_TYPE_STRING, &event_type, DBUS_TYPE_INVALID);
  reply = _atspi_dbus_send_with_reply_and_block (message, error);
  if (reply)
    dbus_message_unref (reply);

  return TRUE;
}

/**
 * atspi_event_listener_register_no_data:
 * @callback: (scope notified): the #AtspiEventListenerSimpleCB to be
 *            registered against an event type.
 * @callback_destroyed: A #GDestroyNotify called when the callback is destroyed.
 * @event_type: a character string indicating the type of events for which
 *            notification is requested.  Format is
 *            EventClass:major_type:minor_type:detail
 *            where all subfields other than EventClass are optional.
 *            EventClasses include "object", "window", "mouse",
 *            and toolkit events (e.g. "Gtk", "AWT").
 *            Examples: "focus:", "Gtk:GtkWidget:button_press_event".
 *
 * Registers an #AtspiEventListenetSimpleCB. The method is similar to 
 * #atspi_event_listener_register, but @callback takes no user_data.
 *
 * Returns: #TRUE if successfull, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_register_no_data (AtspiEventListenerSimpleCB callback,
				 GDestroyNotify callback_destroyed,
				 const gchar              *event_type,
				 GError **error)
{
  return atspi_event_listener_register_from_callback (remove_datum, callback,
                                                      callback_destroyed,
                                                      event_type, error);
}

static gboolean
is_superset (const gchar *super, const gchar *sub)
{
  if (!super || !super [0])
    return TRUE;
  return (strcmp (super, sub) == 0);
}

/**
 * atspi_event_listener_deregister:
 * @listener: The #AtspiEventListener to deregister.
 * @event_type: a string specifying the event type for which this
 *             listener is to be deregistered.
 *
 * Deregisters an #AtspiEventListener from the registry, for a specific
 *             event type.
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_deregister (AtspiEventListener *listener,
				               const gchar              *event_type,
				               GError **error)
{
  return atspi_event_listener_deregister_from_callback (listener->callback,
                                                        listener->user_data,
                                                        event_type, error);
}

/**
 * atspi_event_listener_deregister_from_callback:
 * @callback: (scope call): the #AtspiEventListenerCB registered against an
 *            event type.
 * @user_data: (closure): User data that was passed in for this callback.
 * @event_type: a string specifying the event type for which this
 *             listener is to be deregistered.
 *
 * Deregisters an #AtspiEventListenerCB from the registry, for a specific
 *             event type.
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_deregister_from_callback (AtspiEventListenerCB callback,
				               void *user_data,
				               const gchar              *event_type,
				               GError **error)
{
  char *category, *name, *detail, *matchrule;
  GList *l;

  if (!convert_event_type_to_dbus (event_type, &category, &name, &detail, &matchrule))
  {
    return FALSE;
  }
  if (!callback)
    {
      return FALSE;
    }

  for (l = event_listeners; l;)
  {
    EventListenerEntry *e = l->data;
    if (e->callback == callback &&
        e->user_data == user_data &&
        is_superset (category, e->category) &&
        is_superset (name, e->name) &&
        is_superset (detail, e->detail))
    {
      gboolean need_replace;
      DBusError d_error;
      DBusMessage *message, *reply;
      need_replace = (l == event_listeners);
      l = g_list_remove (l, e);
      if (need_replace)
        event_listeners = l;
      dbus_error_init (&d_error);
      dbus_bus_remove_match (_atspi_bus(), matchrule, &d_error);
      dbus_error_init (&d_error);
      message = dbus_message_new_method_call (atspi_bus_registry,
	    atspi_path_registry,
	    atspi_interface_registry,
	    "DeregisterEvent");
      if (!message)
      return FALSE;
      dbus_message_append_args (message, DBUS_TYPE_STRING, &event_type, DBUS_TYPE_INVALID);
      reply = _atspi_dbus_send_with_reply_and_block (message, error);
      if (reply)
        dbus_message_unref (reply);

      listener_entry_free (e);
    }
    else l = g_list_next (l);
  }
  g_free (category);
  g_free (name);
  if (detail) g_free (detail);
  g_free (matchrule);
  return TRUE;
}

/**
 * atspi_event_listener_deregister_no_data:
 * @callback: (scope call): the #AtspiEventListenerSimpleCB registered against
 *            an event type.
 * @event_type: a string specifying the event type for which this
 *             listener is to be deregistered.
 *
 * deregisters an #AtspiEventListenerSimpleCB from the registry, for a specific
 *             event type.
 *
 * Returns: #TRUE if successful, otherwise #FALSE.
 **/
gboolean
atspi_event_listener_deregister_no_data (AtspiEventListenerSimpleCB callback,
				   const gchar              *event_type,
				   GError **error)
{
  return atspi_event_listener_deregister_from_callback (remove_datum, callback,
                                                        event_type,
                                                        error);
}

static AtspiEvent *
atspi_event_copy (AtspiEvent *src)
{
  AtspiEvent *dst = g_new0 (AtspiEvent, 1);
  dst->type = g_strdup (src->type);
  dst->source = g_object_ref (src->source);
  dst->detail1 = src->detail1;
  dst->detail2 = src->detail2;
  g_value_init (&dst->any_data, G_VALUE_TYPE (&src->any_data));
  g_value_copy (&src->any_data, &dst->any_data);
  return dst;
}

static void
atspi_event_free (AtspiEvent *event)
{
  g_object_unref (event->source);
  g_free (event->type);
  g_value_unset (&event->any_data);
  g_free (event);
}

void
_atspi_send_event (AtspiEvent *e)
{
  char *category, *name, *detail;
  GList *l;

  /* Ensure that the value is set to avoid a Python exception */
  /* TODO: Figure out how to do this without using a private field */
  if (e->any_data.g_type == 0)
  {
    g_value_init (&e->any_data, G_TYPE_INT);
    g_value_set_int (&e->any_data, 0);
  }

  if (!convert_event_type_to_dbus (e->type, &category, &name, &detail, NULL))
  {
    g_warning ("Atspi: Couldn't parse event: %s\n", e->type);
    return;
  }
  for (l = event_listeners; l; l = g_list_next (l))
  {
    EventListenerEntry *entry = l->data;
    if (!strcmp (category, entry->category) &&
        (entry->name == NULL || !strcmp (name, entry->name)) &&
        (entry->detail == NULL || !strcmp (detail, entry->detail)))
    {
        entry->callback (atspi_event_copy (e), entry->user_data);
    }
  }
  if (detail) g_free (detail);
  g_free (name);
  g_free (category);
}

DBusHandlerResult
_atspi_dbus_handle_event (DBusConnection *bus, DBusMessage *message, void *data)
{
  char *detail = NULL;
  const char *category = dbus_message_get_interface (message);
  const char *member = dbus_message_get_member (message);
  const char *signature = dbus_message_get_signature (message);
  gchar *name;
  gchar *converted_type;
  DBusMessageIter iter, iter_variant;
  dbus_message_iter_init (message, &iter);
  AtspiEvent e;
  dbus_int32_t detail1, detail2;
  char *p;

  if (strcmp (signature, "siiv(so)") != 0)
  {
    g_warning ("Got invalid signature %s for signal %s from interface %s\n", signature, member, category);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  memset (&e, 0, sizeof (e));

  if (category)
  {
    category = g_utf8_strrchr (category, -1, '.');
    if (category == NULL)
    {
      // TODO: Error
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    category++;
  }
  dbus_message_iter_get_basic (&iter, &detail);
  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &detail1);
  e.detail1 = detail1;
  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &detail2);
  e.detail2 = detail2;
  dbus_message_iter_next (&iter);

  converted_type = convert_name_from_dbus (category);
  name = convert_name_from_dbus (member);
  detail = convert_name_from_dbus (detail);

  if (strcasecmp  (category, name) != 0)
  {
    p = g_strconcat (converted_type, ":", name, NULL);
    g_free (converted_type);
    converted_type = p;
  }
  else if (detail [0] == '\0')
  {
    p = g_strconcat (converted_type, ":",  NULL);
    g_free (converted_type);
    converted_type = p;
  }

  if (detail[0] != '\0')
  {
    p = g_strconcat (converted_type, ":", detail, NULL);
    g_free (converted_type);
    converted_type = p;
  }
  e.type = converted_type;
  e.source = _atspi_ref_accessible (dbus_message_get_sender(message), dbus_message_get_path(message));

  dbus_message_iter_recurse (&iter, &iter_variant);
  switch (dbus_message_iter_get_arg_type (&iter_variant))
  {
    case DBUS_TYPE_STRUCT:
    {
      AtspiRect rect;
      if (demarshal_rect (&iter_variant, &rect))
      {
	g_value_init (&e.any_data, ATSPI_TYPE_RECT);
	g_value_set_boxed (&e.any_data, &rect);
      }
      else
      {
        AtspiAccessible *accessible;
	accessible = _atspi_dbus_return_accessible_from_iter (&iter_variant);
	g_value_init (&e.any_data, ATSPI_TYPE_ACCESSIBLE);
	g_value_set_instance (&e.any_data, accessible);
	g_object_unref (accessible);	/* value now owns it */
      }
      break;
    }
    case DBUS_TYPE_STRING:
    {
      dbus_message_iter_get_basic (&iter_variant, &p);
      g_value_init (&e.any_data, G_TYPE_STRING);
      g_value_set_string (&e.any_data, p);
      break;
    }
  default:
    break;
  }

  if (!strncmp (e.type, "object:children-changed", 23))
  {
    cache_process_children_changed (&e);
  }
  else if (!strncmp (e.type, "object:property-change", 22))
  {
    cache_process_property_change (&e);
  }
  else if (!strncmp (e.type, "object:state-changed", 20))
  {
    cache_process_state_changed (&e);
  }

  _atspi_send_event (&e);

  g_free (converted_type);
  g_free (name);
  g_free (detail);
  g_object_unref (e.source);
  g_value_unset (&e.any_data);
  return DBUS_HANDLER_RESULT_HANDLED;
}

G_DEFINE_BOXED_TYPE (AtspiEvent, atspi_event, atspi_event_copy, atspi_event_free)
