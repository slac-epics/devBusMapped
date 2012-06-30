#ifndef DEV_BUS_MAPPED_SUPPORT_H
#define DEV_BUS_MAPPED_SUPPORT_H
/* $Id: devBusMapped.h,v 1.1.1.2 2010/04/27 16:06:49 strauman Exp $ */

/* Unified device support for simple, bus-mapped device registers */

/* A helper routine to parse the INP link field. Since there
 * are not too many link types, we must use VME for everything :-(
 * The idea is as follows:
 * 
 *  The INP/OUT link field is set to something like:
 *  #C<inst> S<shift> @<theDevice>+<offset>,<method>
 *
 *  <theDevice> is a device name string that is looked-up in the
 *  registry, i.e. the device driver must have stored it there
 *  (using the 'devBusMappedRegister()' wrapper).
 *
 *  The device may support multiple (identical) 'subdevices' or
 *  'channels' which can be found at
 *
 *   <base address> + (<inst> << <shift>)
 *
 *  Finally, the <offset> is added to calculate the real register
 *  address.
 *
 *  <method> may be any of 'be8[s]','be16[s]','be32','le16[s]','le32',
 *  'm8', 'm16[s]', or 'm32[s]' to specify the access method that
 *  should be used (big/little/cpu endian, data width; the optional
 *  's' [e.g. le16s] indicates that the short little endian data are
 *  signed). If no <method> is specified, *  "be32" is assumed.
 *  The 'm' methods are appropriate to access variables in memory
 *  (cpu endianness).
 *
 *  In addition, user supported access methods are supported. A driver
 *  can register its own DevBusMappedAccessRec with a call to
 *  'devBusMappedRegisterIO()' which is then free to do any kind
 *  of fancy things at a low level.
 */

#include <dbCommon.h>
/* Due to EPICS inconsistency an application cannot include
 * both, cadef.h and dbAccess.h (otherwise there will be double
 * defined CPP symbols).
 * Hence we avoid including any of these from here!
#include <dbAccess.h>
 */
#include <dbScan.h>
#include <recGbl.h>
#include <epicsMutex.h>
#include <epicsTypes.h>
#include <link.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DevBusMappedPvtRec_ *DevBusMappedPvt;
typedef struct DevBusMappedAccessRec_ *DevBusMappedAccess;

/* Read and write methods which are used by the device support 'read' and 'write'
 * routines.
 */
typedef int (*DevBusMappedRead)(DevBusMappedPvt pvt, epicsUInt32 *pvalue, dbCommon *prec);
typedef int (*DevBusMappedWrite)(DevBusMappedPvt pvt, epicsUInt32 value, dbCommon *prec);

typedef struct DevBusMappedAccessRec_ {
	DevBusMappedRead	rd;		/* read access routine				 */
	DevBusMappedWrite	wr;		/* read access routine				 */
} DevBusMappedAccessRec;

/* invoke the access methods and raise alarms if the access
 * fails.
 */
int
devBusMappedGetVal(DevBusMappedPvt pvt, epicsUInt32 *pvalue, dbCommon *prec);

int
devBusMappedPutVal(DevBusMappedPvt pvt, epicsUInt32 value, dbCommon *prec);

/* "per-device" information kept in the registry */
typedef struct DevBusMappedDevRec_ {
	volatile void *baseAddr;
	epicsMutexId  mutex;		/* any other driver/devSup sharing registers
								 * with devBusMapped MUST LOCK THIS MUTEX when
                                 * performing modifications or non-atomical reads.
								 */
	void          *udata;		/* for use by the driver / user */
	const char    *name;
} DevBusMappedDevRec, *DevBusMappedDev;

/* Data "private" to the 'devBusMapped' device support. This goes
 * into a struct to be attached to the record's 'DPVT' field.
 * (In most cases, it's the only thing to be attached there - that's
 * what happens if you call 'devBusVmeLinkInit()' below with a
 * NULL 'pvt' argument. However, special device support could
 * make the 'DevBusMappedPvtRec' part of a bigger struct and pass
 * a pointer to the 'DevBusMappedPvtRec' subpart to 'devBusVmeLinkInit()'.)
 *
 * Nobody should access fields in this structure directly.
 */
typedef struct DevBusMappedPvtRec_ {
	dbCommon			*prec;	/* record this devsup is attached to */
	DevBusMappedAccess	acc;	/* pointer to access methods         */
	DevBusMappedDev		dev;	/* per-device info                   */
	IOSCANPVT			scan;	/* io intr scan list for 'prec'      */
	void				*udata;	/* private data for access methods   */
	volatile void		*addr;	/* reg. address (offset from base)   */
} DevBusMappedPvtRec;

/* Parse the link in *l and setup the pvt structure; the
 * caller may pass a preallocated pvt struct.
 * If she passes 'pvt==NULL' a PvtRec is allocated and attached
 * to prec->dpvt. If pvt is non-NULL, it is _not_ attached.
 *
 * (Reason for passing 'l' is that we don't know whether prec has
 * 'inp' or 'out'...)
 *
 * RETURNS: 0 on success nonzero on error (no base address found, parsing
 *          error etc.)
 */

unsigned long
devBusVmeLinkInit(DBLINK *l, DevBusMappedPvt pvt, dbCommon *prec);

/* Register a device's base address and return a pointer to a
 * freshly allocated and registered 'DevBusMappedDev' struct
 * or NULL on failure.
 * 'baseAddress' is the device's base address as seen by the
 * CPU, i.e. VME or PCI base addresses must be properly translated
 * prior to submitting them to this routine.
 */
DevBusMappedDev
devBusMappedRegister(const char *name, volatile void * baseAddress);

/* Register an IO access method; returns 0 on success, nonzero on failure */
int
devBusMappedRegisterIO(const char *name, DevBusMappedAccess accessMethods);

/* Register an IO Intr scan list; returns 0 on success, nonzero on failure */
int
devBusMappedRegisterIOScan(const char *name, IOSCANPVT scan);

/* Find the 'devBusMappedDev' of a registered device by name */
DevBusMappedDev
devBusMappedFind(const char *name);

/* Helper to retrieve the 'scan' (IOSCANPVT) field of a DevBusMappedPvtRec
 * attached to a record's DPVT field.
 */
long
devBusMappedGetIointInfo(int delFrom, dbCommon *prec, IOSCANPVT *ppvt);

/* Print info about 'dev'. If 'dev' is NULL then info about
 * all registered addresses and IO methods is dumped.
 *
 * RETURNS number of entries dumped.
 */
int
devBusMappedDump(DevBusMappedDev dev);

#ifdef __cplusplus
}
#endif

#endif
