/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2018-2020 Triad National Security, LLC. All rights
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


int MPI_Session_init (MPI_Info info, MPI_Errhandler errhandler, MPI_Session *session)
{
    int rc, flag;
    int ts_level = MPI_THREAD_SINGLE;  /* for now we default to thread single for OMPI sessions */
    char info_value[MPI_MAX_INFO_VAL + 1];
    const char ts_level_multi[] = "MPI_THREAD_MULTIPLE";

    if ( MPI_PARAM_CHECK ) {
        if (NULL == errhandler || NULL == session) {
            return MPI_ERR_ARG;
        }

        if (NULL == info || ompi_info_is_freed (info)) {
            return MPI_ERR_INFO;
        }
    }

    if (MPI_INFO_NULL != info) {
        (void) ompi_info_get (info, "mpi_thread_support_level", MPI_MAX_INFO_VAL, info_value, &flag);
        if (flag) {
            if(strncmp(info_value, ts_level_multi, strlen(ts_level_multi)) == 0) {
                ts_level = MPI_THREAD_MULTIPLE;
            }
        }
    }

    rc = ompi_mpi_instance_init (ts_level, &info->super, errhandler, session);
    /* if an error occured raise it on the null session */
    OMPI_ERRHANDLER_RETURN (rc, MPI_SESSION_NULL, rc, FUNC_NAME);
}
