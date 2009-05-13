/* mutlicastTestMain.cpp */
/* Author:  Bruce Hill Date:    May 8, 2009 */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "epicsThread.h"
#include "iocsh.h"
#include "multicastTestBld.h"

int main(int argc,char *argv[])
{
	testMulticast( argc >= 1 ? argv[1] : NULL );
    //if(argc>=2) {    
    //    iocsh(argv[1]);
    //    epicsThreadSleep(.2);
    //}
    //iocsh(NULL);
    return(0);
}
