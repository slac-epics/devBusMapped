/* File: drvMDrivePlus.cc
         Motor record driver level support for Intelligent Motion Systems, Inc.
         MDrivePlus series; M17, M23, M34.
   DESIGN LIMITATIONS...
         1 - Like all controllers, the MDrivePlus must be powered-on when EPICS is first booted up.
         2 - The MDrivePlus cannot be power cycled while EPICS is up and running.
             The consequences are permanent communication lose with the MDrivePlus until EPICS is rebooted.
*/

#include <string.h>
#include <epicsThread.h>
#include <drvSup.h>
#include "motor.h"
#include "drvIM483.h"
#include "asynOctetSyncIO.h"
#include "epicsExport.h"

#define MDrivePlus_NUM_CARDS    32
#define MAX_AXES        8
#define BUFF_SIZE 13        /* Maximum length of string to/from MDrivePlus */

/*----------------debugging-----------------*/
#ifdef __GNUG__
    #ifdef  DEBUG
    #define Debug(l, f, args...) {if (l <= drvMDrivePlusdebug) printf(f, ## args);}
    #else
    #define Debug(l, f, args...)
    #endif
#else
    #define Debug()
#endif
volatile int drvMDrivePlusdebug = 0;
extern "C" {epicsExportAddress(int, drvMDrivePlusdebug);}

/* --- Local data. --- */
int MDrivePlus_num_cards = 0;
static char *MDrivePlus_axis[] = {"1", "2", "3", "4", "5", "6", "7", "8"};

/* Local data required for every driver; see "motordrvComCode.h" */
#include    "motordrvComCode.h"

/*----------------functions-----------------*/
static int recv_mess(int, char *, int);
static RTN_STATUS send_mess(int, char const *, char *);
static int set_status(int, int);
static long report(int);
static long init();
static int motor_init();
static void query_done(int, int, struct mess_node *);

/*----------------functions-----------------*/

struct driver_table MDrivePlus_access =
{
    motor_init,
    motor_send,
    motor_free,
    motor_card_info,
    motor_axis_info,
    &mess_queue,
    &queue_lock,
    &free_list,
    &freelist_lock,
    &motor_sem,
    &motor_state,
    &total_cards,
    &any_motor_in_motion,
    send_mess,
    recv_mess,
    set_status,
    query_done,
    NULL,
    &initialized,
    MDrivePlus_axis
};

struct
{
    long number;
    long (*report) (int);
    long (*init) (void);
} drvMDrivePlus = {2, report, init};

extern "C" {epicsExportAddress(drvet, drvMDrivePlus);}

static struct thread_args targs = {SCAN_RATE, &MDrivePlus_access, 0.0};


/*********************************************************
 * Print out driver status report
 *********************************************************/
static long report(int level)
{
    int card;

    if (MDrivePlus_num_cards <=0)
    printf("    No MDrivePlus controllers configured.\n");
    else
    {
    for (card = 0; card < MDrivePlus_num_cards; card++)
    {
        struct controller *brdptr = motor_state[card];

        if (brdptr == NULL)
        printf("    MDrivePlus controller %d connection failed.\n", card);
        else
        {
        struct IM483controller *cntrl;

        cntrl = (struct IM483controller *) brdptr->DevicePrivate;
        printf("    MDrivePlus controller #%d, port=%s, id: %s \n", card,
               cntrl->asyn_port, brdptr->ident);
        }
    }
    }
    return(OK);
}


static long init()
{
    /*
     * We cannot call motor_init() here, because that function can do GPIB I/O,
     * and hence requires that the drvGPIB have already been initialized.
     * That cannot be guaranteed, so we need to call motor_init from device
     * support
     */
    /* Check for setup */
    if (MDrivePlus_num_cards <= 0)
    {
    Debug(1, "init(): MDrivePlus driver disabled. MDrivePlusSetup() missing from startup script.\n");
    }
    return((long) 0);
}


static void query_done(int card, int axis, struct mess_node *nodeptr)
{
}


