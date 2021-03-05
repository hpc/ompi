/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2014 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012      Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2017      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2018-2020 Triad National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**
 * @file
 *
 * Back-end MPI attribute engine.
 *
 * This is complicated enough that it deserves a lengthy discussion of
 * what is happening.  This is extremely complicated stuff, paired
 * with the fact that it is not described well in the MPI standard.
 * There are several places in the standard that should be read about
 * attributes:
 *
 * MPI-1:   Section 5.7 (pp 167-173)
 * MPI-1:   Section 7.1 (pp 191-192) predefined attributes in MPI-1
 * MPI-2:   Section 4.12.7 (pp 57-59) interlanguage attribute
 *          clarifications
 * MPI-2:   Section 6.2.2 (pp 112) window predefined attributes
 * MPI-2:   Section 8.8 (pp 198-208) new attribute caching functions
 * MPI-3.1: Section 11.2.6 (pp 414-415) window attributes
 *
 * After reading all of this, note the following:
 *
 * - C MPI-1 and MPI-2 attribute functions and functionality are
 *   identical except for their function names.
 * - Fortran MPI-1 and MPI-2 attribute functions and functionality are
 *   different (namely: the parameters are different sizes, both in the
 *   functions and the user callbacks, and the assignments to the
 *   different sized types occur differently [e.g., truncation and sign
 *   extension])
 * - C functions store values by reference (i.e., writing an attribute
 *   means writing a pointer to an instance of something; changing the
 *   value of that instance will make it visible to anyone who reads
 *   that attribute value).
 * - C also internally store some int attributes of a MPI_Win by value,
 *   and these attributes are read-only (i.e. set once for all)
 * - Fortran functions store values by value (i.e., writing an
 *   attribute value means that anyone who reads that attribute value
 *   will not be able to affect the value read by anyone else).
 * - The predefined attribute MPI_WIN_BASE seems to flaunt the rules
 *   designated by the rest of the standard; it is handled
 *   specifically in the MPI_WIN_GET_ATTR binding functions (see the
 *   comments in there for an explanation).
 * - MPI-2 4.12.7:Example 4.13 (p58) is wrong.  The C->Fortran example
 *   should have the Fortran "val" variable equal to &I.
 *
 * By the first two of these, there are 12 possible use cases -- 4
 * possibilities for writing an attribute value, each of which has 3
 * possibilities for reading that value back.  The following lists
 * each of the 12 cases, and what happens in each.
 *
 * Cases where C writes an attribute value:
 * ----------------------------------------
 *
 * In all of these cases, a pointer was written by C (e.g., a pointer
 * to an int -- but it could have been a pointer to anything, such as
 * a struct).  These scenarios each have 2 examples:
 *
 * Example A: int foo = 3;
 *            MPI_Attr_put(..., &foo);
 * Example B: struct foo bar;
 *            MPI_Attr_put(..., &bar);
 *
 * 1. C reads the attribute value.  Clearly, this is a "unity" case,
 * and no translation occurs.  A pointer is written, and that same
 * pointer is returned.
 *
 * Example A: int *ret;
 *            MPI_Attr_get(..., &ret);
 *            --> *ret will equal 3
 * Example B: struct foo *ret;
 *            MPI_Attr_get(..., &ret);
 *            --> *ret will point to the instance bar that was written
 *
 * 2. Fortran MPI-1 reads the attribute value.  The C pointer is cast
 * to a fortran INTEGER (i.e., MPI_Fint) -- potentially being
 * truncated if sizeof(void*) > sizeof(INTEGER).
 *
 * Example A: INTEGER ret
 *            CALL MPI_ATTR_GET(..., ret, ierr)
 *            --> ret will equal &foo, possibly truncated
 * Example B: INTEGER ret
 *            CALL MPI_ATTR_GET(..., ret, ierr)
 *            --> ret will equal &bar, possibly truncated
 *
 * 3. Fortran MPI-2 reads the attribute value.  The C pointer is cast
 * to a fortran INTEGER(KIND=MPI_ADDRESS_KIND) (i.e., a (MPI_Aint)).
 *
 * Example A: INTEGER(KIND=MPI_ADDRESS_KIND) ret
 *            CALL MPI_COMM_GET_ATTR(..., ret, ierr)
 *            --> ret will equal &foo
 * Example B: INTEGER(KIND=MPI_ADDRESS_KIND) ret
 *            CALL MPI_COMM_GET_ATTR(..., ret, ierr)
 *            --> ret will equal &bar
 *
 * Cases where C writes an int attribute:
 * ----------------------------------------------------
 *
 * In all of these cases, an int is written by C.
 * This is done internally when writing the attributes of a MPI_Win
 *
 * Example: int foo = 7;
 *          ompi_set_attr_int(..., foo, ...)
 *
 * 4. C reads the attribute value.  The value returned is a pointer
 *    that points to an int that has a value
 *    of 7.
 *
 * Example: int *ret;
 *          MPI_Attr_get(..., &ret);
 *          -> *ret will equal 7.
 *
 * 5. Fortran MPI-1 reads the attribute value.  This is the unity
 *    case; the same value is returned.
 *
 * Example: INTEGER ret
 *          CALL MPI_ATTR_GET(..., ret, ierr)
 *          --> ret will equal 7
 *
 * 6. Fortran MPI-2 reads the attribute value.  The same value is
 *    returned, but potentially sign-extended if sizeof(INTEGER) <
 *    sizeof(INTEGER(KIND=MPI_ADDRESS_KIND)).
 *
 * Example: INTEGER(KIND=MPI_ADDRESS_KIND) ret
 *          CALL MPI_COMM_GET_ATTR(..., ret, ierr)
 *          --> ret will equal 7
 *
 * Cases where Fortran MPI-1 writes an attribute value:
 * ----------------------------------------------------
 *
 * In all of these cases, an INTEGER is written by Fortran.
 *
 * Example: INTEGER FOO = 7
 *          CALL MPI_ATTR_PUT(..., foo, ierr)
 *
 * 7. C reads the attribute value.  The value returned is a pointer
 *    that points to an INTEGER (i.e., an MPI_Fint) that has a value
 *    of 7.
 *    --> NOTE: The external MPI interface does not distinguish between
 *        this case and case 7.  It is the programer's responsibility
 *        to code accordingly.
 *
 * Example: MPI_Fint *ret;
 *          MPI_Attr_get(..., &ret);
 *          -> *ret will equal 7.
 *
 * 8. Fortran MPI-1 reads the attribute value.  This is the unity
 *    case; the same value is returned.
 *
 * Example: INTEGER ret
 *          CALL MPI_ATTR_GET(..., ret, ierr)
 *          --> ret will equal 7
 *
 * 9. Fortran MPI-2 reads the attribute value.  The same value is
 *    returned, but potentially sign-extended if sizeof(INTEGER) <
 *    sizeof(INTEGER(KIND=MPI_ADDRESS_KIND)).
 *
 * Example: INTEGER(KIND=MPI_ADDRESS_KIND) ret
 *          CALL MPI_COMM_GET_ATTR(..., ret, ierr)
 *          --> ret will equal 7
 *
 * Cases where Fortran MPI-2 writes an attribute value:
 * ----------------------------------------------------
 *
 * In all of these cases, an INTEGER(KIND=MPI_ADDRESS_KIND) is written
 * by Fortran.
 *
 * Example A: INTEGER(KIND=MPI_ADDRESS_KIND) FOO = 12
 *            CALL MPI_COMM_PUT_ATTR(..., foo, ierr)
 * Example B: // Assume a platform where sizeof(void*) = 8 and
 *            // sizeof(INTEGER) = 4.
 *            INTEGER(KIND=MPI_ADDRESS_KIND) FOO = pow(2, 40)
 *            CALL MPI_COMM_PUT_ATTR(..., foo, ierr)
 *
 * 10. C reads the attribute value.  The value returned is a pointer
 *    that points to an INTEGER(KIND=MPI_ADDRESS_KIND) (i.e., a void*)
 *    that has a value of 12.
 *    --> NOTE: The external MPI interface does not distinguish between
 *        this case and case 4.  It is the programer's responsibility
 *        to code accordingly.
 *
 * Example A: MPI_Aint *ret;
 *            MPI_Attr_get(..., &ret);
 *            -> *ret will equal 12
 * Example B: MPI_Aint *ret;
 *            MPI_Attr_get(..., &ret);
 *            -> *ret will equal 2^40
 *
 * 11. Fortran MPI-1 reads the attribute value.  The same value is
 *    returned, but potentially truncated if sizeof(INTEGER) <
 *    sizeof(INTEGER(KIND=MPI_ADDRESS_KIND)).
 *
 * Example A: INTEGER ret
 *            CALL MPI_ATTR_GET(..., ret, ierr)
 *            --> ret will equal 12
 * Example B: INTEGER ret
 *            CALL MPI_ATTR_GET(..., ret, ierr)
 *            --> ret will equal 0
 *
 * 12. Fortran MPI-2 reads the attribute value.  This is the unity
 *    case; the same value is returned.
 *
 * Example A: INTEGER(KIND=MPI_ADDRESS_KIND) ret
 *            CALL MPI_COMM_GET_ATTR(..., ret, ierr)
 *            --> ret will equal 7
 * Example B: INTEGER(KIND=MPI_ADDRESS_KIND) ret
 *            CALL MPI_COMM_GET_ATTR(..., ret, ierr)
 *            --> ret will equal 2^40
 */

