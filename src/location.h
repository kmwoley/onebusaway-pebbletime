#ifndef LOCATION_H
#define LOCATION_H

#include <pebble.h>
#include "math-sll.h"

sll DistanceBetweenSLL(sll lat1, sll lon1, sll lat2, sll lon2);
double DistanceBetween(double lat1, double lon1, double lat2, double lon2);

#endif //LOCATION_H
