/* OPEN QUESTIONS:

- should all Fortran INTEGER references be mqs_taddr_t in case they're
  not the same size as C int?

- what to do if we have a new Fortran interface (use mpi3) with
  non-INTEGER handles?

 */

/*---------------------------------------------------------------------------*/

/*
 * Copyright (c) 2007      High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2007-2009 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2007      The University of Tennessee and The University of
 *                         Tennessee Research Foundation.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * Some text copied from and references made to mpi_interface.h.
 * 
 * Copyright (C) 2000-2004 by Etnus, LLC
 * Copyright (C) 1999 by Etnus, Inc.
 * Copyright (C) 1997-1998 Dolphin Interconnect Solutions Inc.
 *
 * $HEADER$
 */

#ifndef __MPIDBG_INTERFACE_H__
#define __MPIDBG_INTERFACE_H__ 1

/* This isn't going to work... AMP 
#include "ompi_config.h"
*/

#define MPI_MAX_OBJECT_NAME 1024

/*
 * This file provides interface functions for a debugger to gather
 * additional information about MPI handles.
 */
#include <sys/types.h>

/* Include the Etnus debugger message queue interface so that we can
   use much of its infrastructure (e.g., the mqs_basic_callbacks,
   mqs_image_callbacks, and mqs_process_callbacks).

   minfo.c already included this AMP.
#define FOR_MPI2 0
#include "msgq_interface.h"
*/

/**************************************************************************
 * Types and macros
 **************************************************************************/

enum {
    MPIDBG_MAX_OBJECT_NAME = MPI_MAX_OBJECT_NAME
};
enum {
    MPIDBG_MAX_FILENAME = 1024
};
enum {
    MPIDBG_INTERFACE_VERSION = 1
};


/*-----------------------------------------------------------------------
 * Global initialization information for the DLL
 *-----------------------------------------------------------------------*/

/* Structure containing types for C and C++ MPI handles */
struct mpidbg_handle_info_t {
    /* C handle types.  They are typically pointers to something or
       integers. */
    /* Back-end type for MPI_Aint */
    mqs_type *hi_c_aint;
    /* Back-end type for MPI_Comm */
    mqs_type *hi_c_comm;
    /* Back-end type for MPI_Datatype */
    mqs_type *hi_c_datatype;
    /* Back-end type for MPI_Errhandler */
    mqs_type *hi_c_errhandler;
    /* Back-end type for MPI_File */
    mqs_type *hi_c_file;
    /* Back-end type for MPI_Group */
    mqs_type *hi_c_group;
    /* Back-end type for MPI_Info */
    mqs_type *hi_c_info;
    /* Back-end type for MPI_Offset */
    mqs_type *hi_c_offset;
    /* Back-end type for MPI_Op */
    mqs_type *hi_c_op;
    /* Back-end type for MPI_Request */
    mqs_type *hi_c_request;
    /* Back-end type for MPI_Status */
    mqs_type *hi_c_status;
    /* Back-end type for MPI_Win */
    mqs_type *hi_c_win;

    /* C++ handle types.  Note that these will always be *objects*,
       never pointers. */
    /* Back-end type for MPI::Aint */
    mqs_type *hi_cxx_aint;
    /* Back-end type for MPI::Comm */
    mqs_type *hi_cxx_comm;
    /* Back-end type for MPI::Intracomm */
    mqs_type *hi_cxx_intracomm;
    /* Back-end type for MPI::Intercomm */
    mqs_type *hi_cxx_intercomm;
    /* Back-end type for MPI::Graphcomm */
    mqs_type *hi_cxx_graphcomm;
    /* Back-end type for MPI::Cartcomm */
    mqs_type *hi_cxx_cartcomm;
    /* Back-end type for MPI::Datatype */
    mqs_type *hi_cxx_datatype;
    /* Back-end type for MPI::Errhandler */
    mqs_type *hi_cxx_errhandler;
    /* Back-end type for MPI::File */
    mqs_type *hi_cxx_file;
    /* Back-end type for MPI::Group */
    mqs_type *hi_cxx_group;
    /* Back-end type for MPI::Info */
    mqs_type *hi_cxx_info;
    /* Back-end type for MPI::Offset */
    mqs_type *hi_cxx_offset;
    /* Back-end type for MPI::Op */
    mqs_type *hi_cxx_op;
    /* Back-end type for MPI::Request */
    mqs_type *hi_cxx_request;
    /* Back-end type for MPI::Prequest */
    mqs_type *hi_cxx_prequest;
    /* Back-end type for MPI::Grequest */
    mqs_type *hi_cxx_grequest;
    /* Back-end type for MPI::Status */
    mqs_type *hi_cxx_status;
    /* Back-end type for MPI::Win */
    mqs_type *hi_cxx_win;
};

