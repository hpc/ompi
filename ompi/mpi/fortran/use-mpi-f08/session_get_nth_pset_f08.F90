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

subroutine MPI_Session_get_nth_pset_f08(session, n, pset_len, pset_name, ierror)
   use :: mpi_f08_types, only : MPI_Session
   use :: ompi_mpifh_bindings, only : ompi_session_get_nth_pset_f
   implicit none
   TYPE(MPI_Session), INTENT(IN) :: session
   INTEGER, OPTIONAL, INTENT(IN) :: n
   INTEGER, OPTIONAL, INTENT(IN) :: pset_len
   CHARACTER(LEN=*), INTENT(OUT) :: pset_name
   INTEGER, OPTIONAL, INTENT(OUT) :: ierror
   integer :: c_ierror

   call ompi_session_get_nth_pset_f(session%MPI_VAL, n, pset_len, pset_name, c_ierror)
   if (present(ierror)) ierror = c_ierror

end subroutine MPI_Session_get_nth_pset_f08
