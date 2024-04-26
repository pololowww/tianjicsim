#include <fstream>
#include <iostream>
#include <cassert>
#include <string.h>
#include <cmath>
#include <random>
#include <set>
#include "../json/json.h"
#include "globals.hpp"
#include "booksim_config.hpp"



int idcast(int x, int y);
void read_json( BookSimConfig const & config);
void read_csv(BookSimConfig const & config);
