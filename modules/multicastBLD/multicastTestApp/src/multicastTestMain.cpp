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
	int iTestType = ( argc >= 2 ? atoi(argv[1]) : 0 );
	char* sInterfaceIp = ( argc >= 3 ? argv[2] : NULL );
	
	testBldClient( iTestType, sInterfaceIp );
    //if(argc>=2) {    
    //    iocsh(argv[1]);
    //    epicsThreadSleep(.2);
    //}
    //iocsh(NULL);
    return(0);
}
