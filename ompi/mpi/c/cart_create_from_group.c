/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2013 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2008 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012-2013 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2012-2013 Inria.  All rights reserved.
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
#include "ompi/mca/topo/base/base.h"
#include "ompi/memchecker.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Cart_create_from_group = PMPI_Cart_create_from_group
#endif
#define MPI_Cart_create_from_group PMPI_Cart_create_from_group
#endif

static const char FUNC_NAME[] = "MPI_Cart_create_from_group";


int MPI_Cart_create_from_group (MPI_Group group, const char *tag, MPI_Info info,
                                MPI_Errhandler errhandler, int ndims, const int dims[],
                                const int periods[], int reorder, MPI_Comm *comm_cart)
{
    mca_topo_base_module_t *topo;
    int err;

    /* check the arguments */
    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (NULL == group) {
            return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                           MPI_ERR_GROUP, FUNC_NAME);
        }
        if (NULL == tag) {
            return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                           MPI_ERR_TAG, FUNC_NAME);
        }
        if (NULL == info || ompi_info_is_freed(info)) {
            return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                           MPI_ERR_INFO, FUNC_NAME);
        }
        if (ndims < 0 || (ndims >= 1 && (NULL == dims || NULL == periods)) || NULL == comm_cart) {
            return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                           MPI_ERR_ARG, FUNC_NAME);
        }

        /* check if the number of processes on the grid are correct */
        {
           int count_nodes = 1;

           for (int i = 0 ; i < ndims ; i++) {
               count_nodes *= dims[i];
           }

           if (group->grp_proc_count < count_nodes) {
               return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                              MPI_ERR_ARG, FUNC_NAME);
           }
        }
    }

    if (MPI_UNDEFINED == group->grp_my_rank) {
        *comm_cart = MPI_COMM_NULL;
        return MPI_SUCCESS;
    }

    /*
     * everything seems to be alright with the communicator, we can go
     * ahead and select a topology module for this purpose and create
     * the new graph communicator
     */
    if (OMPI_SUCCESS != (err = mca_topo_base_group_select (group, NULL, &topo, OMPI_COMM_CART))) {
        return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                       err, FUNC_NAME);
    }

    /* Now let that topology module rearrange procs/ranks if it wants to */
    err = topo->topo.cart.cart_create_from_group (topo, group, tag, &info->super, errhandler,
                                                  ndims, dims, periods, !!reorder,
                                                  comm_cart);
    OPAL_CR_EXIT_LIBRARY();

    if (MPI_SUCCESS != err) {
        OBJ_RELEASE(topo);
        return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                       err, FUNC_NAME);
    }

    /* All done */
    return MPI_SUCCESS;
}
