/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2009 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2018      Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef MCA_COLL_LAZY_EXPORT_H
#define MCA_COLL_LAZY_EXPORT_H

#include "ompi_config.h"

#include "mpi.h"

#include "opal/class/opal_object.h"
#include "opal/mca/mca.h"
#include "opal/util/output.h"

#include "ompi/constants.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/base.h"
#include "ompi/communicator/communicator.h"

BEGIN_C_DECLS

/* API functions */

int mca_coll_lazy_init_query(bool enable_progress_threads,
                             bool enable_mpi_threads);
mca_coll_base_module_t
*mca_coll_lazy_comm_query(struct ompi_communicator_t *comm,
                          int *priority);

int mca_coll_lazy_module_enable(mca_coll_base_module_t *module,
                                struct ompi_communicator_t *comm);

int mca_coll_lazy_barrier (struct ompi_communicator_t *comm, mca_coll_base_module_t *module);

/* Types */
/* Module */

typedef struct mca_coll_lazy_module_t {
    mca_coll_base_module_t super;
} mca_coll_lazy_module_t;

OBJ_CLASS_DECLARATION(mca_coll_lazy_module_t);

/* Component */

typedef struct mca_coll_lazy_component_t {
    mca_coll_base_component_2_0_0_t super;

    /* Priority of this component */
    int priority;
} mca_coll_lazy_component_t;

/* Globally exported variables */

OMPI_MODULE_DECLSPEC extern mca_coll_lazy_component_t mca_coll_lazy_component;

END_C_DECLS

#endif /* MCA_COLL_LAZY_EXPORT_H */
