#include "buses.h"
#include "utility.h"
#include "location.h"
#include "persistence.h"

void ListBuses(const Buses* buses) {
#ifndef RELEASE
  APP_LOG(APP_LOG_LEVEL_INFO, "Number of buses:%u", (uint)buses->count);
  for(uint32_t i = 0; i < buses->count; i++)  {
    Bus b = buses->data[i];
    APP_LOG(APP_LOG_LEVEL_INFO,
            "%u - route:%s\troute_id:%s\tstop_id:%s",
            (uint)i, 
            b.route_name, 
            b.route_id, 
            b.stop_id);
  }
#endif
}

void FilterBusesByLocation(const sll lat, const sll lon, Buses* buses) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Filtering buses by location:");
  buses->filter_count = 0;
  FreeAndClearPointer((void**)&buses->filter_index);
  for(uint32_t i = 0; i < buses->count; i++)  {
    Bus b = buses->data[i];
    // distance in KM
    sll d = DistanceBetweenSLL(b.lat, b.lon, lat, lon);

    //TODO / IDEA: make this return at least one stop, 
    //  or search outward from the radius to find some...
    //TODO: make DEFINE or var set in settings, coordinate with JS OBA calls 
    //  w/ radius set. One pattern could be that this distance is 2x, 4x the 
    //  stop search dist?
    if(d < dbl2sll(1.000)) {
      uint32_t* temp = (uint32_t*)malloc(sizeof(uint32_t) * 
                                         (buses->filter_count+1));
      if(temp == NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "NULL FILTER POINTER");
      }
      if(buses->filter_index != NULL) {
        memcpy(temp, buses->filter_index, sizeof(uint32_t)*buses->filter_count);
        free(buses->filter_index);
      }
      buses->filter_index = temp;
      buses->filter_index[buses->filter_count] = i;
      buses->filter_count += 1;
    }
  }
}

static bool CreateBus(const char* route_id, 
                      const char* route_name,
                      const char* description,
                      const char* stop_id,
                      const char* stop_name, 
                      const sll lat,
                      const sll lon, 
                      const char* direction,
                      Buses* buses) {

  APP_LOG(APP_LOG_LEVEL_INFO, 
          "Creating bus - routename:%s, route_id:%s, buses: %p, count: %i",
          route_name,
          route_id,
          buses,
          (int)buses->count);

  // create new bus
  Bus temp_bus;
  StringCopy(temp_bus.route_id, route_id, ID_LENGTH);
  StringCopy(temp_bus.stop_id, stop_id, ID_LENGTH);
  StringCopy(temp_bus.route_name, route_name, ROUTE_SHORT_LENGTH);
  StringCopy(temp_bus.stop_name, stop_name, STOP_LENGTH);
  StringCopy(temp_bus.description, description, DESCRIPTION_LENGTH);
  StringCopy(temp_bus.direction, direction, DIRECTION_LENGTH);
  temp_bus.lat = lat;
  temp_bus.lon = lon;

  if(SaveBusToPersistence(&temp_bus, buses->count)) {
    // add bus to the end of buses
    Bus* temp_buses = (Bus *)malloc(sizeof(Bus)*((buses->count)+1));
    if(temp_buses == NULL) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "NULL TEMP BUSES");
      return false;
    }
    if(buses->data != NULL) {
      memcpy(temp_buses, buses->data, sizeof(Bus)*(buses->count));
      free(buses->data);
    }

    buses->data = temp_buses;
    buses->data[buses->count] = temp_bus;
    buses->count+=1;
    SaveBusCountToPersistence(buses->count);
  }
  else {
    return false;
  }
  return true;
}

bool AddBus(const Bus* bus, Buses* buses) {
  return CreateBus(bus->route_id,
                   bus->route_name,
                   bus->description,
                   bus->stop_id,
                   bus->stop_name,
                   bus->lat,
                   bus->lon,
                   bus->direction,
                   buses);
}

bool AddBusFromStopRoute(const Stop* stop, const Route* route, Buses* buses) {
  return CreateBus(route->route_id,
                   route->route_name,
                   route->description,
                   stop->stop_id,
                   stop->stop_name,
                   stop->lat,
                   stop->lon,
                   stop->direction,
                   buses);
}

int32_t GetBusIndex(
    const char* stop_id, 
    const char* route_id, 
    const Buses* buses) {
      
  for(uint32_t i  = 0; i < buses->count; i++) {
    Bus bus = buses->data[i];
    if((strcmp(bus.stop_id,stop_id) == 0) && 
        (strcmp(bus.route_id, route_id) == 0)) {
      return i;
    }
  }
  return -1;
}

