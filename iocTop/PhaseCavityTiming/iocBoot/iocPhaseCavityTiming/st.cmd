## RTEMS startup script
## Install this st.cmd in the top of the project directory

ld( "bin/RTEMS-beatnik/PhaseCavityTiming.obj" )

#< envPaths

# Startup for Bill Ross' 120MSPS VME Digitizer Test Driver
# (simple EPICS waveform)
# Make sure we can transfer arrays > 16k.
# Must be > max # elements * 8 since data
# may be converted to doubles on IOC if
# the client requests it.
setenv("EPICS_CA_MAX_ARRAY_BYTES","2000000",1)

# Silence BSP warnings
bspExtVerbosity = 0

# Configure VME digitizers
#
#   devVmeDigiConfig(card_no, slot_no, vme_a32_addr, int_vec, int_lvl)
#
# card_no:
#     'unit number' associated with a particular card (1..8).
#
# slot_no:
#     VME slot the card is sitting in (1-based, i.e., a number from 1..21).
#
# vme_a32_addr:
#     Address in VME A32 space where card's sample memory is to be mapped.
#
# int_vec:
#     Unique VME interupt vector.
#
# int_lvl:
#     VME interrupt level (may be shared among cards).
#
# NOTE: 'slot_no' may be zero. In this case the VME bus is scanned
#       for instance 'card_no'. E.g., if digitizers are in slot 5 + 7
#       and 'card_no==2', 'slot_no==0' then the card in slot 7 is
#       be configured and associated with card/unit number '2'.
#
#       If both, 'card_no' and 'slot_no' are zero then the VME bus
#       is scanned and all cards that are found are configured and
#       assigned unique card numbers (from 1..N).
#       The sample memories of the cards are mapped at consecutive
#       blocks starting at vme_a32_addr. Interrupt vectors are assigned
#       starting with 'int_vec'. E.g., if 'int_vec==0x30' then cards
#       1..3 will use vectors 0x30, 0x31 and 0x32. The same 'int_lvl'
#       is shared by all cards.
#
devVmeDigiConfig(0,0,0x20000000,0x40,3)

# Initialize Acromag IPAC carrier
# Slot 0:	IP440AE   Digital Input
# Slot 1:	IP445E    Digital Output
# Slot 2:	IP330AE   Analog  Input
# Slot 3:	IP231-16E Analog  Output
ipacAddCarrier(&xy9660,"0x0000")

# Initialize IP440 digital input module
# xy2440Create( <name>, <card>, <slot>, <modeName>, <intHndlr>, <usrFunc>, <vector>, <event>, <debounce> )
xy2440Create( "di0", 0, 0, "STANDARD", "LEVEL", 0x0, 0x80, 0x0, 0x00 )

# Initialize IP445 digital output module
# xy2445Create( <name>, <card>, <slot> )
#xy2445Create( "do0", 0, 1 )

# Initialize IP330 analog input module
#ip330Create( "ai0", 0, 2, "0to5D", "ch12-ch15", 0, 0, "burstCont-Output-Avg1", "80*3@8MHz", 0x66 )
#ip330StartConvertByName( "ai0" )

# Initialize IP231 analog output module
# ip231Create( <name>, <card>, <slot>, <dacmode> )
# <dacmode> must be "transparent" or "simultaneous"
# In transparent mode, outputs are updated when written
# In simultaneous mode, multiple output values can be setup,
# and all outputs update when ip231SimulTrigger(<cardNo>) is called
#ip231Create( "ao0", 0, 3, "transparent" )

# Load EPICS database definition
dbLoadDatabase("dbd/PhaseCavityTiming.dbd",0,0)

## Register all support components
PhaseCavityTiming_registerRecordDeviceDriver(pdbbase) 

## Load EPICS records
#dbLoadRecords("db/IP231.db","CARD=ao0")
#dbLoadRecords("db/IP330.db","CARD=ai0")
dbLoadRecords("db/ip440.db","CARD=di0")
#dbLoadRecords("db/ip445.db","CARD=do0")
dbLoadRecords("db/vmeDigiApp.db","digi=dig1,card=1,nelm=4096")
# An EDM panel for the digitizer can be
# started with the command:
# edm -x -m "digi=<digi>" display/phaseCavity.edl

# Print list of loaded binaries (helpful for debugging)
lsmod()

# Convenience aliases
reboot=rtemsReboot
mon=rtemsMonitor

# 
iocInit()

