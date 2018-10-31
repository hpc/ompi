#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2009 High Performance Computing Center Stuttgart,
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2016      Intel, Inc. All rights reserved
# Copyright (c) 2017      IBM Corporation.  All rights reserved.
# Copyright (c) 2018      Triad National Security, LLC. All rights
#                         reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

sources = \
        coll_lazy.h \
        coll_lazy_component.c \
        coll_lazy_module.c \
        coll_lazy_barrier.c

if MCA_BUILD_ompi_coll_lazy_DSO
component_noinst =
component_install = mca_coll_lazy.la
else
component_noinst = libmca_coll_lazy.la
component_install =
endif

mcacomponentdir = $(ompilibdir)
mcacomponent_LTLIBRARIES = $(component_install)
mca_coll_lazy_la_SOURCES = $(sources)
mca_coll_lazy_la_LDFLAGS = -module -avoid-version
mca_coll_lazy_la_LIBADD = $(top_builddir)/ompi/lib@OMPI_LIBMPI_NAME@.la

noinst_LTLIBRARIES = $(component_noinst)
libmca_coll_lazy_la_SOURCES =$(sources)
libmca_coll_lazy_la_LDFLAGS = -module -avoid-version
