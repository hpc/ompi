/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2018 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2012-2013 Inria.  All rights reserved.
 * Copyright (c) 2013      Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2017      IBM Corporation. All rights reserved.
 * Copyright (c) 2018      Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 */

#include "ompi_config.h"

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/memchecker.h"
#include "ompi/mca/topo/topo.h"
#include "ompi/mca/topo/base/base.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Dist_graph_create_from_group = PMPI_Dist_graph_create_from_group
#endif
#define MPI_Dist_graph_create_from_group PMPI_Dist_graph_create_from_group
#endif

static const char FUNC_NAME[] = "MPI_Dist_graph_create_from_group";

int MPI_Dist_graph_create_from_group (MPI_Group group, const char *tag, MPI_Errhandler errhandler,
                                      int n, const int sources[], const int degrees[], const int destinations[],
                                      const int weights[], MPI_Info info, int reorder, MPI_Comm * newcomm)
{
    mca_topo_base_module_t* topo;
    int index, err, group_size;

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
        if (n < 0 || NULL == newcomm || (n > 0 && (NULL == sources || NULL == degrees ||
                                                   NULL == destinations || NULL == weights))) {
            return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                           MPI_ERR_ARG, FUNC_NAME);
        }

        /* Ensure the arrays are full of valid-valued integers */
        group_size = ompi_group_size (group);
        for (int i = index = 0 ; i < n ; ++i) {
            if (((sources[i] < 0) && (sources[i] != MPI_PROC_NULL)) || sources[i] >= group_size || degrees[i] < 0) {
                return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                               MPI_ERR_ARG, FUNC_NAME);
            }
            for (int j = 0 ; j < degrees[i] ; ++j) {
                if (((destinations[index] < 0) && (destinations[index] != MPI_PROC_NULL)) || destinations[index] >= group_size ||
                    (MPI_UNWEIGHTED != weights && weights[index] < 0)) {
                    return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                                   MPI_ERR_ARG, FUNC_NAME);
                }
                index++;
            }
        }
    }

    /* Ensure there is a topo attached to this communicator */
    if(OMPI_SUCCESS != (err = mca_topo_base_group_select (group, NULL, &topo, OMPI_COMM_DIST_GRAPH))) {
        return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                       err, FUNC_NAME);
    }

    err = topo->topo.dist_graph.dist_graph_create_from_group (topo, group, tag, errhandler, n, sources, degrees,
                                                              destinations, weights, &(info->super),
                                                              reorder, newcomm);
    OMPI_ERRHANDLER_RETURN(err, MPI_COMM_SELF, err, FUNC_NAME);
}