#include "ompi_config.h"

#include "opal/class/opal_bitmap.h"
#include "opal/mca/threads/mutex.h"
#include "opal/sys/atomic.h"

#include "ompi/attribute/attribute.h"
#include "ompi/constants.h"
#include "ompi/datatype/ompi_datatype.h"
#include "ompi/communicator/communicator.h"  /* ompi_communicator_t generated in [COPY|DELETE]_ATTR_CALLBACKS */
#include "ompi/win/win.h"                    /* ompi_win_t generated in [COPY|DELETE]_ATTR_CALLBACKS */
#include "ompi/instance/instance.h"
#include "ompi/mpi/fortran/base/fint_2_int.h"


/*
 * Macros
 */

#define ATTR_TABLE_SIZE 10

/* This is done so that I can have a consistent interface to my macros
   here */

#define MPI_DATATYPE_NULL_COPY_FN MPI_TYPE_NULL_COPY_FN
#define attr_communicator_f c_f_to_c_index
#define attr_datatype_f d_f_to_c_index
#define attr_win_f w_f_to_c_index
#define attr_instance_f i_f_to_c_index

#define CREATE_KEY(key) opal_bitmap_find_and_set_first_unset_bit(attr_subsys->key_bitmap, (key))

#define FREE_KEY(key) opal_bitmap_clear_bit(attr_subsys->key_bitmap, (key))


/* Not checking for NULL_DELETE_FN here, since according to the
   MPI-standard it should be a valid function that returns
   MPI_SUCCESS.

   This macro exists because we have to replicate the same code for
   MPI_Comm, MPI_Datatype, and MPI_Win.  Ick.

   There are 3 possible sets of callbacks:

   1. MPI-1 Fortran-style: attribute and extra state arguments are of
      type (INTEGER).  This is used if both the OMPI_KEYVAL_F77 and
      OMPI_KEYVAL_F77_INT flags are set.
   2. MPI-2 Fortran-style: attribute and extra state arguments are of
      type (INTEGER(KIND=MPI_ADDRESS_KIND)).  This is used if the
      OMPI_KEYVAL_F77 flag is set and the OMPI_KEYVAL_F77_INT flag is
      *not* set.
   3. C-style: attribute arguments are of type (void*).  This is used
      if OMPI_KEYVAL_F77 is not set.

   Ick.
 */

