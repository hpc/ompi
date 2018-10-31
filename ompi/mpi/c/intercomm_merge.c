/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2006-2009 University of Houston.  All rights reserved.
 * Copyright (c) 2010-2012 Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2012-2013 Inria.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Los Alamos National Security, LLC. All rights
 *                         reserved.
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

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/communicator/communicator.h"
#include "ompi/proc/proc.h"
#include "ompi/memchecker.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Intercomm_merge = PMPI_Intercomm_merge
#endif
#define MPI_Intercomm_merge PMPI_Intercomm_merge
#endif

static const char FUNC_NAME[] = "MPI_Intercomm_merge";


int MPI_Intercomm_merge(MPI_Comm intercomm, int high,
                        MPI_Comm *newcomm)
{
    ompi_group_t *new_group_pointer;
    ompi_communicator_t *newcomp;
    int first, rc, thigh = high;
    char tag[128];

    MEMCHECKER(
        memchecker_comm(intercomm);
    );

    if ( MPI_PARAM_CHECK ) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);

        if (ompi_comm_invalid ( intercomm ) ||
             !( intercomm->c_flags & OMPI_COMM_INTER ) )
            return OMPI_ERRHANDLER_INVOKE ( MPI_COMM_WORLD, MPI_ERR_COMM,
                                            FUNC_NAME);

        if ( NULL == newcomm )
            return OMPI_ERRHANDLER_INVOKE ( intercomm, MPI_ERR_ARG,
                                            FUNC_NAME);
    }

#if OPAL_ENABLE_FT_MPI
    /*
     * An early check, so as to return early if we are using a broken
     * communicator. This is not absolutely necessary since we will
     * check for this, and other, error conditions during the operation.
     */
    if( OPAL_UNLIKELY(!ompi_comm_iface_create_check(intercomm, &rc)) ) {
        OMPI_ERRHANDLER_RETURN(rc, intercomm, rc, FUNC_NAME);
    }
#endif

    first = ompi_comm_determine_first ( intercomm, thigh );
    if ( MPI_UNDEFINED == first ) {
        return OMPI_ERRHANDLER_INVOKE(intercomm, MPI_ERR_INTERN, FUNC_NAME);
    }

    if ( first ) {
        ompi_group_union ( intercomm->c_local_group, intercomm->c_remote_group, &new_group_pointer );
    }
    else {
        ompi_group_union ( intercomm->c_remote_group, intercomm->c_local_group, &new_group_pointer );
    }

    /* NTH: the merge can easily be done with create_from_group. no reason not to unless we want
     * to try and optimize the extended CID space (there are 2^128 possible extended CIDs) */
    snprintf (tag, sizeof (tag), "OMPIi_ICM_%s::%s", ompi_comm_print_cid (intercomm),
              OPAL_NAME_PRINT(ompi_group_get_proc_name (new_group_pointer, 0)));

    rc = ompi_comm_create_from_group (new_group_pointer, tag, &ompi_mpi_info_null.info.super,
                                      intercomm->error_handler, &newcomp);

    ompi_group_free (&new_group_pointer);

    if (MPI_SUCCESS != rc) {
        *newcomm = MPI_COMM_NULL;
        return OMPI_ERRHANDLER_INVOKE(intercomm, rc,  FUNC_NAME);
    }

    *newcomm = newcomp;
    return MPI_SUCCESS;
}

