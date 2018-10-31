/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2018      Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/errhandler/errhandler.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Session_init = PMPI_Session_init
#endif
#define MPI_Session_init PMPI_Session_init
#endif

static const char FUNC_NAME[] = "MPI_Session_init";


int MPI_Session_init (MPI_Flags *flags, MPI_Info info, MPI_Errhandler errhandler, MPI_Session *session)
{
    int rc;

    if ( MPI_PARAM_CHECK ) {
        if (NULL == errhandler || NULL == flags || NULL == session) {
            return MPI_ERR_ARG;
        }

        if (NULL == info || ompi_info_is_freed (info)) {
            return MPI_ERR_INFO;
        }
    }

    rc = ompi_mpi_instance_init (flags, &info->super, errhandler, session);
    /* if an error occured raise it on the null session */
    OMPI_ERRHANDLER_RETURN (rc, MPI_SESSION_NULL, rc, FUNC_NAME);
}
