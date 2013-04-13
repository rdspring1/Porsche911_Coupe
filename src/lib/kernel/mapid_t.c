#include "mapid_t.h"

mapid_t_vals init(mapid_t_vals mtv)
{
    mtv.next_free = 0;
    mtv.id = num_procs;
    num_procs++;
    return mtv;
}

mapid_t_vals new_mapping(mapid_t_vals mtv)
{
    struct mapid_t new_map;
    new_map.id = mtv.next_free;
    mtv.mappings[mtv.next_free] = new_map;
    mtv.next_free++;
    return mtv;
}
