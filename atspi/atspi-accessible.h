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

#ifndef _ATSPI_ACCESSIBLE_H_
#define _ATSPI_ACCESSIBLE_H_

#include "glib-object.h"

#include "atspi-application.h"
#include "atspi-constants.h"
#include "atspi-object.h"
#include "atspi-stateset.h"
#include "atspi-types.h"

#define ATSPI_TYPE_ACCESSIBLE                        (atspi_accessible_get_type ())
#define ATSPI_ACCESSIBLE(obj)                        (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATSPI_TYPE_ACCESSIBLE, AtspiAccessible))
#define ATSPI_ACCESSIBLE_CLASS(klass)                (G_TYPE_CHECK_CLASS_CAST ((klass), ATSPI_TYPE_ACCESSIBLE, AtspiAccessibleClass))
#define ATSPI_IS_ACCESSIBLE(obj)                     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATSPI_TYPE_ACCESSIBLE))
#define ATSPI_IS_ACCESSIBLE_CLASS(klass)             (G_TYPE_CHECK_CLASS_TYPE ((klass), ATSPI_TYPE_ACCESSIBLE))
#define ATSPI_ACCESSIBLE_GET_CLASS(obj)              (G_TYPE_INSTANCE_GET_CLASS ((obj), ATSPI_TYPE_ACCESSIBLE, AtspiAccessibleClass))

struct _AtspiAccessible
{
  AtspiObject parent;
  AtspiAccessible *accessible_parent;
  GList *children;
  AtspiRole role;
  gint interfaces;
  char *name;
  char *description;
  AtspiStateSet *states;
  GHashTable *attributes;
  guint cached_properties;
};

typedef struct _AtspiAccessibleClass AtspiAccessibleClass;
struct _AtspiAccessibleClass
{
  AtspiObjectClass parent_class;
};

GType atspi_accessible_get_type (void); 

AtspiAccessible *
_atspi_accessible_new (AtspiApplication *app, const gchar *path);

gchar * atspi_role_get_name (AtspiRole role);

gchar * atspi_accessible_get_name (AtspiAccessible *obj, GError **error);

gchar * atspi_accessible_get_description (AtspiAccessible *obj, GError **error);

AtspiAccessible * atspi_accessible_get_parent (AtspiAccessible *obj, GError **error);

gint atspi_accessible_get_child_count (AtspiAccessible *obj, GError **error);

AtspiAccessible * atspi_accessible_get_child_at_index (AtspiAccessible *obj, gint    child_index, GError **error);

gint atspi_accessible_get_index_in_parent (AtspiAccessible *obj, GError **error);

GArray * atspi_accessible_get_relation_set (AtspiAccessible *obj, GError **error);

AtspiRole atspi_accessible_get_role (AtspiAccessible *obj, GError **error);

gchar * atspi_accessible_get_role_name (AtspiAccessible *obj, GError **error);

gchar * atspi_accessible_get_localized_role_name (AtspiAccessible *obj, GError **error);

AtspiStateSet * atspi_accessible_get_state_set (AtspiAccessible *obj);

GHashTable * atspi_accessible_get_attributes (AtspiAccessible *obj, GError **error);

GArray * atspi_accessible_get_attributes_as_array (AtspiAccessible *obj, GError **error);

gchar * atspi_accessible_get_toolkit_name (AtspiAccessible *obj, GError **error);

gchar * atspi_accessible_get_toolkit_version (AtspiAccessible *obj, GError **error);

gchar * atspi_accessible_get_atspi_version (AtspiAccessible *obj, GError **error);

gint atspi_accessible_get_id (AtspiAccessible *obj, GError **error);

AtspiAccessible * atspi_accessible_get_application (AtspiAccessible *obj, GError **error);

AtspiAction * atspi_accessible_get_action (AtspiAccessible *obj);

AtspiCollection * atspi_accessible_get_collection (AtspiAccessible *obj);

AtspiComponent * atspi_accessible_get_component (AtspiAccessible *obj);

AtspiDocument * atspi_accessible_get_document (AtspiAccessible *obj);

AtspiEditableText * atspi_accessible_get_editable_text (AtspiAccessible *obj);

AtspiHyperlink * atspi_accessible_get_hyperlink (AtspiAccessible *obj);

AtspiHypertext * atspi_accessible_get_hypertext (AtspiAccessible *obj);

AtspiImage * atspi_accessible_get_image (AtspiAccessible *obj);

AtspiSelection * atspi_accessible_get_selection (AtspiAccessible *obj);

AtspiTable * atspi_accessible_get_table (AtspiAccessible *obj);

AtspiText * atspi_accessible_get_text (AtspiAccessible *obj);

AtspiValue * atspi_accessible_get_value (AtspiAccessible *obj);

GArray * atspi_accessible_get_interfaces (AtspiAccessible *obj);

void atspi_accessible_set_cache_mask (AtspiAccessible *accessible, AtspiCache mask);

void atspi_accessible_clear_cache (AtspiAccessible *accessible);

guint atspi_accessible_get_process_id (AtspiAccessible *accessible, GError **error);

/* private */
void _atspi_accessible_add_cache (AtspiAccessible *accessible, AtspiCache flag);
AtspiCache _atspi_accessible_get_cache_mask (AtspiAccessible *accessible);
gboolean _atspi_accessible_test_cache (AtspiAccessible *accessible, AtspiCache flag);
#endif	/* _ATSPI_ACCESSIBLE_H_ */
