! -*- f90 -*-
!
! Copyright (c) 2009-2012 Cisco Systems, Inc.  All rights reserved.
! Copyright (c) 2009-2013 Los Alamos National Security, LLC.
!                         All rights reserved.
! Copyright (c) 2018      Research Organization for Information Science
!                         and Technology (RIST).  All rights reserved.
! Copyright (c) 2019      Triad National Security, LLC. All rights
!                         reserved.
! $COPYRIGHT$

subroutine PMPI_Session_get_nth_psetlen_f08(session, n, pset_len, ierror)
   use :: mpi_f08_types, only : MPI_Session
   use :: ompi_mpifh_bindings, only : ompi_session_get_nth_psetlen_f
   implicit none
   TYPE(MPI_Session), INTENT(IN) :: session
   INTEGER, OPTIONAL, INTENT(IN) :: n
   INTEGER, OPTIONAL, INTENT(OUT) :: pset_len
   INTEGER, OPTIONAL, INTENT(OUT) :: ierror
   integer :: c_ierror

   call ompi_session_get_nth_psetlen_f(session%MPI_VAL, n, pset_len, c_ierror)
   if (present(ierror)) ierror = c_ierror

end subroutine PMPI_Session_get_nth_psetlen_f08
