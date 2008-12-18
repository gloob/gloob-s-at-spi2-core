#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <droute/droute.h>

#include "dbus/dbus-glib-lowlevel.h"

#define TEST_OBJECT_PATH    "/test/object"
#define TEST_INTERFACE_ONE  "test.interface.One"
#define TEST_INTERFACE_TWO  "test.interface.Two"

#define OBJECT_ONE "ObjectOne";
#define OBJECT_TWO "ObjectTwo";

#if !defined TEST_INTROSPECTION_DIRECTORY
    #error "No introspection XML directory defined"
#endif

#define STRING_ONE "StringOne"
#define STRING_TWO "StringTwo"

#define INT_ONE 0
#define INT_TWO 456

#define NONE_REPLY_STRING "NoneMethod"

typedef struct _AnObject
{
    gchar *astring;
    guint *anint;
} AnObject;

static DBusConnection *bus;
static GMainLoop      *main_loop;
static gboolean       success = TRUE;

static DBusMessage *
impl_null (DBusConnection *bus, DBusMessage *message, void *user_data)
{
    AnObject    *object = (AnObject *) user_data;
    DBusMessage *reply;
    DBusError    error;

    reply = dbus_message_new_method_return (message);
    return reply;
}

static DBusMessage *
impl_getInt (DBusConnection *bus, DBusMessage *message, void *user_data)
{
    AnObject    *object = (AnObject *) user_data;
    DBusMessage *reply;
    DBusError    error;

    dbus_error_init (&error);

    reply = dbus_message_new_method_return (message);
    dbus_message_append_args (reply, DBUS_TYPE_INT32, &(object->anint), DBUS_TYPE_INVALID);
    return reply;
}

static DBusMessage *
impl_setInt (DBusConnection *bus, DBusMessage *message, void *user_data)
{
    AnObject    *object = (AnObject *) user_data;
    DBusMessage *reply;
    DBusError    error;

    dbus_error_init (&error);

    dbus_message_get_args (message, &error, DBUS_TYPE_INT32, &(object->anint), DBUS_TYPE_INVALID);

    reply = dbus_message_new_method_return (message);
    return reply;
}

static DBusMessage *
impl_getString (DBusConnection *bus, DBusMessage *message, void *user_data)
{
    AnObject    *object = (AnObject *) user_data;
    DBusMessage *reply;
    DBusError    error;

    dbus_error_init (&error);

    reply = dbus_message_new_method_return (message);
    dbus_message_append_args (reply, DBUS_TYPE_STRING, &(object->astring), DBUS_TYPE_INVALID);
    return reply;
}

static DBusMessage *
impl_setString (DBusConnection *bus, DBusMessage *message, void *user_data)
{
    AnObject    *object = (AnObject *) user_data;
    DBusMessage *reply;
    DBusError    error;

    dbus_error_init (&error);

    g_free (object->astring);
    dbus_message_get_args (message, &error, DBUS_TYPE_STRING, &(object->astring), DBUS_TYPE_INVALID);

    reply = dbus_message_new_method_return (message);
    return reply;
}

static DBusMessage *
impl_getInterfaceOne (DBusConnection *bus, DBusMessage *message, void *user_data)
{
    AnObject    *object = (AnObject *) user_data;
    DBusMessage *reply;
    DBusError    error;
    gchar       *itf = TEST_INTERFACE_ONE;

    dbus_error_init (&error);

    reply = dbus_message_new_method_return (message);
    dbus_message_append_args (reply, DBUS_TYPE_STRING, &itf, DBUS_TYPE_INVALID);
    return reply;
}

static DBusMessage *
impl_getInterfaceTwo (DBusConnection *bus, DBusMessage *message, void *user_data)
{
    AnObject    *object = (AnObject *) user_data;
    DBusMessage *reply;
    DBusError    error;
    gchar       *itf = TEST_INTERFACE_TWO;

    dbus_error_init (&error);

    reply = dbus_message_new_method_return (message);
    dbus_message_append_args (reply, DBUS_TYPE_STRING, &itf, DBUS_TYPE_INVALID);
    return reply;
}

static DRouteMethod test_methods_one[] = {
    {impl_null,            "null"},
    {impl_getInt,          "getInt"},
    {impl_setInt,          "setInt"},
    {impl_getString,       "getString"},
    {impl_setString,       "setString"},
    {impl_getInterfaceOne, "getInterfaceOne"},
    {NULL, NULL}
};

static DRouteMethod test_methods_two[] = {
    {impl_null,            "null"},
    {impl_getInt,          "getInt"},
    {impl_setInt,          "setInt"},
    {impl_getString,       "getString"},
    {impl_setString,       "setString"},
    {impl_getInterfaceTwo, "getInterfaceTwo"},
    {NULL, NULL}
};

static DRouteProperty test_properties[] = {
    {NULL, NULL, NULL}
};

gboolean
do_tests_func (gpointer data)
{
    DBusError  error;
    gchar     *bus_name;

    gchar     *expected_string;
    gchar     *result_string;

    dbus_error_init (&error);
    bus_name = dbus_bus_get_unique_name (bus);

    /* --------------------------------------------------------*/

    dbind_method_call_reentrant (bus,
                                 bus_name,
                                 TEST_OBJECT_PATH,
                                 TEST_INTERFACE_ONE,
                                 "null",
                                 NULL,
                                 "");

    /* --------------------------------------------------------*/

    expected_string = TEST_INTERFACE_ONE;
    result_string = NULL;
    dbind_method_call_reentrant (bus,
                                 bus_name,
                                 TEST_OBJECT_PATH,
                                 TEST_INTERFACE_ONE,
                                 "getInterfaceOne",
                                 NULL,
                                 "=>s",
                                 &result_string);
    if (g_strcmp0(expected_string, result_string))
    {
            g_print ("Failed: reply to getInterfaceOne not as expected\n");
            goto out;
    }

    /* --------------------------------------------------------*/

out:
    g_main_loop_quit (main_loop);
    return FALSE;
}


int main (int argc, char **argv)
{
    DRouteContext  *cnx;
    DRoutePath     *path;
    AnObject       *object;
    DBusError       error;

    /* Setup some server object */

    object = g_new0(AnObject, 1);
    object->astring = g_strdup (STRING_ONE);
    object->anint = INT_ONE;

    dbus_error_init (&error);
    main_loop = g_main_loop_new(NULL, FALSE);
    bus = dbus_bus_get (DBUS_BUS_SESSION, &error);
    dbus_connection_setup_with_g_main(bus, g_main_context_default());

    cnx = droute_new (bus, TEST_INTROSPECTION_DIRECTORY);
    path = droute_add_one (cnx, TEST_OBJECT_PATH, object);

    droute_path_add_interface (path,
                               TEST_INTERFACE_ONE,
                               test_methods_one,
                               test_properties);

    droute_path_add_interface (path,
                               TEST_INTERFACE_TWO,
                               test_methods_two,
                               test_properties);

    g_idle_add (do_tests_func, NULL);
    g_main_run(main_loop);
    if (success)
            return 0;
    else
            return 1;
}