/******************************************************************************
*
* FUNCTION NAME: set_status
*
* LOGIC:
*   Initialize.
*   Send "Moving Status" query.
*   Read response.
*   IF normal response to query.
*       Set communication status to NORMAL.
*   ELSE
*       IF communication status is NORMAL.
*           Set communication status to RETRY.
*           NORMAL EXIT.
*       ELSE
*           Set communication status error.
*           ERROR EXIT.
*       ENDIF
*   ENDIF
*
*   IF "Moving Status" indicates any motion (i.e. status != 0).
*       Clear "Done Moving" status bit.
*   ELSE
*       Set "Done Moving" status bit.
*   ENDIF
*
******************************************************************************/

static int set_status(int card, int signal)
{
    struct IM483controller *cntrl;
    struct mess_node *nodeptr;
    register struct mess_info *motor_info;
    /* Message parsing variables */
    char buff[BUFF_SIZE];
    int rtnval, rtn_state;
    double motorData;
    epicsUInt8 Lswitch;
    bool plusdir, ls_active = false;
    msta_field status;

    cntrl = (struct IM483controller *) motor_state[card]->DevicePrivate;
    input_config *confptr = cntrl->inconfig;
    motor_info = &(motor_state[card]->motor_info[signal]);
    nodeptr = motor_info->motor_motion;
    status.All = motor_info->status.All;

    send_mess(card, "PR MV", MDrivePlus_axis[signal]);
    rtn_state = recv_mess(card, buff, 1);
    if (rtn_state > 0)
    {
    cntrl->status = NORMAL;
    status.Bits.CNTRL_COMM_ERR = 0;
    }
    else
    {
    if (cntrl->status == NORMAL)
    {
        cntrl->status = RETRY;
        rtn_state = OK;
        goto exit;
    }
    else
    {
        cntrl->status = COMM_ERR;
        status.Bits.CNTRL_COMM_ERR = 1;
        status.Bits.RA_PROBLEM     = 1;
        rtn_state = 1;
        goto exit;
    }
    }

    rtnval = atoi(buff);

    status.Bits.RA_DONE = (rtnval != 0) ? 0 : 1;

    /*
     * Parse motor position
     * Position string format: 1TP5.012,2TP1.123,3TP-100.567,...
     * Skip to substring for this motor, convert to double
     */

    send_mess(card, "PR P", MDrivePlus_axis[signal]);
    recv_mess(card, buff, 1);

    motorData = atof(buff);

    if (motorData == motor_info->position)
    {
    if (nodeptr != 0)   /* Increment counter only if motor is moving. */
        motor_info->no_motion_count++;
    }
    else
    {
    epicsInt32 newposition;

    newposition = NINT(motorData);
    status.Bits.RA_DIRECTION = (newposition >= motor_info->position) ? 1 : 0;
    motor_info->position = newposition;
    motor_info->no_motion_count = 0;
    }

    plusdir = (status.Bits.RA_DIRECTION) ? true : false;

    if (confptr->plusLS == 0 || confptr->minusLS == 0)
    {
    status.Bits.RA_PLUS_LS  = 0;
    status.Bits.RA_MINUS_LS = 0;
    }
    else
    {
    sprintf(buff, "PR I%d", confptr->plusLS);
    send_mess(card, buff, MDrivePlus_axis[signal]);
    recv_mess(card, buff, 1);
    Lswitch = atoi(buff);

    /* Set limit Lswitch error indicators. */
    if (Lswitch != 0)
    {
        status.Bits.RA_PLUS_LS = 1;
        if (plusdir == true)
        ls_active = true;
    }
    else
        status.Bits.RA_PLUS_LS = 0;

    sprintf(buff, "PR I%d", confptr->minusLS);
    send_mess(card, buff, MDrivePlus_axis[signal]);
    recv_mess(card, buff, 1);
    Lswitch = atoi(buff);

    if (Lswitch != 0)
    {
        status.Bits.RA_MINUS_LS = 1;
        if (plusdir == false)
        ls_active = true;
    }
    else
        status.Bits.RA_MINUS_LS = 0;
    }

    if (confptr->homeLS == 0)
    status.Bits.RA_HOME = 0;
    else
    {
    sprintf(buff, "PR I%d", confptr->homeLS);
    send_mess(card, buff, MDrivePlus_axis[signal]);
    recv_mess(card, buff, 1);
    Lswitch = atoi(buff);
    status.Bits.RA_HOME = (Lswitch) ? 1 : 0;
    }

    /* !!! Assume no closed-looped control!!!*/
    status.Bits.EA_POSITION = 0;

    /* encoder status */
    status.Bits.EA_SLIP       = 0;
    status.Bits.EA_SLIP_STALL = 0;
    status.Bits.EA_HOME       = 0;

    if (motor_state[card]->motor_info[signal].encoder_present == NO)
    motor_info->encoder_position = 0;
    else
    {
    send_mess(card, "PR C2", MDrivePlus_axis[signal]);
    recv_mess(card, buff, 1);
    motorData = atof(buff);
    motor_info->encoder_position = (epicsInt32) motorData;
    }

    status.Bits.RA_PROBLEM  = 0;

    /* Parse motor velocity? */
    /* NEEDS WORK */

    motor_info->velocity = 0;

    if (!status.Bits.RA_DIRECTION)
    motor_info->velocity *= -1;

    rtn_state = (!motor_info->no_motion_count || ls_active == true ||
         status.Bits.RA_DONE | status.Bits.RA_PROBLEM) ? 1 : 0;

    /* Test for post-move string. */
    if ((status.Bits.RA_DONE || ls_active == true) && nodeptr != 0 &&
    nodeptr->postmsgptr != 0)
    {
    strcpy(buff, nodeptr->postmsgptr);
    send_mess(card, buff, MDrivePlus_axis[signal]);
    nodeptr->postmsgptr = NULL;
    }

exit:
    motor_info->status.All = status.All;
    return(rtn_state);
}


