/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2009 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2018      Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include <string.h>

#include "opal/util/output.h"

#include "mpi.h"
#include "ompi/constants.h"
#include "coll_lazy.h"

int mca_coll_lazy_barrier (struct ompi_communicator_t *comm, mca_coll_base_module_t *module)
{
    mca_coll_lazy_module_t *lazy_module = (mca_coll_lazy_module_t *) module;
    ompi_request_t *request;

    comm->c_coll->coll_ibarrier (comm, &request, comm->c_coll->coll_ibarrier_module);
    do {
        for (int i = 0 ; i < 10 ; ++i) {
            opal_progress ();
            if (REQUEST_COMPLETE(request)) {
                break;
            }
        }

        if (REQUEST_COMPLETE(request)) {
            break;
        }

        nanosleep (&(struct timespec) {.tv_sec = 0, .tv_nsec = 10000}, NULL);
    } while (1);

    return OMPI_SUCCESS;
}