enum mpidbg_return_codes_t {
    /* Success */
    MPIDBG_SUCCESS,
    /* Something was not found */
    MPIDBG_ERR_NOT_FOUND,
    /* Something is not supported */
    MPIDBG_ERR_NOT_SUPPORTED,
    /* Something is out of range */
    MPIDBG_ERR_OUT_OF_RANGE,
    /* Something is not available */
    MPIDBG_ERR_UNAVAILABLE,
    /* Ran out of memory */
    MPIDBG_ERR_NO_MEM,
    /* Sentinel max value */
    MPIDBG_MAX_RETURN_CODE
};

/*-----------------------------------------------------------------------
 * General data structures
 *-----------------------------------------------------------------------*/

/* Information about MPI processes */
struct mpidbg_process_t {
    /* JMS: need something to uniquely ID MPI processes in the
       presence of MPI_COMM_SPAWN */

    /* Global rank in MPI_COMM_WORLD */
    int mpi_comm_world_rank;
};
/* ==> JMS Should we just use mqs_process_location instead?  George
   thinks that this is unncessary -- perhaps due to the fact that we
   could use mqs_process_location...?  Need to get some feedback from
   others on this one.  Need to check Euro PVM/MPI '06 paper... */

/* General name -> handle address mappings.  This is an optional type
   that is used to describe MPI's predefined handles if the
   pre-defined names do not appear as symbols in the MPI process.
   E.g., if MPI_COMM_WORLD is a #define that maps to some other value,
   this data structure can be used to map the string "MPI_COMM_WORLD"
   to the actual value of the handle that it corresponds to (e.g., 0
   or a pointer value). */
struct mpidbg_name_map_t {
    /* Name of the handle */
    char *map_name;

    /* Handle that the name corresponds to.  Will be 0/NULL if there
       is no corresponding back-end object. */
    mqs_taddr_t map_handle;
};

/* MPI attribute / value pairs.  Include both a numeric and string
   key; pre-defined MPI keyvals (e.g., MPI_TAG_MAX) have a
   human-readable string name.  The string will be NULL for
   non-predefined keyvals. 

   The int keyval member is last to avoid "holes" in the memory
   layout. */
struct mpidbg_attribute_pair_t {
    /* Keyval name; will be non-NULL for attributes that have a
       human-readable name (i.e., MPI predefined keyvals) */
    char *keyval_name;
    /* Value */
    char *value;
    /* Keyval */
    int keyval;
};

/*-----------------------------------------------------------------------
 * Communicators
 *-----------------------------------------------------------------------*/

/* Using an enum instead of #define because debuggers can show the
   *names* of enum values, not just the values. */
enum mpidbg_comm_capabilities_t {
    /* Whether this MPI DLL supports returning basic information about
       communicators */
    MPIDBG_COMM_CAP_BASIC =                  0x01,
    /* Whether this MPI DLL supports returning names of
       communicators */
    MPIDBG_COMM_CAP_STRING_NAMES =           0x02,
    /* Whether this MPI DLL supports indicating whether a communicator
       has been freed by the user application */
    MPIDBG_COMM_CAP_FREED_HANDLE =           0x04,
    /* Whether this MPI DLL supports indicating whether a communicator
       object has been freed by the MPI implementation or not */
    MPIDBG_COMM_CAP_FREED_OBJECT =           0x08,
    /* Whether this MPI DLL supports returning the list of MPI request
       handles that are pending on a communicator */
    MPIDBG_COMM_CAP_REQUEST_LIST =           0x10,
    /* Whether this MPI DLL supports returning the list of MPI window
       handles that were derived from a given communicator */
    MPIDBG_COMM_CAP_WINDOW_LIST =            0x20,
    /* Whether this MPI DLL supports returning the list of MPI file
       handles that were derived from a given communicator */
    MPIDBG_COMM_CAP_FILE_LIST =              0x40,
    /* Sentinel max value */
    MPIDBG_COMM_CAP_MAX
};

enum mpidbg_comm_info_bitmap_t {
    /* Predefined communicator if set (user-defined if not set) */
    MPIDBG_COMM_INFO_PREDEFINED =      0x001,
    /* Whether this communicator is a cartesian communicator or not
       (mutually exclusive with _GRAPH and _INTERCOMM) */
    MPIDBG_COMM_INFO_CARTESIAN =       0x002,
    /* Whether this communicator is a graph communicator or not
       (mutually exclusive with _CARTESIAN and _INTERCOMM) */
    MPIDBG_COMM_INFO_GRAPH =           0x004,
    /* If a cartesian or graph communicator, whether the processes in
       this communicator were re-ordered when the topology was
       assigned. */
    MPIDBG_COMM_INFO_TOPO_REORDERED =  0x008,

    /* Whether this is an intercommunicator or not (this communicator
       is an intracommunicator if this flag is not yet). */
    MPIDBG_COMM_INFO_INTERCOMM =       0x010,

    /* This communicator has been marked for freeing by the user
       application if set */
    MPIDBG_COMM_INFO_FREED_HANDLE =    0x020,
    /* This communicator has actually been freed by the MPI
       implementation if set */
    MPIDBG_COMM_INFO_FREED_OBJECT =    0x040,
    /* The queried communicator is MPI_COMM_NULL */
    MPIDBG_COMM_INFO_COMM_NULL =       0x080,