#define DELETE_ATTR_CALLBACKS(type, attribute, keyval_obj, object, err)     \
do { \
    opal_mutex_unlock(&attribute_lock); \
    if (0 != (keyval_obj->attr_flag & OMPI_KEYVAL_F77)) { \
        MPI_Fint f_key = OMPI_INT_2_FINT(key); \
        MPI_Fint f_err; \
        MPI_Fint attr_##type##_f;                \
        attr_##type##_f = OMPI_INT_2_FINT(((ompi_##type##_t *)keyval_obj)->attr_##type##_f); \
        /* MPI-1 Fortran-style */ \
        if (0 != (keyval_obj->attr_flag & OMPI_KEYVAL_F77_INT)) { \
            MPI_Fint attr_val = translate_to_fint(attribute); \
            (*((keyval_obj->delete_attr_fn).attr_fint_delete_fn)) \
                (&attr_##type##_f,                                        \
                 &f_key, &attr_val, &keyval_obj->extra_state.f_integer, &f_err); \
            if (MPI_SUCCESS != OMPI_FINT_2_INT(f_err)) { \
                err = OMPI_FINT_2_INT(f_err);           \
            } \
        } \
        /* MPI-2 Fortran-style */ \
        else { \
            MPI_Aint attr_val = translate_to_aint(attribute); \
            (*((keyval_obj->delete_attr_fn).attr_aint_delete_fn)) \
                (&attr_##type##_f,                                        \
                 &f_key, (int*)&attr_val, &keyval_obj->extra_state.f_address, &f_err); \
            if (MPI_SUCCESS != OMPI_FINT_2_INT(f_err)) { \
                err = OMPI_FINT_2_INT(f_err); \
            } \
        } \
    } \
    /* C style */ \
    else { \
        void *attr_val = translate_to_c(attribute); \
        err = (*((keyval_obj->delete_attr_fn).attr_##type##_delete_fn)) \
            ((ompi_##type##_t *)object,                                 \
             key, attr_val,                                             \
             keyval_obj->extra_state.c_ptr);                            \
    } \
    opal_mutex_lock(&attribute_lock); \
} while (0)

/* See the big, long comment above from DELETE_ATTR_CALLBACKS -- most of
   that text applies here, too. */

#define COPY_ATTR_CALLBACKS(type, old_object, keyval_obj, in_attr, new_object, out_attr, err) \
do { \
    opal_mutex_unlock(&attribute_lock); \
    if (0 != (keyval_obj->attr_flag & OMPI_KEYVAL_F77)) { \
        MPI_Fint f_key = OMPI_INT_2_FINT(key); \
        MPI_Fint f_err; \
        ompi_fortran_logical_t f_flag; \
        /* MPI-1 Fortran-style */ \
        if (0 != (keyval_obj->attr_flag & OMPI_KEYVAL_F77_INT)) { \
            MPI_Fint in, out;                        \
            MPI_Fint attr_##type##_f;                \
            in = translate_to_fint(in_attr); \
            attr_##type##_f = OMPI_INT_2_FINT(((ompi_##type##_t *)old_object)->attr_##type##_f); \
            (*((keyval_obj->copy_attr_fn).attr_fint_copy_fn)) \
                (&attr_##type##_f, \
                 &f_key, &keyval_obj->extra_state.f_integer, \
                 &in, &out, &f_flag, &f_err); \
            if (MPI_SUCCESS != OMPI_FINT_2_INT(f_err)) { \
                err = OMPI_FINT_2_INT(f_err);           \
            } else {                                    \
                out_attr->av_value = (void*) 0;         \
                *out_attr->av_fint_pointer = out;    \
                flag = OMPI_LOGICAL_2_INT(f_flag);      \
            }                                           \
        } \
        /* MPI-2 Fortran-style */ \
        else { \
            MPI_Aint in, out;                        \
            MPI_Fint attr_##type##_f;                \
            in = translate_to_aint(in_attr); \
            attr_##type##_f = OMPI_INT_2_FINT(((ompi_##type##_t *)old_object)->attr_##type##_f); \
            (*((keyval_obj->copy_attr_fn).attr_aint_copy_fn)) \
                (&attr_##type##_f, \
                 &f_key, &keyval_obj->extra_state.f_address, &in, &out, \
                 &f_flag, &f_err); \
            if (MPI_SUCCESS != OMPI_FINT_2_INT(f_err)) { \
                err = OMPI_FINT_2_INT(f_err);           \
            } else {                                    \
                out_attr->av_value = (void *) out;      \
                flag = OMPI_LOGICAL_2_INT(f_flag);      \
            }                                           \
        } \
    } \
    /* C style */ \
    else { \
        void *in, *out; \
        in = translate_to_c(in_attr); \
        if ((err = (*((keyval_obj->copy_attr_fn).attr_##type##_copy_fn)) \
              ((ompi_##type##_t *)old_object, key, keyval_obj->extra_state.c_ptr, \
               in, &out, &flag, (ompi_##type##_t *)(new_object))) == MPI_SUCCESS) { \
            out_attr->av_value = out;                                   \
        }                                                               \
    } \
    opal_mutex_lock(&attribute_lock); \
} while (0)

/*
 * Cases for attribute values
 */
typedef enum ompi_attribute_translate_t {
    OMPI_ATTRIBUTE_C,
    OMPI_ATTRIBUTE_INT,
    OMPI_ATTRIBUTE_FINT,
    OMPI_ATTRIBUTE_AINT
} ompi_attribute_translate_t;

/*
 * struct to hold attribute values on each MPI object
 */
typedef struct attribute_value_t {
    opal_object_t super;
    int av_key;
    void *av_value;
    int *av_int_pointer;
    MPI_Fint *av_fint_pointer;
    MPI_Aint *av_aint_pointer;
    int av_set_from;
    int av_sequence;
} attribute_value_t;

/*
 * struct to hold state of attr subsys
 */

typedef  struct attr_subsys_t {
    opal_object_t            super;
    opal_hash_table_t *keyval_hash;
    opal_bitmap_t *key_bitmap;
 } attr_subsys_t;

/*
 * Local functions
 */
static void attribute_value_construct(attribute_value_t *item);
static void ompi_attribute_keyval_construct(ompi_attribute_keyval_t *keyval);
static void ompi_attribute_keyval_destruct(ompi_attribute_keyval_t *keyval);
static void attr_subsys_construct(attr_subsys_t *subsys);
static void attr_subsys_destruct(attr_subsys_t *subsys);
static int set_value(ompi_attribute_type_t type, void *object,
                     opal_hash_table_t **attr_hash, int key,
                     attribute_value_t *new_attr,
                     bool predefined);
static int get_value(opal_hash_table_t *attr_hash, int key,
                     attribute_value_t **attribute, int *flag);
static void *translate_to_c(attribute_value_t *val);
static MPI_Fint translate_to_fint(attribute_value_t *val);
static MPI_Aint translate_to_aint(attribute_value_t *val);

static int compare_attr_sequence(const void *attr1, const void *attr2);

/*
 * attribute_subsys_t class
 */
static OBJ_CLASS_INSTANCE(attr_subsys_t,
                          opal_object_t,
                          attr_subsys_construct,
                          attr_subsys_destruct);

/*
 * attribute_value_t class
 */
static OBJ_CLASS_INSTANCE(attribute_value_t,
                          opal_object_t,
                          attribute_value_construct,
                          NULL);


/*
 * ompi_attribute_entry_t classes
 */
static OBJ_CLASS_INSTANCE(ompi_attribute_keyval_t,
                          opal_object_t,
                          ompi_attribute_keyval_construct,
                          ompi_attribute_keyval_destruct);


/*
 * Static variables
 */

static attr_subsys_t *attr_subsys = NULL;
static unsigned int int_pos = 12345;
static unsigned int integer_pos = 12345;
static int attr_sequence;

/*
 * MPI attributes are *not* high performance, so just use a One Big Lock
 * approach. However, this lock is released before a user provided callback is
 * triggered and acquired right after, allowing for recursive behaviors.
 */
static opal_mutex_t attribute_lock = OPAL_MUTEX_STATIC_INIT; 

/*
 * attribute_value_t constructor function
 */
static void attribute_value_construct(attribute_value_t *item)
{
    item->av_key = MPI_KEYVAL_INVALID;
    item->av_aint_pointer = (MPI_Aint*) &item->av_value;
    item->av_int_pointer = (int *)&item->av_value + int_pos;
    item->av_fint_pointer = (MPI_Fint *)&item->av_value + integer_pos;
    item->av_set_from = 0;
    item->av_sequence = -1;
}


/*
 * ompi_attribute_keyval_t constructor / destructor
 */
static void
ompi_attribute_keyval_construct(ompi_attribute_keyval_t *keyval)
{
    keyval->attr_type = UNUSED_ATTR;
    keyval->attr_flag = 0;
    keyval->copy_attr_fn.attr_communicator_copy_fn = NULL;
    keyval->delete_attr_fn.attr_communicator_copy_fn = NULL;
    keyval->extra_state.c_ptr = NULL;
    keyval->bindings_extra_state = NULL;

    /* Set the keyval->key value to an invalid value so that we can know
       if it has been initialized with a proper value or not.
       Specifically, the destructor may get invoked if we weren't able
       to assign a key properly.  So we don't want to try to remove it
       from the table if it wasn't there. */
    keyval->key = -1;
}


static void
ompi_attribute_keyval_destruct(ompi_attribute_keyval_t *keyval)
{
    if (-1 != keyval->key) {
        /* If the bindings_extra_state pointer is not NULL, free it */
        if (NULL != keyval->bindings_extra_state) {
            free(keyval->bindings_extra_state);
        }

        opal_hash_table_remove_value_uint32(attr_subsys->keyval_hash, keyval->key);
        FREE_KEY(keyval->key);
    }
}


int ompi_attr_get_ref(void)
{
    int ret = OMPI_SUCCESS;

    opal_mutex_lock(&attribute_lock);

    if (NULL == attr_subsys) {
        attr_subsys = OBJ_NEW(attr_subsys_t);
	if (NULL == attr_subsys) {
            ret = OMPI_ERR_OUT_OF_RESOURCE;
	    goto fn_exit;
	}
    } else {
        OBJ_RETAIN(attr_subsys);
    }

fn_exit:
    opal_mutex_unlock(&attribute_lock);

    return ret;
}

int ompi_attr_put_ref(void)
{
    if (NULL != attr_subsys) {
        OBJ_RELEASE(attr_subsys);
    }
    return OMPI_SUCCESS;
}

static void attr_subsys_construct(attr_subsys_t *subsys)
{
    int ret;
    void *bogus = (void*) 1;
    int *p = (int *) &bogus;

    subsys->keyval_hash = OBJ_NEW(opal_hash_table_t);
    assert (NULL != subsys->keyval_hash);

    subsys->key_bitmap = OBJ_NEW(opal_bitmap_t);

    /*
     * Set the max size to OMPI_FORTRAN_HANDLE_MAX to enforce bound
     */
    opal_bitmap_set_max_size (subsys->key_bitmap, 
                              OMPI_FORTRAN_HANDLE_MAX);
    ret = opal_bitmap_init(subsys->key_bitmap, 32);
    assert(OPAL_SUCCESS == ret);

    for (int i = 0; i <= MPI_WIN_MODEL; i++) {
        opal_bitmap_set_bit(subsys->key_bitmap, i);
    }

    for (int_pos = 0; int_pos < (sizeof(void*) / sizeof(int));
         ++int_pos) {
        if (p[int_pos] == 1) {
            break;
        }
    }

    for (integer_pos = 0; integer_pos < (sizeof(void*) / sizeof(MPI_Fint));
         ++integer_pos) {
        if (p[integer_pos] == 1) {
            break;
        }
    }

    ret = opal_hash_table_init(subsys->keyval_hash, ATTR_TABLE_SIZE);
    assert (OPAL_SUCCESS == ret);

    attr_sequence = 0;    
}


/*
 * Cleanup everything when no more refs to the attr subsys
 */
static void attr_subsys_destruct(attr_subsys_t *subsys)
{
    ompi_attr_free_predefined();
    OBJ_RELEASE(subsys->keyval_hash);
    OBJ_RELEASE(subsys->key_bitmap);
}

/*****************************************************************************/

static int ompi_attr_create_keyval_impl(ompi_attribute_type_t type,
                            ompi_attribute_fn_ptr_union_t copy_attr_fn,
                            ompi_attribute_fn_ptr_union_t delete_attr_fn,
                            int *key,
                            ompi_attribute_fortran_ptr_t *extra_state,
                            int flags,
                            void *bindings_extra_state)
{
    ompi_attribute_keyval_t *keyval;
    int ret;

    /* Allocate space for the list item */
    keyval = OBJ_NEW(ompi_attribute_keyval_t);
    if (NULL == keyval) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    /* Fill in the list item (must be done before we set the keyval
       on the keyval_hash in case some other thread immediately reads
       it from the keyval_hash) */
    keyval->copy_attr_fn = copy_attr_fn;
    keyval->delete_attr_fn = delete_attr_fn;
    keyval->extra_state = *extra_state;
    keyval->attr_type = type;
    keyval->attr_flag = flags;
    keyval->key = -1;
    keyval->bindings_extra_state = bindings_extra_state;

    /* Create a new unique key and fill the hash */
    opal_mutex_lock(&attribute_lock);
    ret = MPI_SUCCESS;
    if (!(flags & OMPI_KEYVAL_PREDEFINED)) {
        ret = CREATE_KEY(key);
    }
    
    if (OMPI_SUCCESS == ret) {
        keyval->key = *key;
        ret = opal_hash_table_set_value_uint32(attr_subsys->keyval_hash,
                                               *key, keyval);
    }

    if (OMPI_SUCCESS != ret) {
        OBJ_RELEASE(keyval);
    } else {
        ret = MPI_SUCCESS;
    }

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);
    return ret;
}

int ompi_attr_create_keyval(ompi_attribute_type_t type,
                            ompi_attribute_fn_ptr_union_t copy_attr_fn,
                            ompi_attribute_fn_ptr_union_t delete_attr_fn,
                            int *key,
                            void *extra_state,
                            int flags,
                            void *bindings_extra_state)
{
    ompi_attribute_fortran_ptr_t es_tmp;
    int rc;

    rc = ompi_mpi_instance_retain ();
    if (OPAL_UNLIKELY(OMPI_SUCCESS != rc)) {
        return rc;
    }

    es_tmp.c_ptr = extra_state;
    rc = ompi_attr_create_keyval_impl(type, copy_attr_fn, delete_attr_fn,
                                      key, &es_tmp, flags,
                                      bindings_extra_state);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != rc)) {
        ompi_mpi_instance_release ();
    }

    return rc;
}

int ompi_attr_create_keyval_fint(ompi_attribute_type_t type,
                                 ompi_attribute_fn_ptr_union_t copy_attr_fn,
                                 ompi_attribute_fn_ptr_union_t delete_attr_fn,
                                 int *key,
                                 MPI_Fint extra_state,
                                 int flags,
                                 void *bindings_extra_state)
{
    ompi_attribute_fortran_ptr_t es_tmp;
    int rc;

    rc = ompi_mpi_instance_retain ();
    if (OPAL_UNLIKELY(OMPI_SUCCESS != rc)) {
        return rc;
    }

    es_tmp.f_integer = extra_state;
#if SIZEOF_INT == OMPI_SIZEOF_FORTRAN_INTEGER
    flags |= OMPI_KEYVAL_F77_INT;
#endif
    return ompi_attr_create_keyval_impl(type, copy_attr_fn, delete_attr_fn,
                                        key, &es_tmp, flags,
                                        bindings_extra_state);
}

int ompi_attr_create_keyval_aint(ompi_attribute_type_t type,
                                 ompi_attribute_fn_ptr_union_t copy_attr_fn,
                                 ompi_attribute_fn_ptr_union_t delete_attr_fn,
                                 int *key,
                                 MPI_Aint extra_state,
                                 int flags,
                                 void *bindings_extra_state)
{
    ompi_attribute_fortran_ptr_t es_tmp;
    int rc;

    rc = ompi_mpi_instance_retain ();
    if (OPAL_UNLIKELY(OMPI_SUCCESS != rc)) {
        return rc;
    }

    es_tmp.f_address = extra_state;
    return ompi_attr_create_keyval_impl(type, copy_attr_fn, delete_attr_fn,
                                        key, &es_tmp, flags,
                                        bindings_extra_state);
}

/*****************************************************************************/

int ompi_attr_free_keyval(ompi_attribute_type_t type, int *key,
                          bool predefined)
{
    int ret;
    ompi_attribute_keyval_t *keyval;

    /* Find the key-value pair */
    opal_mutex_lock(&attribute_lock);
    ret = opal_hash_table_get_value_uint32(attr_subsys->keyval_hash, *key,
                                           (void **) &keyval);
    if ((OMPI_SUCCESS != ret) || (NULL == keyval) ||
        (keyval->attr_type != type) ||
        ((!predefined) && (keyval->attr_flag & OMPI_KEYVAL_PREDEFINED))) {
        opal_mutex_unlock(&attribute_lock);
        return OMPI_ERR_BAD_PARAM;
    }

    /* MPI says to set the returned value to MPI_KEYVAL_INVALID */
    *key = MPI_KEYVAL_INVALID;

    /* This will delete the key only when no attributes are associated
       with it, else it will just decrement the reference count, so that when
       the last attribute is deleted, this object gets deleted too */
    OBJ_RELEASE(keyval);

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);

    /* balance out retain in keyval_create */
    ompi_mpi_instance_release ();

    return MPI_SUCCESS;
}

/*****************************************************************************/

/*
 * Front-end function called by the C MPI API functions to set an
 * attribute.
 */
int ompi_attr_set_c(ompi_attribute_type_t type, void *object,
                    opal_hash_table_t **attr_hash,
                    int key, void *attribute, bool predefined)
{
    int ret;
    attribute_value_t *new_attr = OBJ_NEW(attribute_value_t);
    if (NULL == new_attr) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    opal_mutex_lock(&attribute_lock);

    new_attr->av_value = attribute;
    new_attr->av_set_from = OMPI_ATTRIBUTE_C;
    ret = set_value(type, object, attr_hash, key, new_attr, predefined);
    if (OMPI_SUCCESS != ret) {
        OBJ_RELEASE(new_attr);
    }

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);

    return ret;
}


/*
 * Front-end function internally called by the C API functions to set an
 * int attribute.
 */
int ompi_attr_set_int(ompi_attribute_type_t type, void *object,
                      opal_hash_table_t **attr_hash,
                      int key, int attribute, bool predefined)
{
    int ret;
    attribute_value_t *new_attr = OBJ_NEW(attribute_value_t);
    if (NULL == new_attr) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    opal_mutex_lock(&attribute_lock);

    new_attr->av_value = (void *) 0;
    *new_attr->av_int_pointer = attribute;
    new_attr->av_set_from = OMPI_ATTRIBUTE_INT;
    ret = set_value(type, object, attr_hash, key, new_attr, predefined);
    if (OMPI_SUCCESS != ret) {
        OBJ_RELEASE(new_attr);
    }

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);

    return ret;
}


/*
 * Front-end function called by the Fortran MPI-1 API functions to set
 * an attribute.
 */
int ompi_attr_set_fint(ompi_attribute_type_t type, void *object,
                       opal_hash_table_t **attr_hash,
                       int key, MPI_Fint attribute,
                       bool predefined)
{
    int ret;
    attribute_value_t *new_attr = OBJ_NEW(attribute_value_t);
    if (NULL == new_attr) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    opal_mutex_lock(&attribute_lock);

    new_attr->av_value = (void *) 0;
    *new_attr->av_fint_pointer = attribute;
    new_attr->av_set_from = OMPI_ATTRIBUTE_FINT;
    ret = set_value(type, object, attr_hash, key, new_attr, predefined);
    if (OMPI_SUCCESS != ret) {
        OBJ_RELEASE(new_attr);
    }

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);

    return ret;
}


/*
 * Front-end function called by the Fortran MPI-2 API functions to set
 * an attribute.
 */
int ompi_attr_set_aint(ompi_attribute_type_t type, void *object,
                       opal_hash_table_t **attr_hash,
                       int key, MPI_Aint attribute,
                       bool predefined)
{
    int ret;
    attribute_value_t *new_attr = OBJ_NEW(attribute_value_t);
    if (NULL == new_attr) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    opal_mutex_lock(&attribute_lock);

    new_attr->av_value = (void *) attribute;
    new_attr->av_set_from = OMPI_ATTRIBUTE_AINT;
    ret = set_value(type, object, attr_hash, key, new_attr, predefined);
    if (OMPI_SUCCESS != ret) {
        OBJ_RELEASE(new_attr);
    }

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);

    return ret;
}

/*****************************************************************************/

/*
 * Front-end function called by the C MPI API functions to get
 * attributes.
 */
int ompi_attr_get_c(opal_hash_table_t *attr_hash, int key,
                    void **attribute, int *flag)
{
    attribute_value_t *val = NULL;
    int ret;

    opal_mutex_lock(&attribute_lock);

    ret = get_value(attr_hash, key, &val, flag);
    if (MPI_SUCCESS == ret && 1 == *flag) {
        *attribute = translate_to_c(val);
    }

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);
    return ret;
}


