/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2002 Ximian, Inc.
 *           2002 Sun Microsystems Inc.
 *           
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

#ifndef _ATSPI_DOCUMENT_H_
#define _ATSPI_DOCUMENT_H_

#include "glib-object.h"

#include "atspi-constants.h"

#include "atspi-types.h"

#define ATSPI_TYPE_DOCUMENT                    (atspi_document_get_type ())
#define ATSPI_IS_DOCUMENT(obj)                 G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATSPI_TYPE_DOCUMENT)
#define ATSPI_DOCUMENT(obj)                    G_TYPE_CHECK_INSTANCE_CAST ((obj), ATSPI_TYPE_DOCUMENT, AtspiDocument)
#define ATSPI_DOCUMENT_GET_IFACE(obj)          (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATSPI_TYPE_DOCUMENT, AtspiDocument))

GType atspi_document_get_type ();

struct _AtspiDocument
{
  GTypeInterface parent;
};

gchar * atspi_document_get_locale (AtspiDocument *obj, GError **error);

gchar * atspi_document_get_attribute_value (AtspiDocument *obj, gchar *attribute, GError **error);

GHashTable * atspi_document_get_attributes (AtspiDocument *obj, GError **error);

#endif	/* _ATSPI_DOCUMENT_H_ */