    /* The queried communicator is a C handle */
    MPIDBG_COMM_INFO_HANDLE_C =        0x100,
    /* The queried communicator is a C++ handle */
    MPIDBG_COMM_INFO_HANDLE_CXX =      0x200,
    /* The queried communicator is a F77/F90 integer handle */
    MPIDBG_COMM_INFO_HANDLE_FINT =     0x400,

    /* Sentinel max value */
    MPIDBG_COMM_INFO_MAX
};

/* Predefined handle -> address mappings.  This enum is the index to
   an array indicating the handle addresses of the 4 pre-defined
   communicators.  */
enum mpidbg_predefined_comm_t {
    MPIDBG_COMM_WORLD,
    MPIDBG_COMM_SELF,
    MPIDBG_COMM_PARENT,
    MPIDBG_COMM_NULL,
    MPIDBG_COMM_MAX
};

/* When a communicator is looked up in an MPI process, the following
   handle is returned.  This handle can be used as a "base class" by
   the DLL to cache additional information, if desired.  This handle
   is a distinct type (rather than, for example, a typedef to (void*))
   to provide compile-time checking, ensuring that handles are not
   queried from one type and used with another. */
struct mpidbg_comm_handle_t {
    /* Image that this handle is in */
    mqs_image *image;
    mqs_image_info *image_info;

    /* Process that this handle is in */
    mqs_process *process;
    mqs_process_info *process_info;

    /* Value of the communicator handle in the MPI process (passed in
       via mpidbg_comm_query()) */
    mqs_taddr_t c_comm;
};

/*-----------------------------------------------------------------------
 * Requests
 *-----------------------------------------------------------------------*/

/* Using an enum instead of #define because debuggers can show the
   *names* of enum values, not just the values. */
enum mpidbg_request_capabilities_t {
    /* Whether this MPI DLL supports returning basic information about
       requests */
    MPIDBG_REQUEST_CAP_BASIC =           0x01,
    /* Sentinel max value */
    MPIDBG_REQUEST_CAP_MAX
};

enum mpidbg_request_info_bitmap_t {
    /* Predefined request if set (user-defined if not set) */
    MPIDBG_REQUEST_INFO_PREDEFINED =      0x01,
    /* Sentinel max value */
    MPIDBG_REQUEST_INFO_MAX
};

/* See note about mpidbg_comm_handle_t, above */
struct mpidbg_request_handle_t {
    /* Image that this handle is in */
    mqs_image *image;
    mqs_image_info *image_info;

    /* Process that this handle is in */
    mqs_process *process;
    mqs_process_info *process_info;

    /* Value of the request handle in the MPI process (passed in via
       mpidbg_request_query()) */
    mqs_taddr_t c_request;
};

/*-----------------------------------------------------------------------
 * Statuses
 *-----------------------------------------------------------------------*/

enum mpidbg_status_capabilities_t {
    /* Whether this MPI DLL supports returning basic information about
       statuses */
    MPIDBG_STATUS_CAP_BASIC =           0x01,
    /* Sentinel max value */
    MPIDBG_STATUS_CAP_MAX
};

enum mpidbg_status_info_bitmap_t {
    /* Predefined status if set (user-defined if not set) */
    MPIDBG_STATUS_INFO_PREDEFINED =      0x01,
    /* Sentinel max value */
    MPIDBG_STATUS_INFO_MAX
};

/* See note about mpidbg_comm_handle_t, above */
struct mpidbg_status_handle_t {
    /* Image that this handle is in */
    mqs_image *image;
    mqs_image_info *image_info;

    /* Process that this handle is in */
    mqs_process *process;
    mqs_process_info *process_info;

    /* Value of the status handle in the MPI process (passed in
       via mpidbg_status_query()) */
    mqs_taddr_t c_status;
};

/*-----------------------------------------------------------------------
 * Error handlers
 *-----------------------------------------------------------------------*/

/* Using an enum instead of #define because debuggers can show the
   *names* of enum values, not just the values. */
enum mpidbg_errhandler_capabilities_t {
    /* Whether this MPI DLL supports returning basic information about
       error handlers */
    MPIDBG_ERRH_CAP_BASIC =           0x01,
    /* Whether this MPI DLL supports returning names of the predefined
       error handlers */
    MPIDBG_ERRH_CAP_STRING_NAMES =    0x02,
    /* Whether this MPI DLL supports indicating whether an error
       handler has been freed by the user application */
    MPIDBG_ERRH_CAP_FREED_HANDLE =    0x04,
    /* Whether this MPI DLL supports indicating whether an error
       handler object has been freed by the MPI implementation or
       not */
    MPIDBG_ERRH_CAP_FREED_OBJECT =    0x08,
    /* Whether this MPI DLL supports returning the list of MPI handles
       that an MPI error handler is attached to */
    MPIDBG_ERRH_CAP_HANDLE_LIST =     0x10,
    /* Sentinel max value */
    MPIDBG_ERRH_CAP_MAX
};

