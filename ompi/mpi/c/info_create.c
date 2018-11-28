/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2020 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2018      Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/info/info.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Info_create = PMPI_Info_create
#endif
#define MPI_Info_create PMPI_Info_create
#endif

static const char FUNC_NAME[] = "MPI_Info_create";

/**
 * Create a new info object
 *
 * @param info Pointer to the MPI_Info handle
 *
 * @retval MPI_SUCCESS
 * @retval MPI_ERR_INFO
 * @retval MPI_ERR_NO_MEM
 *
 * When an MPI_Info object is not being used, it should be freed using
 * MPI_Info_free
 */
int MPI_Info_create(MPI_Info *info)
{
    int rc;
    OPAL_CR_NOOP_PROGRESS();

    if (MPI_PARAM_CHECK) {
        if (NULL == info) {
            return OMPI_ERRHANDLER_NOHANDLE_INVOKE(MPI_ERR_INFO,
                                          FUNC_NAME);
        }
    }

    rc = ompi_mpi_instance_retain ();
    if (OPAL_UNLIKELY(OMPI_SUCCESS != rc)) {
        /* NTH: seriously, what can we do other than abort () or return? we failed to
         * set up the most basic infrastructure! */
        return rc;
    }

    /*
     * Call the object create function. This function not only
     * allocates the space for MPI_Info, but also calls all the
     * relevant init functions. Should I check if the fortran
     * handle is valid
     */
    (*info) = OBJ_NEW(ompi_info_t);
    if (NULL == (*info)) {
        return OMPI_ERRHANDLER_NOHANDLE_INVOKE(MPI_ERR_NO_MEM,
                                      FUNC_NAME);
    }

    return MPI_SUCCESS;
}
