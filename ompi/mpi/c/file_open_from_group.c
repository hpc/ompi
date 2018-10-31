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
 * Copyright (c) 2013      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2013      Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      University of Houston. All rights reserved.
 * Copyright (c) 2017      IBM Corporation. All rights reserved.
 * Copyright (c) 2018      Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/info/info.h"
#include "ompi/file/file.h"
#include "ompi/mca/io/io.h"
#include "ompi/mca/io/base/base.h"
#include "ompi/memchecker.h"


extern opal_mutex_t ompi_mpi_file_bootstrap_mutex;

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_File_open_from_group = PMPI_File_open_from_group
#endif
#define MPI_File_open_from_group PMPI_File_open_from_group
#endif

static const char FUNC_NAME[] = "MPI_File_open_from_group";


int MPI_File_open_from_group (MPI_Group group, const char *tag, MPI_Errhandler errhandler,
                              const char *filename, int amode, MPI_Info info, MPI_File *fh)
{
    ompi_communicator_t *comm;
    int rc;

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (NULL == info || ompi_info_is_freed(info)) {
            return ompi_errhandler_invoke (errhandler, MPI_FILE_NULL, errhandler->eh_mpi_object_type,
                                           MPI_ERR_INFO, FUNC_NAME);
        }
        if (NULL == group) {
            return ompi_errhandler_invoke (errhandler, MPI_FILE_NULL, errhandler->eh_mpi_object_type,
                                           MPI_ERR_GROUP, FUNC_NAME);
        }
        if (NULL == tag) {
            return ompi_errhandler_invoke (errhandler, MPI_FILE_NULL, errhandler->eh_mpi_object_type,
                                           MPI_ERR_TAG, FUNC_NAME);
        }
    }

    /* Note that MPI-2:9.7 (p265 in the ps; p261 in the pdf) says that
       errors in MPI_FILE_OPEN_FROM_GROUP (before the file handle is created)
       should invoke the default error handler on MPI_FILE_NULL.
       Hence, if we get a file handle out of ompi_file_open(), invoke
       the error handler on that.  If not, invoke the error handler on
       MPI_FILE_NULL. */

    /* The io framework is only initialized lazily.  If it hasn't
       already been initialized, do so now (note that MPI_FILE_OPEN_FROM_GROUP
       and MPI_FILE_DELETE are the only two places that it will be
       initialized). */

    /* For multi-threaded scenarios, initializing the file i/o
       framework and mca infrastructure needs to be protected
       by a mutex, similarly to the other frameworks in
       ompi/runtime/ompi_mpi_init.c
    */

    /* NTH: until we have support for groups down in the io framework just do the
     * file open on a temporary communicator. */
    rc = ompi_comm_create_from_group (group, tag, &info->super, MPI_ERRORS_RETURN, &comm);
    if (OPAL_UNLIKELY(OMPI_SUCCESS == rc)) {
        return ompi_errhandler_invoke (errhandler, MPI_FILE_NULL, errhandler->eh_mpi_object_type,
                                       rc, FUNC_NAME);
    }

    opal_mutex_lock(&ompi_mpi_file_bootstrap_mutex);
    rc = mca_base_framework_open(&ompi_io_base_framework, 0);
    opal_mutex_unlock(&ompi_mpi_file_bootstrap_mutex);
    if (OMPI_SUCCESS != rc) {
        return ompi_errhandler_invoke (errhandler, MPI_FILE_NULL, errhandler->eh_mpi_object_type,
                                       rc, FUNC_NAME);
    }

    OPAL_CR_ENTER_LIBRARY();

    /* Create an empty MPI_File handle */

    *fh = MPI_FILE_NULL;
    rc = ompi_file_open(comm, filename, amode, &(info->super), fh);

    /* release the temporary communicator */
    ompi_comm_free (&comm);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != rc)) {
        return ompi_errhandler_invoke (errhandler, MPI_FILE_NULL, errhandler->eh_mpi_object_type,
                                       rc, FUNC_NAME);
    }

    /* Creating the file handle also selects a component to use,
       creates a module, and calls file_open() on the module.  So
       we're good to go. */
    return MPI_SUCCESS;
}
