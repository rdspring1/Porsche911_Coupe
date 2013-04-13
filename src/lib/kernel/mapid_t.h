#include "userprog/fdt.h"

struct mapid_t
  {
    int id;
    void* start_address;
    int file_size;
    int fd;
  };

struct mapid_t_vals
  {
    mapid_t mappings[FDT_MAX_FILES];
    int id;
    int next_free;
  };

mapid_t_vals init(mapid_t_vals);
mapid_t_vals new_mapping(mapid_t_vals);
static int num_procs = -1;
