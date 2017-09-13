enum {
	VCTWANPORT0,
	VCTWANPORT1,
	VCTLANPORT0,
	VCTLANPORT1,
	VCTLANPORT2,
	VCTLANPORT3,
	VCTLANPORT4,
};
int VCTSetPortSpeed(char *ifname, char *portspeed);
int VCTGetPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode);
int VCTGetPortConnectState(char *ifname, int portNum, char *connect_state);