enum mpidbg_errhandler_info_bitmap_t {
    /* Predefined error handler if set (user-defined if not set) */
    MPIDBG_ERRH_INFO_PREDEFINED =      0x01,
    /* Communicator error handler if set */
    MPIDBG_ERRH_INFO_COMMUNICATOR =    0x02,
    /* File error handler if set */
    MPIDBG_ERRH_INFO_FILE =            0x04,
    /* Window error handler if set */
    MPIDBG_ERRH_INFO_WINDOW =          0x08,
    /* Callback is in C if set */
    MPIDBG_ERRH_INFO_C_CALLBACK =      0x10,
    /* Callback is in Fortran if set */
    MPIDBG_ERRH_INFO_FORTRAN_CALLBACK =0x20,
    /* Callback is in C++ if set */
    MPIDBG_ERRH_INFO_CXX_CALLBACK =    0x40,
    /* This errorhandler has been marked for freeing by the user
       application if set */
    MPIDBG_ERRH_INFO_FREED_HANDLE =    0x80,
    /* This errorhandler has actually been freed by the MPI
       implementation if set */
    MPIDBG_ERRH_INFO_FREED_OBJECT =    0x100,
    /* Sentinel max value */
    MPIDBG_ERRH_INFO_MAX
};

/* Predefined handle -> address mappings.  This enum is the index to
   an array indicating the handle addresses of the pre-defined
   errhandlers.  */
enum mpidbg_predefined_errhandler_t {
    MPIDBG_ERRHANDLER_ARE_FATAL,
    MPIDBG_ERRHANDLER_RETURN,
    MPIDBG_ERRHANDLER_THROW_EXCEPTIONS,
    MPIDBG_ERRHANDLER_NULL,
    MPIDBG_ERRHANDLER_MAX
};

/* See note about mpidbg_comm_handle_t, above */
struct mpidbg_errhandler_handle_t {
    /* Image that this handle is in */
    mqs_image *image;
    mqs_image_info *image_info;

    /* Process that this handle is in */
    mqs_process *process;
    mqs_process_info *process_info;

    /* Value of the errhandler handle in the MPI process (passed in
       via mpidbg_errhandler_query()) */
    mqs_taddr_t c_errhandler;
};

/**************************************************************************
 * Global variables
 *
 * mpidbg_dll_locations is in the MPI application; all others are in
 * the DLL.
 **************************************************************************/

/* Array of filenames instantiated IN THE MPI APPLICATION (*NOT* in
   the DLL) that provides an set of locations where DLLs may be found.
   The last pointer in the array will be a NULL sentinel value.  The
   debugger can scan the entries in the array, find one that matches
   the debugger (i.e., whether the dlopen works or not), and try to
   dynamically open the dl_filename.  Notes:

   1. It is not an error if a dl_filename either does not exist or is
      otherwise un-openable (the debugger can just try the next
      match).
   2. This array values are not valid until MPIR_Breakpoint.
   3. If a filename is absolute, the debugger will attempt to load
      exactly that.  If the filename is relative, the debugger may try
      a few prefix variations to find the DLL.
 */
extern char **mpidbg_dll_locations;

/* Global variable *in the DLL* describing the DLL's capabilties with
   regards to communicators.  This value is valid after a successfull
   call to mpidbg_init_per_process(). */
extern enum mpidbg_comm_capabilities_t mpidbg_comm_capabilities;

/* Global variable *in the DLL* that is an array of MPI communicator
   handle names -> handle mappings (the last entry in the array is
   marked by a NULL string value).  For example, MPI_COMM_WORLD may
   not appear as a symbol in an MPI process, but the debugger needs to
   be able to map this name to a valid handle.  MPI implementations
   not requiring this mapping can either have a NULL value for this
   variable or have a single entry that has a NULL string value.  This
   variable is not valid until after a successfull call to
   mpidbg_init_per_process().  */
extern mqs_taddr_t mpidbg_predefined_comm_map[MPIDBG_COMM_MAX];

/* Global variable *in the DLL* describing the DLL's capabilties with
   regards to error handlers.  This value is valid after a successfull
   call to mpidbg_init_per_process(). */
extern enum mpidbg_errhandler_capabilities_t mpidbg_errhandler_capabilities;

/* Global variable *in the DLL* that is an array of MPI error handler
   handle names -> handle mappings.  It is analogous to
   mpidbg_predefined_comm_map; see above for details. */
extern mqs_taddr_t mpidbg_predefined_errhandler_map[MPIDBG_ERRHANDLER_MAX];

/**************************************************************************
 * Functions
 **************************************************************************/

/*-----------------------------------------------------------------------
 * DLL infrastructure functions
 *-----------------------------------------------------------------------*/

/* This function must be called once before any other mpidbg_*()
   function is called, and before most other global mpidbg_* data is
   read.  It is only necessary to call this function once for a given
   debugger instantiation.  This function will initialize all mpidbg
   global state, to include setting all relevant global capability
   flags.

   Parameters:

   IN: callbacks: Table of pointers to the debugger functions. The DLL
                  need only save the pointer, the debugger promises to
                  maintain the table of functions valid for as long as
                  needed.  The table remains the property of the
                  debugger, and should not be altered or deallocated
                  by the DLL. This applies to all of the callback
                  tables.

   This function will return:

   MPIDBG_SUCCESS: if all initialization went well
   MPIDBG_ERR_*: if something went wrong.
*/
int mpidbg_init_once(const mqs_basic_callbacks *callbacks);

