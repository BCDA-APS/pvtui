# ../../bin/${EPICS_HOST_ARCH}/testIOC st.cmd
< envPaths

dbLoadDatabase("../../dbd/ioctestIOCLinux.dbd")
ioctestIOCLinux_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("PREFIX", "$(USER):")

# Some test records. This database only includes record
# types included in EPICS base
dbLoadRecords("$(TOP)/db/example.db", "P=$(PREFIX)")

# Load simulated motor (Requires motor module)
epicsEnvSet("INSTANCE", "motorSim")
epicsEnvSet("NUM_AXES", "16")
epicsEnvSet("DHLM", "100000000")
epicsEnvSet("DLLM", "-100000000")
epicsEnvSet("HIGH_LIM", "50000")
epicsEnvSet("LOW_LIM", "-50000")
motorSimCreate($(CONTROLLER=0), 0, $(LOW_LIM=-32000), $(HIGH_LIM=32000), $(HOME_POS=0), 1, $(NUM_AXES=1))
drvAsynMotorConfigure("$(INSTANCE)$(CONTROLLER=0)", "$(INSTANCE)", $(CONTROLLER=0), $(NUM_AXES=1))
dbLoadTemplate("motorSim.substitutions", "P=$(PREFIX), DTYP='asynMotor', PORT=$(INSTANCE)$(CONTROLLER=0),DHLM=$(DHLM=320.0),DLLM=$(DLLM=-320.0)")

# An asyn record which connects to test_server.py (requires asyn module)
drvAsynIPPortConfigure("TEST_PORT", "localhost:5000", 0, 0, 0)
asynOctetSetOutputEos("TEST_PORT", 0, "\r\n")
asynOctetSetInputEos("TEST_PORT", 0, "\r\n")
dbLoadRecords("$(ASYN)/db/asynRecord.db", "P=$(PREFIX),R=asyn1,PORT=TEST_PORT,ADDR=0,OMAX=256,IMAX=256")

# transform record (requires calc module)
dbLoadRecords("$(TOP)/db/transformRecord.db", "P=$(PREFIX),R=tran1")

###############################################################################
iocInit
###############################################################################

# print the time our boot was finished
date