void RemoveBus(uint32_t index, Buses *buses) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Removing bus - index:%u", (uint)index);
  if(buses->count <= 0) return;
  if(buses->count <= index) return;

  // delete persistence 
  DeleteBusFromPersistence(buses, index);
  
  if(buses->count == 1) {
    FreeAndClearPointer((void**)&buses->data);
    buses->count = 0;    
    return;
  }

  Bus* temp_buses = (Bus *)malloc(sizeof(Bus)*((buses->count)-1));
  if(temp_buses == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL BUS POINTER");
  }

  // first part copy
  if(index > 0) {
    memcpy(temp_buses, buses->data, sizeof(Bus)*(index));
  }
  // second part copy
  if((index+1) < buses->count) {
    memcpy(&temp_buses[index],
           &buses->data[index+1], 
           sizeof(Bus)*(buses->count-index-1));
  }

  free(buses->data);
  buses->data = temp_buses;
  buses->count-=1;
}

void AddStop(const char* stop_id,
             const char* stop_name,
             const char* detail_string,
             const sll lat,
             const sll lon,
             const char * direction,
             Stops* stops) {
      
  APP_LOG(APP_LOG_LEVEL_INFO, 
          "Creating stop - stop:%s, name:%s, details:%s",
          stop_id,
          stop_name, 
          detail_string);
  Stop* temp_stops = (Stop *)malloc(sizeof(Stop)*((stops->count)+1));
  if(temp_stops == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL STOP POINTER");
  }
  if(stops->data != NULL) {
    memcpy(temp_stops, stops->data, sizeof(Stop)*(stops->count));
    free(stops->data);
  }
  stops->data = temp_stops;
  Stop temp = StopConstructor(stop_id, 
                              stop_name, 
                              detail_string, 
                              lat, 
                              lon, 
                              direction);
  stops->data[stops->count] = temp;
  stops->count+=1;
}

void AddRoute(const char *route_id,
              const char *routeName,
              const char *stop_id_list,
              const char *description,
              Routes* routes) {
      
  APP_LOG(APP_LOG_LEVEL_INFO, 
          "Creating route - route:%s, name:%s, stops:%s", 
          route_id, 
          routeName, 
          stop_id_list);
  Route* temp_routes = (Route*)malloc(sizeof(Route)*((routes->count)+1));
  if(temp_routes == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "NULL ROUTES POINTER");
  }
  if(routes->data != NULL) {
    memcpy(temp_routes, routes->data, sizeof(Route)*(routes->count));
    free(routes->data);
  }
  routes->data = temp_routes;
  Route temp_route = RouteConstructor(route_id, 
                                      routeName,
                                      stop_id_list, 
                                      description);
  routes->data[routes->count] = temp_route;
  routes->count+=1;
}

Stop StopConstructor(const char* stop_id, 
                     const char* stop_name, 
                     const char* detail_string, 
                     const sll lat, 
                     const sll lon, 
                     const char* direction) {
  
  Stop stop;
  StringCopy(stop.stop_id, stop_id, ID_LENGTH);
  StringCopy(stop.stop_name, stop_name, STOP_LENGTH);
  StringCopy(stop.detail_string, detail_string, DESCRIPTION_LENGTH);
  stop.lat = lat;
  stop.lon = lon;
  StringCopy(stop.direction, direction, DIRECTION_LENGTH);
  
  return stop;
}

void StopsInit(Stops *stops) {
  stops->count = 0;
  stops->data = NULL;
}

void StopsDestructor(Stops *stops) {
  stops->count = 0;
  FreeAndClearPointer((void**)&stops->data);
}

Route RouteConstructor(const char* route_id, 
                       const char* route_name, 
                       const char* stop_id_list, 
                       const char* direction) {

  Route t;
  int i;

  i = strlen(route_id);
  t.route_id = (char *)malloc(sizeof(char)*(i+1));
  StringCopy(t.route_id, route_id, i+1);

  i = strlen(route_name);
  t.route_name = (char *)malloc(sizeof(char)*(i+1));
  StringCopy(t.route_name, route_name, i+1);

  i = strlen(stop_id_list);
  t.stop_id_list = (char *)malloc(sizeof(char)*(i+1));
  StringCopy(t.stop_id_list, stop_id_list, i+1);

  i = strlen(direction);
  t.description = (char *)malloc(sizeof(char)*(i+1));
  StringCopy(t.description, direction, i+1);

  return t;
}

void RouteDestructor(Route *t) {
  FreeAndClearPointer((void**)&t->route_id);
  FreeAndClearPointer((void**)&t->route_name);
  FreeAndClearPointer((void**)&t->stop_id_list);
  FreeAndClearPointer((void**)&t->description);
}

void RoutesDestructor(Routes *r) {
  for(uint32_t i = 0; i < r->count; i++) {
    RouteDestructor(&r->data[i]);
  }
  FreeAndClearPointer((void**)&r->data);
  r->count = 0;
}

void RoutesInit(Routes *r) {
  r->data = NULL;
  r->count = 0;
}