/*-----------------------------------------------------------------------*/

/* Query the DLL to find out what version of the interface it
   supports. 

   Parameters:

   None.

   This function will return:

   MPIDBG_INTERFACE_VERSION
*/

int mpidbg_interface_version_compatibility(void);

/*-----------------------------------------------------------------------*/

/* Returns a string describing this DLL.

   Parameters: 

   None

   This function will return:

   A null-terminated string describing this DLL.
*/   
char *mpidbg_version_string(void);

/*-----------------------------------------------------------------------*/

/* Returns the address width that this DLL was compiled with.

   Parameters: 

   None

   This function will return:

   sizeof(mqs_taddr_t)
*/

int mpidbg_dll_taddr_width(void);

/*-----------------------------------------------------------------------*/

/* Setup debug information for a specific image, this must save the
   callbacks (probably in the mqs_image_info), and use those functions
   for accessing this image.

   The DLL should use the mqs_put_image_info and mqs_get_image_info
   functions to associate whatever information it wants to keep with
   the image (e.g., all of the type offsets it needs could be kept
   here).  The debugger will call mqs_destroy_image_info when it no
   longer wants to keep information about the given executable.
 
   This will be called once for each executable image in the parallel
   job.

   Parameters:

   IN: image: the application image.
   IN: callbacks: Table of pointers to the debugger image-specific
                  functions. The DLL need only save the pointer, the
                  debugger promises to maintain the table of functions
                  valid for as long as needed.  The table remains the
                  property of the debugger, and should not be altered
                  or deallocated by the DLL. This applies to all of
                  the callback tables.
   IN/OUT: handle_types: a pointer to a pre-allocated struct
                         containing mqs_types for each of the MPI
                         handle types.  Must be filled in with results
                         from mqs_find_type for each MPI handle type.

   This function will return:

   MPIDBG_SUCCESS: if all initialization went well
   MPIDBG_ERR_NOT_SUPPORTED: if the image does not support the MPIDBG
                   interface.  In this case, no other mpidbg functions
                   will be invoked on this image (not even
                   mpidbg_finalize_per_image()).
   MPIDBG_ERR_*: if something went wrong.
*/
int mpidbg_init_per_image(mqs_image *image,
                          const mqs_image_callbacks *callbacks,
                          struct mpidbg_handle_info_t *handle_types);

/* This function will be called once when an application image that
   previously had mpidbg_init_per_image() successfully invoked that is
   now ending (e.g., the debugger is exiting, the debugger has
   unloaded this image, etc.).  This function can be used to clean up
   any image-specific data.

   Parameters:

   IN: image: the application image.
   IN: image_info: the info associated with the application image.
*/
void mpidbg_finalize_per_image(mqs_image *image, mqs_image_info *image_info);

/*-----------------------------------------------------------------------*/

/* This function will only be called if mpidbg_init_per_image()
   returned successfully, indicating that the image contains
   information for MPI handle information.  If you cannot tell whether
   a process will have MPI handle information in it by examining the
   image, you should return SUCCESS from mpidbg_init_per_image() and
   use this function to check whether MPI handle information is
   available in the process.

   Set up whatever process specific information we need.  For instance,
   addresses of global variables should be handled here rather than in
   the image information, because if data may be in dynamic libraries
   which could end up mapped differently in different processes.

   Note that certain global variables are not valid until after this
   call completes successfully (see above; e.g.,
   mpidbg_comm_capabilities, mpidbg_comm_name_mapping, etc.).

   Parameters:

   IN: process: the process
   IN: callbacks: Table of pointers to the debugger process-specific
                  functions. The DLL need only save the pointer, the
                  debugger promises to maintain the table of functions
                  valid for as long as needed.  The table remains the
                  property of the debugger, and should not be altered
                  or deallocated by the DLL. This applies to all of
                  the callback tables.
   IN/OUT: handle_types: the same handle_types that was passed to
                         mqs_init_per_image.  It can be left unaltered
                         if the results from mqs_init_per_image were
                         sufficient, or modified if necessary to be
                         specific to this process.

   This function will return:

   MPIDBG_SUCCESS: if all initialization went well
   MPIDBG_ERR_NOT_SUPPORTED: if the process does not support the MPIDBG
                   interface.  In this case, no other mpidbg functions
                   will be invoked on this image (not even
                   mpidbg_finalize_per_process()).
   MPIDBG_ERR_*: if something went wrong.
*/
int mpidbg_init_per_process(mqs_process *process, 
                            const mqs_process_callbacks *callbacks,
                            struct mpidbg_handle_info_t *handle_types);

/* This function will be called once when an application image that
   previously had mpidbg_init_per_process() successfully invoked that
   is now ending (e.g., the debugger is exiting, the debugger has
   stopped executing this process, etc.).  This function can be used
   to clean up any process-specific data.

   Parameters:

   IN: process: the application process.
   IN: process_info: the info associated with the application process.
*/
void mpidbg_finalize_per_process(mqs_process *process,
                                 mqs_process_info *process_info);