/*****************************************************/
/* send a message to the MDrivePlus board                */
/* send_mess()                                       */
/*****************************************************/
static RTN_STATUS send_mess(int card, char const *com, char *name)
{
    char local_buff[MAX_MSG_SIZE];
    struct IM483controller *cntrl;
    int comsize, namesize;
    size_t nwrite;

    comsize = (com == NULL) ? 0 : strlen(com);
    namesize = (name == NULL) ? 0 : strlen(name);

    if ((comsize + namesize) > MAX_MSG_SIZE)
    {
    errlogMessage("drvMDrivePlus.c:send_mess(); message size violation.\n");
    return(ERROR);
    }
    else if (comsize == 0)  /* Normal exit on empty input message. */
    return(OK);

    if (!motor_state[card])
    {
    errlogPrintf("drvMDrivePlus.c:send_mess() - invalid card #%d\n", card);
    return(ERROR);
    }

    /* Make a local copy of the string and add the command line terminator. */
    if (namesize != 0)
    {
    strcpy(local_buff, name);    /* put in axis */
    strcat(local_buff, com);
    }
    else
    strcpy(local_buff, com);

    Debug(2, "send_mess(): message = %s\n", local_buff);

    cntrl = (struct IM483controller *) motor_state[card]->DevicePrivate;
    pasynOctetSyncIO->write(cntrl->pasynUser, local_buff, strlen(local_buff),
                COMM_TIMEOUT, &nwrite);

    return(OK);
}


