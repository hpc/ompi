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
#include "ompi/instance/instance.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Session_get_psetlen = PMPI_Session_get_psetlen
#endif
#define MPI_Session_get_psetlen PMPI_Session_get_psetlen
#endif

static const char FUNC_NAME[] = "MPI_Session_get_psetlen";

int MPI_Session_get_psetlen (MPI_Session session, int n, int *pset_name_len)
{
    int rc;

    if ( MPI_PARAM_CHECK ) {
        if (NULL == session || NULL == pset_name_len || n < 0) {
            return OMPI_ERRHANDLER_INVOKE(session, MPI_ERR_ARG, FUNC_NAME);
        }
    }

    rc = ompi_instance_get_psetlen (session, n, pset_name_len);
    /* if an error occured raise it on the null session */
    OMPI_ERRHANDLER_RETURN (rc, MPI_SESSION_NULL, rc, FUNC_NAME);
}
