# Makefile
# Sample for RH_NRF24 on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC            = g++
CFLAGS        = -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY
LIBS          = -lbcm2835
RADIOHEADBASE = ../..
INCLUDE       = -I$(RADIOHEADBASE)

all: RasPiRH

RasPi.o: $(RADIOHEADBASE)/RHutil/RasPi.cpp
	$(CC) $(CFLAGS) -c $(RADIOHEADBASE)/RHutil/RasPi.cpp $(INCLUDE)

RasPiRH.o: RasPiRH.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RH_NRF24.o: $(RADIOHEADBASE)/RH_NRF24.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHMesh.o: $(RADIOHEADBASE)/RHMesh.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHRouter.o: $(RADIOHEADBASE)/RHRouter.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHReliableDatagram.o: $(RADIOHEADBASE)/RHReliableDatagram.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHDatagram.o: $(RADIOHEADBASE)/RHDatagram.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHHardwareSPI.o: $(RADIOHEADBASE)/RHHardwareSPI.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHNRFSPIDriver.o: $(RADIOHEADBASE)/RHNRFSPIDriver.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericDriver.o: $(RADIOHEADBASE)/RHGenericDriver.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericSPI.o: $(RADIOHEADBASE)/RHGenericSPI.cpp
	$(CC) $(CFLAGS) -c $(INCLUDE) $<

RasPiRH: RasPiRH.o RH_NRF24.o RHMesh.o RHRouter.o RHReliableDatagram.o RHDatagram.o RasPi.o RHHardwareSPI.o RHNRFSPIDriver.o RHGenericDriver.o RHGenericSPI.o
	$(CC) $^ $(LIBS) -o RasPiRH


clean:
	rm -rf *.o RasPiRH

