/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
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
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/attribute/attribute.h"
#include "ompi/memchecker.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Session_set_attr = PMPI_Session_set_attr
#endif
#define MPI_Session_set_attr PMPI_Session_set_attr
#endif

static const char FUNC_NAME[] = "MPI_Session_set_attr";

int MPI_Session_set_attr (MPI_Session session, int session_keyval, void *attribute_val)
{
    int ret;

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (NULL == session || MPI_SESSION_NULL == session) {
            return OMPI_ERRHANDLER_INVOKE(MPI_SESSION_NULL, MPI_ERR_ARG, FUNC_NAME);
        }
    }

    OPAL_CR_ENTER_LIBRARY();

    ret = ompi_attr_set_c (INSTANCE_ATTR, session, &session->i_keyhash,
                           session_keyval, attribute_val, false);
    OMPI_ERRHANDLER_RETURN(ret, session, MPI_ERR_OTHER, FUNC_NAME);
}