/*
 * Front-end function called by the Fortran MPI-1 API functions to get
 * attributes.
 */
int ompi_attr_get_fint(opal_hash_table_t *attr_hash, int key,
                       MPI_Fint *attribute, int *flag)
{
    attribute_value_t *val = NULL;
    int ret;

    opal_mutex_lock(&attribute_lock);

    ret = get_value(attr_hash, key, &val, flag);
    if (MPI_SUCCESS == ret && 1 == *flag) {
        *attribute = translate_to_fint(val);
    }

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);
    return ret;
}


/*
 * Front-end function called by the Fortran MPI-2 API functions to get
 * attributes.
 */
int ompi_attr_get_aint(opal_hash_table_t *attr_hash, int key,
                       MPI_Aint *attribute, int *flag)
{
    attribute_value_t *val = NULL;
    int ret;

    opal_mutex_lock(&attribute_lock);

    ret = get_value(attr_hash, key, &val, flag);
    if (MPI_SUCCESS == ret && 1 == *flag) {
        *attribute = translate_to_aint(val);
    }

    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);
    return ret;
}

/*****************************************************************************/

/*
 * Copy all the attributes from one MPI object to another.  Called
 * when MPI objects are copied (e.g., back-end actions to
 * MPI_COMM_DUP).
 */
