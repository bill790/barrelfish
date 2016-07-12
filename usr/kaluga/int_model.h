#ifndef INT_MODEL_H_
#define INT_MODEL_H_

#include "stdint.h"
#include <errors/errno.h>

/* The interrupt models a driver might use.
 * device_db.pl contains the information what driver wants to use which model.
 */
enum int_model {
    INT_MODEL_LEGACY = 0,
    INT_MODEL_MSI = 1,
    INT_MODEL_MSIX = 2
};

/*
 * This is the struct that contains information regarding the int routing that
 * must be passed on to the driver
 */
struct int_startup_argument {
    enum int_model model;
    uint64_t int_range_start;
    uint64_t int_range_end;
};

errval_t int_startup_argument(struct int_startup_argument * arg, char ** out);


#endif /*INT_MODEL_H_ */
