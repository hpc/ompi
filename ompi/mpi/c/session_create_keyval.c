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
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/attribute/attribute.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Session_create_keyval = PMPI_Session_create_keyval
#endif
#define MPI_Session_create_keyval PMPI_Session_create_keyval
#endif

/* static const char FUNC_NAME[] = "MPI_Session_create_keyval"; */


int MPI_Session_create_keyval (MPI_Session_delete_attr_function *session_delete_attr_fn, int *session_keyval,
                               void *extra_state)
{
    int ret;
    ompi_attribute_fn_ptr_union_t del_fn;

    if (MPI_PARAM_CHECK) {
        if (NULL == session_delete_attr_fn || NULL == session_keyval) {
            return MPI_ERR_ARG;
        }
    }

    OPAL_CR_ENTER_LIBRARY();

    del_fn.attr_instance_delete_fn = session_delete_attr_fn;

    ret = ompi_attr_create_keyval (INSTANCE_ATTR, (ompi_attribute_fn_ptr_union_t){.attr_communicator_copy_fn = NULL},
                                   del_fn, session_keyval, extra_state, 0, NULL);
    return ompi_errcode_get_mpi_code (ret);
}
