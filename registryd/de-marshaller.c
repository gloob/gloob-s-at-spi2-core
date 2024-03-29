/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2008 Novell, Inc.
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

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus.h>

#include "de-types.h"

dbus_bool_t spi_dbus_message_iter_get_struct(DBusMessageIter *iter, ...)
{
  va_list args;
  DBusMessageIter iter_struct;
  int type;
  void *ptr;

  dbus_message_iter_recurse(iter, &iter_struct);
  va_start(args, iter);
  for (;;)
  {
    type = va_arg(args, int);
    if (type == DBUS_TYPE_INVALID) break;
    if (type != dbus_message_iter_get_arg_type(&iter_struct))
    {
      va_end(args);
      return FALSE;
    }
    ptr = va_arg(args, void *);
    dbus_message_iter_get_basic(&iter_struct, ptr);
    dbus_message_iter_next(&iter_struct);
  }
  dbus_message_iter_next(iter);
  va_end(args);
  return TRUE;
}

dbus_bool_t spi_dbus_message_iter_append_struct(DBusMessageIter *iter, ...)
{
  va_list args;
  DBusMessageIter iter_struct;
  int type;
  void *ptr;

  if (!dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, NULL, &iter_struct)) return FALSE;
  va_start(args, iter);
  for (;;)
  {
    type = va_arg(args, int);
    if (type == DBUS_TYPE_INVALID) break;
    ptr = va_arg(args, void *);
    dbus_message_iter_append_basic(&iter_struct, type, ptr);
  }
  if (!dbus_message_iter_close_container(iter, &iter_struct)) return FALSE;
  va_end(args);
  return TRUE;
}

dbus_bool_t spi_dbus_marshal_deviceEvent(DBusMessage *message, const Accessibility_DeviceEvent *e)
{
  DBusMessageIter iter;

  if (!message) return FALSE;
  dbus_message_iter_init_append(message, &iter);
  return spi_dbus_message_iter_append_struct(&iter, DBUS_TYPE_UINT32, &e->type, DBUS_TYPE_INT32, &e->id, DBUS_TYPE_UINT32, &e->hw_code, DBUS_TYPE_UINT32, &e->modifiers, DBUS_TYPE_INT32, &e->timestamp, DBUS_TYPE_STRING, &e->event_string, DBUS_TYPE_BOOLEAN, &e->is_text, DBUS_TYPE_INVALID);
}

dbus_bool_t spi_dbus_demarshal_deviceEvent(DBusMessage *message, Accessibility_DeviceEvent *e)
{
  DBusMessageIter iter;
  dbus_uint16_t hw_code;
  dbus_uint16_t modifiers;

  dbus_message_iter_init(message, &iter);
  if (spi_dbus_message_iter_get_struct(&iter, DBUS_TYPE_UINT32, &e->type, DBUS_TYPE_INT32, &e->id, DBUS_TYPE_INT32, &e->hw_code, DBUS_TYPE_INT32, &e->modifiers, DBUS_TYPE_INT32, &e->timestamp, DBUS_TYPE_STRING, &e->event_string, DBUS_TYPE_BOOLEAN, &e->is_text, DBUS_TYPE_INVALID))
    return TRUE;
  /* TODO: Perhaps remove the below code for 2.1 */
  if (!spi_dbus_message_iter_get_struct(&iter, DBUS_TYPE_UINT32, &e->type, DBUS_TYPE_INT32, &e->id, DBUS_TYPE_INT16, &hw_code, DBUS_TYPE_INT16, &modifiers, DBUS_TYPE_INT32, &e->timestamp, DBUS_TYPE_STRING, &e->event_string, DBUS_TYPE_BOOLEAN, &e->is_text, DBUS_TYPE_INVALID))
    return FALSE;
  e->hw_code = hw_code;
  e->modifiers = modifiers;
  return TRUE;
}

/*
 * This is a rather annoying function needed to replace
 * NULL values of strings with the empty string. Null string
 * values can be created by the atk_object_get_name or text selection
 */
static const void *
provide_defaults(const gint type,
		 const void *val)
{
  switch (type)
    {
      case DBUS_TYPE_STRING:
      case DBUS_TYPE_OBJECT_PATH:
	   if (!val)
	      return "";
	   else
	      return val;
      default:
	   return val;
    }
}
