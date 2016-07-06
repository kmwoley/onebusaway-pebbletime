#include <pebble.h>
#include "main.h"
#include "appdata.h"
#include "settings_stops.h"
#include "settings_routes.h"
#include "persistence.h"
#include "communication.h"
#include "main_window.h"
#include "error_window.h"
#include "utility.h"

static AppData s_appdata;

static void BluetoothCallback(bool connected) {
  if(connected) {
    ErrorWindowRemove();
  }
  else {
    ErrorWindowPush(DIALOG_MESSAGE_BLUETOOTH_ERROR, true);
  }
}

static void HandleInit(void) {
  // Upgrade persistence (as needed)
  PersistenceVersionControl();
  
  // Initialize app data
  s_appdata.initialized = false;
  s_appdata.show_settings = true;
  ArrivalsInit(&s_appdata.arrivals);
  LoadBusesFromPersistence(&s_appdata.buses);

  // Initialize app message communication
  CommunicationInit(&s_appdata);
  
  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = BluetoothCallback
  });
  BluetoothCallback(connection_service_peek_pebble_app_connection());

  // Initialize windows
  SettingsStopsInit();
  SettingsRoutesInit();
  MainWindowInit(&s_appdata);
}

static void HandleDeinit(void) {
  SettingsRoutesDeinit();
  SettingsStopsDeinit();
  FreeAndClearPointer((void**)&s_appdata.buses.data);
  FreeAndClearPointer((void**)&s_appdata.buses.filter_index);
  ArrivalsDestructor(&s_appdata.arrivals);
  CommunicationDeinit();
  MainWindowDeinit();
}

void AppExit() {
  // pop all the windows, which triggers the app exit
  window_stack_pop_all(true);
}

int main(void) {
  HandleInit();
  app_event_loop();
  HandleDeinit();
}