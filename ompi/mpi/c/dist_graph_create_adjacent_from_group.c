/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2008      The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2011-2018 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2013-2014 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2012-2013 Inria.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2017      IBM Corporation. All rights reserved.
 * Copyright (c) 2018      Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * Author(s): Torsten Hoefler
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
#pragma weak MPI_Dist_graph_create_adjacent_from_group = PMPI_Dist_graph_create_adjacent_from_group
#endif
#define MPI_Dist_graph_create_adjacent_from_group PMPI_Dist_graph_create_adjacent_from_group
#endif

static const char FUNC_NAME[] = "MPI_Dist_graph_create_adjacent_from_group";


int MPI_Dist_graph_create_adjacent_from_group (MPI_Group group, const char *tag, MPI_Errhandler errhandler,
                                               int indegree, const int sources[], const int sourceweights[],
                                               int outdegree, const int destinations[], const int destweights[],
                                               MPI_Info info, int reorder, MPI_Comm *comm_dist_graph)
{
    mca_topo_base_module_t* topo;
    int group_size, err;

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
        if (indegree < 0 || outdegree < 0 || NULL == comm_dist_graph) {
            return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                           MPI_ERR_ARG, "MPI_Dist_graph_create_adjacent_from_group negative degree");
        } else if ((indegree > 0 && (NULL == sources || NULL == sourceweights)) ||
                   (outdegree > 0 && (NULL == destinations || NULL == destweights))) {
            return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                           MPI_ERR_ARG, "MPI_Dist_graph_create_adjacent_from_group mismatched sources or destinations");
        }

        group_size = ompi_group_size (group);
        for (int i = 0 ; i < indegree ; ++i) {
            if (((sources[i] < 0) && (sources[i] != MPI_PROC_NULL)) || sources[i] >= group_size) {
                return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type, MPI_ERR_ARG,
                                              "MPI_Dist_graph_create_adjacent_from_group invalid sources");
            } else if (MPI_UNWEIGHTED != sourceweights && sourceweights[i] < 0) {
                return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type, MPI_ERR_ARG,
                                              "MPI_Dist_graph_create_adjacent_from_group invalid sourceweights");
            }
        }
        for (int i = 0; i < outdegree; ++i) {
            if (((destinations[i] < 0) && (destinations[i] != MPI_PROC_NULL)) || destinations[i] >= group_size) { 
                return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type, MPI_ERR_ARG,
                                              "MPI_Dist_graph_create_adjacent_from_group invalid destinations");
            } else if (MPI_UNWEIGHTED != destweights && destweights[i] < 0) {
                return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type, MPI_ERR_ARG,
                                              "MPI_Dist_graph_create_adjacent_from_group invalid destweights");
            }
        }
    }

    /* Ensure there is a topo attached to this communicator */
    if(OMPI_SUCCESS != (err = mca_topo_base_group_select (group, NULL, &topo, OMPI_COMM_DIST_GRAPH))) {
        return ompi_errhandler_invoke (errhandler, MPI_COMM_SELF, errhandler->eh_mpi_object_type,
                                       err, FUNC_NAME);
    }

    err = topo->topo.dist_graph.dist_graph_create_adjacent_from_group (topo, group, tag, errhandler, indegree,
                                                                       sources, sourceweights, outdegree,
                                                                       destinations, destweights, &(info->super),
                                                                       reorder, comm_dist_graph);
    OMPI_ERRHANDLER_RETURN(err, MPI_COMM_SELF, err, FUNC_NAME);
}

