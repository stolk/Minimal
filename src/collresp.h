#include "ode/ode.h"
#include "categories.h"

extern void collresp_respond( dGeomID g0, dGeomID g1, int num_contacts, dContact* contacts );

extern void collresp_init( dWorldID world, dJointGroupID contactgroup );

