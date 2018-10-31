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
 * Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
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

#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/info/info.h"
#include "ompi/win/win.h"
#include "ompi/memchecker.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Win_allocate_from_group = PMPI_Win_allocate_from_group
#endif
#define MPI_Win_allocate_from_group PMPI_Win_allocate_from_group
#endif

static const char FUNC_NAME[] = "MPI_Win_allocate_from_group";


int MPI_Win_allocate_from_group (MPI_Aint size, int disp_unit, MPI_Info info,
                                 MPI_Group group, const char *tag, void *baseptr,
                                 MPI_Win *win)
{
    int ret;

    /* argument checking */
    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);

        if (NULL == group) {
            return MPI_ERR_GROUP;
        } else if (NULL == info || ompi_info_is_freed(info)) {
            return MPI_ERR_INFO;
        } else if (NULL == win) {
            return MPI_ERR_WIN;
        } else if ( size < 0 ) {
            return MPI_ERR_SIZE;
        } else if ( disp_unit <= 0 ) {
            return MPI_ERR_DISP;
        } else if (NULL == tag) {
            return MPI_ERR_TAG;
        }
    }


    OPAL_CR_ENTER_LIBRARY();

    /* create window and return */
    ret = ompi_win_allocate_from_group ((size_t)size, disp_unit, &(info->super),
                                        group, tag, baseptr, win);
    if (OMPI_SUCCESS != ret) {
        *win = MPI_WIN_NULL;
        OPAL_CR_EXIT_LIBRARY();
        return ompi_errcode_get_mpi_code (ret);
    }

    OPAL_CR_EXIT_LIBRARY();
    return MPI_SUCCESS;
}