/*****************************************************/
/* receive a message from the MDrivePlus board           */
/* recv_mess()                                       */
/*****************************************************/
static int recv_mess(int card, char *com, int flag)
{
    struct IM483controller *cntrl;
    const double timeout = 1.0;
    size_t nread = 0;
    asynStatus status = asynError;
    int eomReason;

    /* Check that card exists */
    if (!motor_state[card])
    return(ERROR);

    cntrl = (struct IM483controller *) motor_state[card]->DevicePrivate;

    if (flag == FLUSH)
    pasynOctetSyncIO->flush(cntrl->pasynUser);
    else
    status = pasynOctetSyncIO->read(cntrl->pasynUser, com, BUFF_SIZE,
                    timeout, &nread, &eomReason);

    if ((status != asynSuccess) || (nread <= 0))
    {
    com[0] = '\0';
    nread = 0;
    }

    Debug(2, "recv_mess(): message = \"%s\"\n", com);
    return(nread);
}


/*****************************************************/
/* Setup system configuration                        */
/* MDrivePlusSetup()                                     */
/*****************************************************/
RTN_STATUS
MDrivePlusSetup(int num_cards,  /* maximum number of chains in system.  */
        int scan_rate)  /* polling rate - 1/60 sec units.  */
{
    int itera;

    if (num_cards < 1 || num_cards > MDrivePlus_NUM_CARDS)
    MDrivePlus_num_cards = MDrivePlus_NUM_CARDS;
    else
    MDrivePlus_num_cards = num_cards;

    /* Set motor polling task rate */
    if (scan_rate >= 1 && scan_rate <= 60)
    targs.motor_scan_rate = scan_rate;
    else
    targs.motor_scan_rate = SCAN_RATE;

    /*
     * Allocate space for motor_state structures.  Note this must be done
     * before IM483Config is called, so it cannot be done in motor_init()
     * This means that we must allocate space for a card without knowing
     * if it really exists, which is not a serious problem
     */
    motor_state = (struct controller **) malloc(MDrivePlus_num_cards *
                        sizeof(struct controller *));

    for (itera = 0; itera < MDrivePlus_num_cards; itera++)
    motor_state[itera] = (struct controller *) NULL;

    return(OK);
}


/*****************************************************/
/* Configure a controller                            */
/* MDrivePlusConfig()                                    */
/*****************************************************/
RTN_STATUS
MDrivePlusConfig(int card,        /* chain being configured */
         const char *name)    /* ASYN port name */
{
    struct IM483controller *cntrl;

    if (card < 0 || card >= MDrivePlus_num_cards)
    return(ERROR);

    motor_state[card] = (struct controller *) malloc(sizeof(struct controller));
    motor_state[card]->DevicePrivate = malloc(sizeof(struct IM483controller));
    cntrl = (struct IM483controller *) motor_state[card]->DevicePrivate;

    strcpy(cntrl->asyn_port, name);
    return(OK);
}