int ompi_attr_copy_all(ompi_attribute_type_t type, void *old_object,
                       void *new_object, opal_hash_table_t *oldattr_hash,
                       opal_hash_table_t *newattr_hash)
{
    int ret;
    int err;
    uint32_t key;
    int flag;
    void *node, *in_node;
    attribute_value_t *old_attr, *new_attr;
    ompi_attribute_keyval_t *hash_value;

    /* If there's nothing to do, just return */
    if (NULL == oldattr_hash) {
        return MPI_SUCCESS;
    }

    if (INSTANCE_ATTR == type) {
        return MPI_ERR_ARG;
    }

    opal_mutex_lock(&attribute_lock);

    /* Get the first attribute in the object's hash */
    ret = opal_hash_table_get_first_key_uint32(oldattr_hash, &key,
                                               (void **) &old_attr,
                                               &node);

    /* While we still have some attribute in the object's key hash */
    while (OMPI_SUCCESS == ret) {
        in_node = node;

        /* Get the keyval in the main keyval hash - so that we know
           what the copy_attr_fn is */
        err = opal_hash_table_get_value_uint32(attr_subsys->keyval_hash, key,
                                               (void **) &hash_value);
        if (OMPI_SUCCESS != err) {
            /* This should not happen! */
            ret = MPI_ERR_INTERN;
            goto out;
        }

        err = 0;
        new_attr = OBJ_NEW(attribute_value_t);
        switch (type) {
        case COMM_ATTR:
            /* Now call the copy_attr_fn */
            COPY_ATTR_CALLBACKS(communicator, old_object, hash_value,
                                old_attr, new_object, new_attr, err);
            break;

        case TYPE_ATTR:
            /* Now call the copy_attr_fn */
            COPY_ATTR_CALLBACKS(datatype, old_object, hash_value,
                                old_attr, new_object, new_attr, err);
            break;

        case WIN_ATTR:
            /* Now call the copy_attr_fn */
            COPY_ATTR_CALLBACKS(win, old_object, hash_value,
                                old_attr, new_object, new_attr, err);
            break;

        default:
            /* This should not happen */
            assert(0);
            break;
        }
        /* Did the callback return non-MPI_SUCCESS? */
        if (0 != err) {
            ret = err;
            OBJ_RELEASE(new_attr);
            goto out;
        }

        /* Hang this off the object's hash */

        /* The COPY_ATTR_CALLBACKS macro will have converted the
           _flag_ callback output value from Fortran's .TRUE. value to
           0/1 (if necessary).  So we only need to check for 0/1 here
           -- not .TRUE. */
        if (1 == flag) {
            if (0 != (hash_value->attr_flag & OMPI_KEYVAL_F77)) {
                if (0 != (hash_value->attr_flag & OMPI_KEYVAL_F77_INT)) {
                    new_attr->av_set_from = OMPI_ATTRIBUTE_FINT;
                } else {
                    new_attr->av_set_from = OMPI_ATTRIBUTE_AINT;
                }
            } else {
                new_attr->av_set_from = OMPI_ATTRIBUTE_C;
            }
            ret = set_value(type, new_object, &newattr_hash, key,
                            new_attr, true);
            if (MPI_SUCCESS != ret) {
                goto out;
            }
        } else {
            OBJ_RELEASE(new_attr);
        }

        ret = opal_hash_table_get_next_key_uint32(oldattr_hash, &key,
                                                  (void **) &old_attr,
                                                  in_node, &node);
    }
    ret = MPI_SUCCESS;

 out:
    /* All done */
    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);
    return ret;
}

