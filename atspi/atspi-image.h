/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2002 Ximian, Inc.
 *           2002 Sun Microsystems Inc.
 * Copyright 2010, 2011 Novell, Inc.
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

#ifndef _ATSPI_IMAGE_H_
#define _ATSPI_IMAGE_H_

#include "glib-object.h"

#include "atspi-constants.h"

#include "atspi-types.h"

#define ATSPI_TYPE_IMAGE                    (atspi_image_get_type ())
#define ATSPI_IS_IMAGE(obj)                 G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATSPI_TYPE_IMAGE)
#define ATSPI_IMAGE(obj)                    G_TYPE_CHECK_INSTANCE_CAST ((obj), ATSPI_TYPE_IMAGE, AtspiImage)
#define ATSPI_IMAGE_GET_IFACE(obj)          (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATSPI_TYPE_IMAGE, AtspiImage))

GType atspi_image_get_type ();

struct _AtspiImage
{
  GTypeInterface parent;
};

gchar * atspi_image_get_image_description (AtspiImage *obj, GError **error);

AtspiPoint * atspi_image_get_image_size (AtspiImage *obj, GError **error);

AtspiPoint * atspi_image_get_image_position (AtspiImage *obj, AtspiCoordType ctype, GError **error);

AtspiRect * atspi_image_get_image_extents (AtspiImage *obj, AtspiCoordType ctype, GError **error);

gchar * atspi_image_get_image_locale  (AtspiImage *obj, GError **error);
#endif	/* _ATSPI_IMAGE_H_ */
