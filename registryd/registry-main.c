/*
 * AT-SPI - Assistive Technology Service Provider Interface
 * (Gnome Accessibility Project; http://developer.gnome.org/projects/gap)
 *
 * Copyright 2001, 2002 Sun Microsystems Inc.,
 * Copyright 2001, 2002 Ximian, Inc.
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
#include <config.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <dlfcn.h>

#include <dbus/dbus.h>

#include "paths.h"
#include "registry.h"
#include "deviceeventcontroller.h"
#include "atspi/atspi.h"

static GMainLoop *mainloop;
static gchar *dbus_name = NULL;
static gboolean use_gnome_session = FALSE;

static GOptionEntry optentries[] =
{
  {"dbus-name", 0, 0, G_OPTION_ARG_STRING, &dbus_name, "Well-known name to register with D-Bus", NULL},
  {"use-gnome-session", 0, 0, G_OPTION_ARG_NONE, &use_gnome_session, "Should register with gnome session manager", NULL},
  {NULL}
};

static GDBusConnection *bus_connection = NULL;
static GDBusProxy      *sm_proxy = NULL;
static char            *client_id = NULL;
static GDBusProxy      *client_proxy = NULL;

#define SM_DBUS_NAME      "org.gnome.SessionManager"
#define SM_DBUS_PATH      "/org/gnome/SessionManager"
#define SM_DBUS_INTERFACE "org.gnome.SessionManager"

#define SM_CLIENT_DBUS_INTERFACE "org.gnome.SessionManager.ClientPrivate"

static void registry_session_init (const char *previous_client_id, const char *exe);

static void
on_session_over (GDBusProxy *proxy,
                 gchar      *sender_name,
                 gchar      *signal_name,
                 GVariant   *parameters,
                 gpointer    user_data)
{
        if (g_strcmp0 (signal_name, "SessionOver") == 0) {
                g_main_loop_quit (mainloop);
        }
}

static gboolean
session_manager_connect (void)
{

        sm_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, 0, NULL,
                                              SM_DBUS_NAME,
                                              SM_DBUS_PATH,
                                              SM_DBUS_INTERFACE, NULL, NULL);

        g_signal_connect (G_OBJECT (sm_proxy), "g-signal",
                          G_CALLBACK (on_session_over), NULL);

        return (sm_proxy != NULL);
}

static void
stop_cb (gpointer data)
{
        g_main_loop_quit (mainloop);
}

static gboolean
end_session_response (gboolean is_okay, const gchar *reason)
{
  GVariant *ret;
        GError *error = NULL;

        if (!reason)
                reason = "";

        ret = g_dbus_proxy_call_sync (client_proxy, "EndSessionResponse",
                                      g_variant_new ("(bs)", is_okay, reason),
                                      0, 1000, NULL, &error);

        if (!ret) {
                g_warning ("Failed to send session response %s", error->message);
                g_error_free (error);
                return FALSE;
        }
        else
                g_variant_unref (ret);

        return TRUE;
}

static void
client_proxy_signal_cb (GDBusProxy *proxy,
                        gchar *sender_name,
                        gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
        if (g_strcmp0 (signal_name, "QueryEndSession") == 0) {
                g_debug ("Got QueryEndSession signal");
                end_session_response (TRUE, NULL);
        } else if (g_strcmp0 (signal_name, "EndSession") == 0) {
                g_debug ("Got EndSession signal");
                end_session_response (TRUE, NULL);
                g_main_loop_quit (mainloop);
        } else if (g_strcmp0 (signal_name, "Stop") == 0) {
                g_debug ("Got Stop signal");
                g_main_loop_quit (mainloop);
        }
}

static gboolean
register_client (void)
{
        GError     *error;
  GVariant *res;
        const char *startup_id;
        const char *app_id;

        startup_id = g_getenv ("DESKTOP_AUTOSTART_ID");
        if (!startup_id)
                startup_id = "";
        app_id = "at-spi-registryd.desktop";

        error = NULL;
        res = g_dbus_proxy_call_sync (sm_proxy,
                                 "RegisterClient",
                                      g_variant_new ("(ss)", app_id,
                                                     startup_id),
                                      0, 1000, NULL, &error);
        if (! res) {
                g_warning ("Failed to register client: %s", error->message);
                g_error_free (error);
                return FALSE;
        }
        g_variant_get (res, "(o)", &client_id);
        g_variant_unref (res);

        client_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, 0, NULL,
                                                  SM_DBUS_NAME,
                                                  client_id,
                                                  SM_CLIENT_DBUS_INTERFACE,
                                                  NULL, NULL);

        g_signal_connect (client_proxy, "g-signal",
                          G_CALLBACK (client_proxy_signal_cb), NULL);

        g_unsetenv ("DESKTOP_AUTOSTART_ID");

        return TRUE;
}

/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/

typedef GObject *(*gconf_client_get_default_t) ();
typedef gboolean (*gconf_client_get_bool_t)(GObject *, const char *, void *);

int
main (int argc, char **argv)
{
  SpiRegistry *registry;
  SpiDEController *dec;

  DBusConnection *bus = NULL;

  GOptionContext *opt;

  GError *err = NULL;
  DBusError error;
  int ret;

  g_type_init();

  /*Parse command options*/
  opt = g_option_context_new(NULL);
  g_option_context_add_main_entries(opt, optentries, NULL);

  if (!g_option_context_parse(opt, &argc, &argv, &err))
      g_error("Option parsing failed: %s\n", err->message);

  if (dbus_name == NULL)
      dbus_name = SPI_DBUS_NAME_REGISTRY;

  dbus_error_init (&error);
  bus = atspi_get_a11y_bus ();
  if (!bus)
  {
    return 0;
  }

  mainloop = g_main_loop_new (NULL, FALSE);
  atspi_dbus_connection_setup_with_g_main(bus, NULL);

  ret = dbus_bus_request_name(bus, dbus_name, DBUS_NAME_FLAG_DO_NOT_QUEUE, &error);
  if (ret == DBUS_REQUEST_NAME_REPLY_EXISTS)
    {
      g_error("Could not obtain D-Bus name - %s\n", dbus_name);
    }
  else
    {
      g_print ("SpiRegistry daemon is running with well-known name - %s\n", dbus_name);
    }

  registry = spi_registry_new (bus);
  dec = spi_registry_dec_new (registry, bus);

  if (use_gnome_session)
    {
      if (!session_manager_connect ())
          g_warning ("Unable to connect to session manager");

      if (!register_client ())
          g_warning ("Unable to register client with session manager");
    }

  g_main_loop_run (mainloop);
  return 0;
}