/*-----------------------------------------------------------------------
 * MPI handle query functions
 * MPI_Comm
 *-----------------------------------------------------------------------*/

/* Query a specific MPI_Comm handle and, if found and valid, return a
   handle that can subsequently be queried for a variety of
   information about this communicator.

   Note that the returned handle is only valid at a specific point in
   time.  If the MPI process advances after the handle is returned,
   the handle should be considered stale and should therefore be
   freed.  This function should be invoked again to obtain a new
   handle.

   The intent behind this design (query the communicator once and
   return a handle for subsequent queries) is to allow a DDL to scan
   the MPI process *once* for all the relevant information about the
   communicator and cache it locally (presumably on the returned
   handle).  The subsequent queries are then all local, not involving
   the probing the MPI process -- which chould be somewhat cheaper /
   faster / easier to implement.

   Of course, a DLL is free to cache only the communicator value in
   the handle and actually probe the MPI process in any of the
   subsequent query functions, if desired.  To be clear: this design
   *allows* for the pre-caching of all the MPI process communicator
   data, but does not mandate it.

   Upon successful return (i.e., returning MPIDBG_SUCCESS), the
   returned comm_handle->c_comm will equal the c_comm parameter value.
   The caller must also eventually free the returned handle via
   mpidbg_comm_handle_free().

   Parameters:

   IN: image: image
   IN: image_info: image info that was previously "put"
   IN: process: process
   IN: process_info: process info that was previously "put"
   IN: comm: communicator handle
   OUT: comm_handle: handle to be passed to the query functions (below)

   This function will return:

   MPIDBG_SUCCESS: if the communicator handle is valid, was found, and
                   the OUT parameter was filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_comm_query(mqs_image *image, 
                      mqs_image_info *image_info, 
                      mqs_process *process, 
                      mqs_process_info *process_info,
                      mqs_taddr_t comm, 
                      struct mpidbg_comm_handle_t **handle);

/* Free a handle returned by the mpidbg_comm_query() function.

   Parameters:

   IN: handle: handle previously returned by mpidbg_comm_query()

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid and was freed successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
*/
int mpidbg_comm_handle_free(struct mpidbg_comm_handle_t *handle);