/*****************************************************/
/* initialize all software and hardware              */
/* This is called from the initialization routine in */
/* device support.                                   */
/* motor_init()                                      */
/*****************************************************/
static int motor_init()
{
    struct controller *brdptr;
    struct IM483controller *cntrl;
    int card_index, motor_index;
    char buff[BUFF_SIZE];
    int total_axis = 0;
    int status;
    asynStatus success_rtn;
    static const char output_terminator[] = "\n";
    static const char input_terminator[]  = "\r\n";

    initialized = true;    /* Indicate that driver is initialized. */

    /* Check for setup */
    if (MDrivePlus_num_cards <= 0)
    return(ERROR);

    for (card_index = 0; card_index < MDrivePlus_num_cards; card_index++)
    {
    if (!motor_state[card_index])
        continue;

    brdptr = motor_state[card_index];
    brdptr->ident[0] = (char) NULL;    /* No controller identification message. */
    brdptr->cmnd_response = false;
    total_cards = card_index + 1;
    cntrl = (struct IM483controller *) brdptr->DevicePrivate;

    /* Initialize communications channel */
    success_rtn = pasynOctetSyncIO->connect(cntrl->asyn_port, 0,
                        &cntrl->pasynUser, NULL);

    if (success_rtn == asynSuccess)
    {
        pasynOctetSyncIO->setOutputEos(cntrl->pasynUser, output_terminator,
                       strlen(output_terminator));
        pasynOctetSyncIO->setInputEos(cntrl->pasynUser, input_terminator,
                      strlen(input_terminator));
        /* Send a message to the board, see if it exists */
        /* flush any junk at input port - should not be any data available */
        pasynOctetSyncIO->flush(cntrl->pasynUser);

        for (total_axis = 0; total_axis < MAX_AXES; total_axis++)
        {
        int retry = 0;

        /* Try 3 times to connect to controller. */
        do
        {
            send_mess(card_index, "PR VR", MDrivePlus_axis[total_axis]);
            status = recv_mess(card_index, buff, 1);
            retry++;
        } while (status == 0 && retry < 3);

        if (status <= 0)
            break;
        else if (total_axis == 0)
            strcpy(brdptr->ident, buff);
        }
        brdptr->total_axis = total_axis;
        cntrl->inconfig = (input_config *) malloc(
                             sizeof(struct IM483controller) * total_axis);
    }

    if (success_rtn == asynSuccess && total_axis > 0)
    {
        input_config *confptr = cntrl->inconfig;

        brdptr->localaddr = (char *) NULL;
        brdptr->motor_in_motion = 0;

        for (motor_index = 0; motor_index < total_axis; motor_index++)
        {
        int itera;
        struct mess_info *motor_info = &brdptr->motor_info[motor_index];

        motor_info->status.All = 0;
        motor_info->no_motion_count = 0;
        motor_info->encoder_position = 0;
        motor_info->position = 0;
        brdptr->motor_info[motor_index].motor_motion = NULL;
        /* Assume no encoder support. */
        motor_info->encoder_present = NO;

        /* Determine if encoder present based last character of "ident". */
        if (brdptr->ident[strlen(brdptr->ident) - 1] == 'E')
        {
            motor_info->pid_present = YES;
            motor_info->status.Bits.GAIN_SUPPORT = 1;
            motor_info->encoder_present = YES;
            motor_info->status.Bits.EA_PRESENT = 1;
        }

        /* Determine input configuration. */
        confptr->homeLS = confptr->minusLS = confptr->plusLS = 0;

        for (itera = 1; itera <= 4; itera++)
        {
            int type, active;

            sprintf(buff, "PR S%d", itera);
            send_mess(card_index, buff, MDrivePlus_axis[motor_index]);
            status = recv_mess(card_index, buff, 1);
            if (status == 0)
            {
            errlogPrintf("Error reading I/O configuration.\n");
            break;
            }

            status = sscanf(buff, "%d,%d", &type, &active);
            switch (type)
            {
            case 0:
            break;
            case 1: // Home switch.
            confptr->homeLS = itera;
            break;
            case 2: // Plus limit switch.
            confptr->plusLS = itera;
            break;
            case 3: // Minus limit switch.
            confptr->minusLS = itera;
            break;
            default:
            errlogPrintf("Invalid I/O type: %d.\n", type);

            }
        }
        // Test for missing configuration.
        if (confptr->minusLS == 0 || confptr->plusLS == 0)
        {
            const char p_label[6] = "Plus", m_label[6] = "Minus";
            errlogPrintf("MDrivePlus chain #%d, motor #%d %s LS not configured.\n",
                 card_index, motor_index,
                 (confptr->minusLS == 0) ? m_label : p_label);
        }

        set_status(card_index, motor_index);  /* Read status of each motor */
        }
    }
    else
        motor_state[card_index] = (struct controller *) NULL;
    }

    any_motor_in_motion = 0;

    mess_queue.head = (struct mess_node *) NULL;
    mess_queue.tail = (struct mess_node *) NULL;

    free_list.head = (struct mess_node *) NULL;
    free_list.tail = (struct mess_node *) NULL;

    epicsThreadCreate((char *) "MDrivePlus_motor", epicsThreadPriorityMedium,
              epicsThreadGetStackSize(epicsThreadStackMedium),
              (EPICSTHREADFUNC) motor_task, (void *) &targs);

    return(OK);
}

