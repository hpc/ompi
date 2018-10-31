/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2018      Intel, Inc. All rights reserved.
 * Copyright (c) 2018      Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>

#include "coll_lazy.h"

#include "mpi.h"

#include "opal/util/show_help.h"
#include "ompi/mca/rte/rte.h"

#include "ompi/constants.h"
#include "ompi/communicator/communicator.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/base.h"
#include "coll_lazy.h"


OBJ_CLASS_INSTANCE(mca_coll_lazy_module_t, mca_coll_base_module_t, NULL, NULL);


/*
 * Initial query function that is invoked during MPI_INIT, allowing
 * this component to disqualify itself if it doesn't support the
 * required level of thread support.
 */
int mca_coll_lazy_init_query(bool enable_progress_threads,
                             bool enable_mpi_threads)
{
    /* Nothing to do */
    return OMPI_SUCCESS;
}


/*
 * Invoked when there's a new communicator that has been created.
 * Look at the communicator and decide which set of functions and
 * priority we want to return.
 */
mca_coll_base_module_t *mca_coll_lazy_comm_query (ompi_communicator_t *comm, int *priority)
{
    mca_coll_lazy_module_t *lazy_module;

    *priority = 0;

    if (ompi_group_have_remote_peers (comm->c_local_group) || !OMPI_COMM_CHECK_ASSERT_LAZY_BARRIER(comm)) {
	return NULL;
    }

    lazy_module = OBJ_NEW(mca_coll_lazy_module_t);
    if (NULL == lazy_module) {
        return NULL;
    }

    *priority = mca_coll_lazy_component.priority;

    /* Choose whether to use [intra|inter] */
    lazy_module->super.coll_module_enable = mca_coll_lazy_module_enable;

    lazy_module->super.coll_barrier    = mca_coll_lazy_barrier;

    return &lazy_module->super;
}


/*
 * Init module on the communicator
 */
int mca_coll_lazy_module_enable (mca_coll_base_module_t *module, ompi_communicator_t *comm)
{
    return OMPI_SUCCESS;
}