/* Query a handle returned by mpidbg_comm_query() and, if found and
   valid, return basic information about the original communicator.

   Parameters:

   IN: comm_handle: handle returned by mpidbg_comm_query()
   OUT: comm_name: string name of the communicator, max of
        MPIDBG_MAX_OBJECT_NAME characters, \0-terminated if less than
        MPIDBG_MAX_OBJECT_NAME characters.
   OUT: comm_bitflags: bit flags describing the communicator
   OUT: comm_rank: rank of this process in this communicator
   OUT: comm_size: total number of processes in this communicator
   OUT: comm_fortran_handle: INTEGER Fortran handle corresponding to
        this communicator, or MPIDBG_ERR_UNAVAILABLE if currently
        unavailable, or MPIDBG_ERR_NOT_SUPPORTED if not supported
   OUT: comm_cxx_handle: Pointer to C++ handle corresponding to
        this communicator, or MPIDBG_ERR_UNAVAILABLE if currently
        unavailable, or MPIDBG_ERR_NOT_SUPPORTED if not supported

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_comm_query_basic(struct mpidbg_comm_handle_t *handle,
                            char comm_name[MPIDBG_MAX_OBJECT_NAME],
                            enum mpidbg_comm_info_bitmap_t *comm_bitflags,
                            int *comm_rank,
                            int *comm_size,
                            int *comm_fortran_handle,
                            mqs_taddr_t *comm_cxx_handle);

/* Query a handle returned by mpidbg_comm_query() and, if found and
   valid, return information about the MPI processes in this
   communicator.

   All arrays returned in OUT variables are allocated by this
   function, but are the responsibility of the caller to be freed.

   Parameters:

   IN: comm_handle: handle returned by mpidbg_comm_query()
   OUT: comm_num_local_procs: filled with the length of the
        comm_local_procs OUT array.
   OUT: comm_local_procs: filled with a pointer to an array of
        mpidbg_process_t instances describing the local processes in
        this communicator.  
   OUT: comm_num_remote_procs: filled with the length of the
        comm_remote_procs OUT array.  Will be 0 if the communicator is
        not an intercommunicator.
   OUT: comm_remote_procs: filled with a pointer to an array of
        mpidbg_process_t instances describing the local processes in
        this communicator.  Will be NULL if the communicator is not an
        intercommunicator.

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_comm_query_procs(struct mpidbg_comm_handle_t *handle,
                            int *comm_num_local_procs,
                            struct mpidbg_process_t **comm_local_procs,
                            int *comm_num_remote_procs,
                            struct mpidbg_process_t **comm_remote_procs);

/* Query a handle returned by mpidbg_comm_query() and, if found and
   valid, return information about the topology associated with the
   communicator (if any).  It is not an error to call this function
   with communicators that do not have associated topologies; such
   communicators will still return MPIDBG_SUCCESS but simply return a
   value of 0 in the comm_out_length OUT parameter.

   All arrays returned in OUT variables are allocated by this
   function, but are the responsibility of the caller to be freed.

   Parameters:

   IN: comm_handle: handle returned by mpidbg_comm_query()
   OUT: comm_out_length: 
        - For cartesian communicators, filled with the number of
          dimensions.
        - For graph communicators, filled with the number of nodes.
        - For all other communicators, filled with 0.
   OUT: comm_cart_dims_or_graph_indexes: 
        - For cartesian communicators, filled with a pointer to an
          array of length *comm_out_length representing the dimenstion
          lengths.
        - For graph communicators, filled with a pointer to an array
          of length *comm_out_length representing the node degrees.
        - For all other communicators, filled with NULL.
   OUT: comm_cart_periods_or_graph_edges:
        - For cartesian communicators, filled with a pointer to an
          array of length *comm_out_length representing whether each
          dimension is periodic or not.
        - For graph communicators, filled with a pointer to an array
          of length *comm_out_length representing the array of edges.
        - For all other communicators, filled with NULL.

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_comm_query_topo(struct mpidbg_comm_handle_t *handle,
                           int *comm_out_length,
                           int **comm_cart_dims_or_graph_indexes,
                           int **comm_cart_periods_or_graph_edges);

/* Query a handle returned by mpidbg_comm_query() and, if found and
   valid, return information about the attributes associated with the
   communicator (if any).  It is not an error to call this function
   with communicators that do not have associated attributes; such
   communicators will still return MPIDBG_SUCCESS but simply return a
   value of 0 in the comm_attrs_length OUT parameter.

   All arrays returned in OUT variables are allocated by this
   function, but are the responsibility of the caller to be freed.

   Parameters:

   IN: comm_handle: handle returned by mpidbg_comm_query()
   OUT: comm_num_attrs: length of the array returned in com_attrs;
         if 0, the value of com_attrs is undefined.
   OUT: comm_attrs: array of length comm_attrs_length containing
         keyval/value pairs.

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_comm_query_attrs(struct mpidbg_comm_handle_t *comm_handle,
                            int *comm_num_attrs,
                            struct mpidbg_attribute_pair_t *comm_attrs);

/* Query a handle returned by mpidbg_comm_query() and, if found and
   valid, return an array of pending requests on this communicator.

   All arrays returned in OUT variables are allocated by this
   function, but are the responsibility of the caller to be freed.

   Parameters:

   IN: comm_handle: handle returned by mpidbg_comm_query()
   OUT: comm_num_requests: filled with the length of the
        comm_pending_requests array.
   OUT: comm_requests: filled with a pointer to an array of pending
        requests on this communicator.

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_comm_query_requests(struct mpidbg_comm_handle_t *handle,
                               int *comm_num_requests,
                               mqs_taddr_t **comm_pending_requests);

/* Query a handle returned by mpidbg_comm_query() and, if found and
   valid, return arrays of MPI_File and MPI_Win handles that were
   derived from this communicator.

   All arrays returned in OUT variables are allocated by this
   function, but are the responsibility of the caller to be freed.

   Parameters:

   IN: comm_handle: handle returned by mpidbg_comm_query()
   OUT: comm_num_derived_files: filled with the length of the
        comm_derived_files array.
   OUT: comm_derived_files: filled with a pointer to an array of
        MPI_File handles derived from this communicator.
   OUT: comm_num_derived_windows: filled with the length of the
        comm_derived_windows array.
   OUT: comm_derived_windows: filled with a pointer to an array of
        MPI_Win handles derived from this communicator.

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_comm_query_derived(struct mpidbg_comm_handle_t *handle,
                              int *comm_num_derived_files,
                              mqs_taddr_t **comm_derived_files,
                              int *comm_num_derived_windows,
                              mqs_taddr_t **comm_derived_windows);

/*-----------------------------------------------------------------------
 * MPI handle query functions
 * MPI_Errhandler
 *-----------------------------------------------------------------------*/

/* These functions are analogous to the mpidbg_comm_* functions, but
   for MPI_Errhandler. */
int mpidbg_errhandler_query(mqs_image *image, 
                            mqs_image_info *image_info, 
                            mqs_process *process, 
                            mqs_process_info *process_info,
                            mqs_taddr_t errhandler,
                            struct mpidbg_errhandler_handle_t **handle);

/* Free a handle returned by the mpidbg_errhandler_query() function.

   Parameters:

   IN: handle: handle previously returned by mpidbg_errhandler_query()

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid and was freed successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
*/
int mpidbg_errhandler_handle_free(struct mpidbg_errhandler_handle_t *handle);