/*****************************************************************************/

/*
 * Back-end function to delete a single attribute.
 *
 * Assumes that you DO already have the attribute_lock.
 */
static int ompi_attr_delete_impl(ompi_attribute_type_t type, void *object,
                                 opal_hash_table_t *attr_hash, int key,
                                 bool predefined)
{
    ompi_attribute_keyval_t *keyval;
    int ret = OMPI_SUCCESS;
    attribute_value_t *attr;

    /* Check if the key is valid in the master keyval hash */
    ret = opal_hash_table_get_value_uint32(attr_subsys->keyval_hash, key,
                                           (void **) &keyval);

    if ((OMPI_SUCCESS != ret) || (NULL == keyval) ||
        (keyval->attr_type!= type) ||
        ((!predefined) && (keyval->attr_flag & OMPI_KEYVAL_PREDEFINED))) {
        ret = OMPI_ERR_BAD_PARAM;
        goto exit;
    }

    /* Ensure that we don't have an empty attr_hash */
    if (NULL == attr_hash) {
        ret = OMPI_ERR_BAD_PARAM;
        goto exit;
    }

    /* Check if the key is valid for the communicator/window/dtype/instance. If
       yes, then delete the attribute and key entry from the object's
       hash */
    ret = opal_hash_table_get_value_uint32(attr_hash, key, (void**) &attr);
    if (OMPI_SUCCESS == ret) {
        switch (type) {
        case COMM_ATTR:
            DELETE_ATTR_CALLBACKS(communicator, attr, keyval, object, ret);
            break;

        case WIN_ATTR:
            DELETE_ATTR_CALLBACKS(win, attr, keyval, object, ret);
            break;

        case TYPE_ATTR:
            DELETE_ATTR_CALLBACKS(datatype, attr, keyval, object, ret);
            break;

        case INSTANCE_ATTR:
            DELETE_ATTR_CALLBACKS(instance, attr, keyval, object, ret);
            break;

        default:
            /* This should not happen */
            assert(0);
            break;
        }
        if (MPI_SUCCESS != ret) {
            goto exit;
        }

        /* Ignore the return value at this point; it can't help any
           more */
        (void) opal_hash_table_remove_value_uint32(attr_hash, key);
        OBJ_RELEASE(attr);
    }

 exit:
    /* Decrement the ref count for the keyval.  If ref count goes to
       0, destroy the keyval (the destructor deletes the key
       implicitly for this object).  The ref count will only go to 0
       here if MPI_*_FREE_KEYVAL was previously invoked and we just
       freed the last attribute that was using the keyval. */
    if (OMPI_SUCCESS == ret) {
        OBJ_RELEASE(keyval);
    }

    return ret;
}