/* Query a handle returned by mpidbg_comm_query() and, if found and
   valid, return basic information about the original communicator.

   Parameters:

   IN: comm_handle: handle returned by mpidbg_comm_query()
   OUT: comm_name: string name of the communicator, max of
        MPIDBG_MAX_OBJECT_NAME characters, \0-terminated if less than
        MPIDBG_MAX_OBJECT_NAME characters.
   OUT: comm_bitflags: bit flags describing the communicator
   OUT: comm_rank: rank of this process in this communicator
   OUT: comm_size: total number of processes in this communicator
   OUT: comm_fortran_handle: INTEGER Fortran handle corresponding to
        this communicator, or MPIDBG_ERR_UNAVAILABLE if currently
        unavailable, or MPIDBG_ERR_NOT_SUPPORTED if not supported
   OUT: comm_cxx_handle: Pointer to C++ handle corresponding to
        this communicator, or MPIDBG_ERR_UNAVAILABLE if currently
        unavailable, or MPIDBG_ERR_NOT_SUPPORTED if not supported

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_errhandler_query_basic(struct mpidbg_errhandler_handle_t *handle,
                                  char errhandler_name[MPIDBG_MAX_OBJECT_NAME],
                                  enum mpidbg_errhandler_info_bitmap_t *errhandler_bitflags,
                                  int *errhandler_fortran_handle,
                                  mqs_taddr_t *errhandler_cxx_handle);

/* Query a handle returned by mpidbg_errhandler_query() and, if found
   and valid, return an array of MPI handle that are using this error
   handler.  The type of the MPI handle returned will be indicated by
   one of the following set in the errhandler bitmap flags:

   - MPIDBG_ERRH_INFO_COMMUNICATOR
   - MPIDBG_ERRH_INFO_FILE
   - MPIDBG_ERRH_INFO_WINDOW

   All arrays returned in OUT variables are allocated by this
   function, but are the responsibility of the caller to be freed.

   Parameters:

   IN: errhandler_handle: handle returned by mpidbg_errhandler_query()
   OUT: errhandler_num_handles: filled with the length of the
        errhandler_derived_files array.
   OUT: erhandler_handles: filled with a pointer to an array of
        MPI handles that are using this errhandler.

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_errhandler_query_handles(struct mpidbg_errhandler_handle_t *handle,
                                    int *errhandler_num_handles,
                                    mqs_taddr_t **errhandler_handles);

/* Query a handle returned by mpidbg_errhandler_query() and, if found
   and valid, return an array of MPI handle that are using this error
   handler.  The signature of the function pointer returned is
   determined by the combintion of errhandler bitmap flags:

   - MPIDBG_ERRH_INFO_COMMUNICATOR, or MPIDBG_ERRH_INFO_FILE, or
     MPIDBG_ERRH_INFO_WINDOW
   - MPIDBG_ERRH_INFO_C_CALLBACK, or MPIDBG_ERRH_INFO_FORTRAN_CALLBACK, or
     MPIDBG_ERRH_INFO_CXX_CALLBACK

   Note that the output function pointer will be NULL if
   MPIDBG_ERRH_INFO_PREDEFINED is set on the errhandler bitmap flags.

   Parameters:

   IN: errhandler_handle: handle returned by mpidbg_errhandler_query()
   OUT: errhandler_func_ptr: filled with the function pointer to the
        callback function.

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid, was found, and the OUT
                   parameters were filled in successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
   MPIDBG_ERR_UNSUPPORTED: if this function is unsupported.
*/
int mpidbg_errhandler_query_callback(struct mpidbg_errhandler_handle_t *handle,
                                     mqs_taddr_t *errhandler_func_ptr);

/*-----------------------------------------------------------------------
 * MPI handle query functions
 * MPI_Request
 *-----------------------------------------------------------------------*/

/* These functions are analogous to the mpidbg_comm_* functions, but
   for MPI_Request. */
int mpidbg_request_query(mqs_image *image, 
                         mqs_image_info *image_info, 
                         mqs_process *process, 
                         mqs_process_info *process_info,
                         mqs_taddr_t request,
                         struct mpidbg_request_handle_t **handle);

/* Free a handle returned by the mpidbg_request_query() function.

   Parameters:

   IN: handle: handle previously returned by mpidbg_request_query()

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid and was freed successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
*/
int mpidbg_request_handle_free(struct mpidbg_request_handle_t *handle);

/*-----------------------------------------------------------------------
 * MPI handle query functions
 * MPI_Status
 *-----------------------------------------------------------------------*/

/* These functions are analogous to the mpidbg_comm_* functions, but
   for MPI_Status. */
int mpidbg_status_query(mqs_image *image, 
                        mqs_image_info *image_info, 
                        mqs_process *process, 
                        mqs_process_info *process_info,
                        mqs_taddr_t status,
                        struct mpidbg_status_handle_t **handle);

/* Free a handle returned by the mpidbg_status_query() function.

   Parameters:

   IN: handle: handle previously returned by mpidbg_status_query()

   This function will return:

   MPIDBG_SUCCESS: if the handle is valid and was freed successfully.
   MPIDBG_ERR_NOT_FOUND: if the handle is not valid / found.
*/
int mpidbg_status_handle_free(struct mpidbg_status_handle_t *handle);

#endif /* __MPIDBG_INTERFACE_H__ */