/*
 * Front end function to delete a single attribute.
 */
int ompi_attr_delete(ompi_attribute_type_t type, void *object,
                     opal_hash_table_t *attr_hash, int key,
                     bool predefined)
{
    int ret;

    opal_mutex_lock(&attribute_lock);
    ret = ompi_attr_delete_impl(type, object, attr_hash, key, predefined);
    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);
    return ret;
}

/*
 * Front-end function to delete all the attributes on an MPI object
 */
int ompi_attr_delete_all(ompi_attribute_type_t type, void *object,
                         opal_hash_table_t *attr_hash)
{
    int ret, i, num_attrs;
    uint32_t key;
    void *node, *in_node, *attr;
    attribute_value_t **attrs;

    /* Ensure that the table is not empty */

    if (NULL == attr_hash) {
        return MPI_SUCCESS;
    }

    opal_mutex_lock(&attribute_lock);

    /* Make an array that contains all attributes in local object's hash */
    num_attrs = opal_hash_table_get_size(attr_hash);
    if (0 == num_attrs) {
        opal_mutex_unlock(&attribute_lock);
        return MPI_SUCCESS;
    }

    attrs = malloc(sizeof(attribute_value_t *) * num_attrs);
    if (NULL == attrs) {
        opal_mutex_unlock(&attribute_lock);
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    ret = opal_hash_table_get_first_key_uint32(attr_hash, &key, &attr, &node);
    for (i = 0; OMPI_SUCCESS == ret; i++) {
        attrs[i] = attr;
        in_node = node;
        ret = opal_hash_table_get_next_key_uint32(attr_hash, &key, &attr,
                                                  in_node, &node);
    }

    /* Sort attributes in the order that they were set */
    qsort(attrs, num_attrs, sizeof(attribute_value_t *), compare_attr_sequence);

    /* Delete attributes in the reverse order that they were set.
       Actually this ordering is required only for MPI_COMM_SELF, as
       specified in MPI-2.2: 8.7.1 Allowing User Functions at Process
       Termination, but we do it for everything -- what the heck.
       :-) */
    for (i = num_attrs - 1; i >= 0; i--) {
        ret = ompi_attr_delete_impl(type, object, attr_hash,
                                    attrs[i]->av_key, true);
        if (OMPI_SUCCESS != ret) {
            break;
        }
    }

    /* All done */

    free(attrs);
    opal_atomic_wmb();
    opal_mutex_unlock(&attribute_lock);
    return ret;
}

/*************************************************************************/

/*
 * Back-end function to set an attribute on an MPI object.  Assumes
 * that you already hold the attribute_lock.
 */
static int set_value(ompi_attribute_type_t type, void *object,
                     opal_hash_table_t **attr_hash, int key,
                     attribute_value_t *new_attr,
                     bool predefined)
{
    ompi_attribute_keyval_t *keyval;
    int ret;
    attribute_value_t *old_attr;
    bool had_old = false;

    /* Note that this function can be invoked by ompi_attr_copy_all()
       to set attributes on the new object (in addition to the
       top-level MPI_* functions that set attributes). */
    ret = opal_hash_table_get_value_uint32(attr_subsys->keyval_hash, key,
                                           (void **) &keyval);

    /* If key not found */
    if ((OMPI_SUCCESS != ret ) || (NULL == keyval) ||
        (keyval->attr_type != type) ||
        ((!predefined) && (keyval->attr_flag & OMPI_KEYVAL_PREDEFINED))) {
        return OMPI_ERR_BAD_PARAM;
    }

    /* Do we need to make a new attr_hash? */
    if (NULL == *attr_hash) {
        ompi_attr_hash_init(attr_hash);
    }

    /* Now see if an attribute is already present in the object's hash
       on the old keyval. If so, delete the old attribute value. */
    ret = opal_hash_table_get_value_uint32(*attr_hash, key, (void**) &old_attr);
    if (OMPI_SUCCESS == ret)  {
        switch (type) {
        case COMM_ATTR:
            DELETE_ATTR_CALLBACKS(communicator, old_attr, keyval, object, ret);
            break;

        case WIN_ATTR:
            DELETE_ATTR_CALLBACKS(win, old_attr, keyval, object, ret);
            break;

        case TYPE_ATTR:
            DELETE_ATTR_CALLBACKS(datatype, old_attr, keyval, object, ret);
            break;

        case INSTANCE_ATTR:
            DELETE_ATTR_CALLBACKS(instance, old_attr, keyval, object, ret);
            break;

        default:
            /* This should not happen */
            assert(0);
            break;
        }
        if (MPI_SUCCESS != ret) {
            return ret;
        }
        OBJ_RELEASE(old_attr);
        had_old = true;
    }

    ret = opal_hash_table_get_value_uint32(attr_subsys->keyval_hash, key,
                                           (void **) &keyval);
    if ((OMPI_SUCCESS != ret ) || (NULL == keyval)) {
        /* Keyval has disappeared underneath us -- this shouldn't
           happen! */
        assert(0);
        return OMPI_ERR_BAD_PARAM;
    }

    new_attr->av_key = key;
    new_attr->av_sequence = attr_sequence++;

    ret = opal_hash_table_set_value_uint32(*attr_hash, key, new_attr);

    /* Increase the reference count of the object, only if there was no
       old atribute/no old entry in the object's key hash */
    if (OMPI_SUCCESS == ret && !had_old) {
        OBJ_RETAIN(keyval);
    }

    return ret;
}

/*************************************************************************/

/*
 * Back-end function to get an attribute from the hash map and return
 * it to the caller.  Translation services are not provided -- they're
 * in small, standalone functions that are called from several
 * different places.
 *
 * Assumes that you do NOT already have the attribute lock.
 */
static int get_value(opal_hash_table_t *attr_hash, int key,
                     attribute_value_t **attribute, int *flag)
{
    int ret;
    void *attr;
    ompi_attribute_keyval_t *keyval;

    /* According to MPI specs, the call is invalid if the keyval does
       not exist (i.e., the key is not present in the main keyval
       hash).  If the keyval exists but no attribute is associated
       with the key, then the call is valid and returns FALSE in the
       flag argument */
    *flag = 0;
    ret = opal_hash_table_get_value_uint32(attr_subsys->keyval_hash, key,
                                           (void**) &keyval);
    if (OMPI_ERR_NOT_FOUND == ret) {
        return MPI_KEYVAL_INVALID;
    }

    /* If we have a null attr_hash table, that means that nothing has
       been cached on this object yet.  So just return *flag = 0. */
    if (NULL == attr_hash) {
        return OMPI_SUCCESS;
    }

    ret = opal_hash_table_get_value_uint32(attr_hash, key, &attr);
    if (OMPI_SUCCESS == ret) {
        *attribute = (attribute_value_t*)attr;
        *flag = 1;
    }

    return OMPI_SUCCESS;
}

/*************************************************************************/

/*
 * Take an attribute and translate it according to the cases listed in
 * the comments at the top of this file.
 *
 * This function does not fail -- it is only invoked in "safe"
 * situations.
 */
static void *translate_to_c(attribute_value_t *val)
{
    switch (val->av_set_from) {
    case OMPI_ATTRIBUTE_C:
        /* Case 1: wrote a C pointer, read a C pointer
           (unity) */
        return val->av_value;

    case OMPI_ATTRIBUTE_INT:
        /* Case 4: wrote an int, read a C pointer */
        return (void *) val->av_int_pointer;

    case OMPI_ATTRIBUTE_FINT:
        /* Case 7: wrote a MPI_Fint, read a C pointer */
        return (void *) val->av_fint_pointer;

    case OMPI_ATTRIBUTE_AINT:
        /* Case 10: wrote a MPI_Aint, read a C pointer */
        return (void *) val->av_aint_pointer;

    default:
        /* Should never reach here */
        return NULL;
    }
}


/*
 * Take an attribute and translate it according to the cases listed in
 * the comments at the top of this file.
 *
 * This function does not fail -- it is only invoked in "safe"
 * situations.
 */
static MPI_Fint translate_to_fint(attribute_value_t *val)
{
    switch (val->av_set_from) {
    case OMPI_ATTRIBUTE_C:
        /* Case 2: wrote a C pointer, read a MPI_Fint */
        return (MPI_Fint)*val->av_int_pointer;

    case OMPI_ATTRIBUTE_INT:
        /* Case 5: wrote an int, read a MPI_Fint */
        return (MPI_Fint)*val->av_int_pointer;

    case OMPI_ATTRIBUTE_FINT:
        /* Case 8: wrote a MPI_Fint, read a MPI_Fint
           (unity) */
        return *val->av_fint_pointer;

    case OMPI_ATTRIBUTE_AINT:
        /* Case 11: wrote a MPI_Aint, read a MPI_Fint */
        return (MPI_Fint)*val->av_fint_pointer;

    default:
        /* Should never reach here */
        return 0;
    }
}


/*
 * Take an attribute and translate it according to the cases listed in
 * the comments at the top of this file.
 *
 * This function does not fail -- it is only invoked in "safe"
 * situations.
 */
static MPI_Aint translate_to_aint(attribute_value_t *val)
{
    switch (val->av_set_from) {
    case OMPI_ATTRIBUTE_C:
       /* Case 3: wrote a C pointer, read a MPI_Aint */
        return (MPI_Aint) val->av_value;

    case OMPI_ATTRIBUTE_INT:
        /* Case 6: wrote an int, read a MPI_Aint */
        return (MPI_Aint) *val->av_int_pointer;

    case OMPI_ATTRIBUTE_FINT:
        /* Case 9: wrote a MPI_Fint, read a MPI_Aint */
        return (MPI_Aint) *val->av_fint_pointer;

    case OMPI_ATTRIBUTE_AINT:
        /* Case 12: wrote a MPI_Aint, read a MPI_Aint
           (unity) */
        return (MPI_Aint) val->av_value;

    default:
        /* Should never reach here */
        return 0;
    }
}

/*
 * Comparator for qsort() to sort attributes in the order that they were set.
 */
static int compare_attr_sequence(const void *attr1, const void *attr2)
{
    return (*(attribute_value_t **)attr1)->av_sequence -
           (*(attribute_value_t **)attr2)->av_sequence;
}
