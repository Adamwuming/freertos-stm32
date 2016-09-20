#include "InvDef.h"
#include <string.h>
#include <stdio.h>
#include "utask.h"

char gErrTmp[128];

//=========================== Growatt ========================
int fnGrowErrCB(unsigned short *mData, int vIndex, int len, char *out);

const Mod2Mem glGrowM2M[] = {
	{ 9,  0, 21, 0, 0, 0},
	{43, 22,  2, 1, 0, 0},	// Device Type, MPPT & phrase
};

const Mem2Lit glGrowM2L[] = {
	{ "FWV",   0, 0x40,  6,  0, 0x01},	
	{ "CFWV",  3, 0x40,  6,  0, 0},	
	{ "sVPVS", 8,    0,  0, -1, 0},
	{ "sTmS",  9,    0,  0,  0, 0},
	{ "sVacL",10,    0,  0, -1, 0},
	{ "sVacH",11,    0,  0, -1, 0},
	{ "sVacL",12,    0,  0, -2, 0},
	{ "sVacH",13,    0,  0, -2, 0},
	{ "SN",   14, 0x40, 10,  0, 0x01},	
	{ "InvMd",19, 0x50,  4,  0, 0x01},
	{ "DTC",  22,    0,  0,  0, 0x01},
};

const Mod2Mem grGrowM2M[] = {
	{  0, 30, 33, 0, 1, 150},	// Information
	{ 33, 70,  8, 1, 1,  60},	// Error value
};

// Stored in addr 30, start from addr 31
const Mem2Lit grGrowM2L[] = {
	{ "PVP",   0, 1, 0, -1, 0x01},	// Total input power
	{ "PV1V",  2, 0, 0, -1, 0x01},
	{ "PV1I",  3, 0, 0, -1, 0x01},
	{ "PV1P",  4, 1, 0, -1, 0x01},
	{ "PV2V",  6, 0, 0, -1, 0x02},
	{ "PV2I",  7, 0, 0, -1, 0x02},
	{ "PV2P",  8, 1, 0, -1, 0x02},
	{ "PAC",  10, 1, 0, -1, 0x01},	
	{ "FAC",  12, 0, 0, -2, 0x01},
	{ "L1V",  13, 0, 0, -1, 0x01},
	{ "L1I",  14, 0, 0, -1, 0x01},
	{ "L1P",  15, 1, 0, -1, 0x01},
	{ "L2V",  17, 0, 0, -1, 0x08},
	{ "L2I",  18, 0, 0, -1, 0x08},
	{ "L2P",  19, 1, 0, -1, 0x08},
	{ "L3V",  21, 0, 0, -1, 0x08},
	{ "L3I",  22, 0, 0, -1, 0x08},
	{ "L3P",  23, 1, 0, -1, 0x08},
	{ "EToday",25,1, 0, -1, 0x01},
	{ "ETotal",27,1, 0, -1, 0x01},
	{ "STotal",29,1, 0, 128, 0x01},
	{ "TInv", 31, 0, 0, -1, 0x01},	
	//{ "PF",   32, 0, -10000, -4},
};

const V2Con	gVCGrow[] = {
	{ 0x101, 0x01},
	{ 0x201, 0x03},
	{ 0x103, 0x09},
	{ 0x203, 0x0b},
};
const TypeCon gTCGrow = { 23, 0xffff, sizeof(gVCGrow)/sizeof(V2Con), gVCGrow };

const ModDevice gDeviceGrow = { 
	90, 
	sizeof(glGrowM2M)/sizeof(Mod2Mem), glGrowM2M, 0, sizeof(glGrowM2L)/sizeof(Mem2Lit), glGrowM2L,
	sizeof(grGrowM2M)/sizeof(Mod2Mem), grGrowM2M, 37,sizeof(grGrowM2L)/sizeof(Mem2Lit), grGrowM2L,	
	&gTCGrow,
	70, 8,
	fnGrowErrCB,
};

static const char *kGrowErr[] = {
	"Autotest failed",
	"No AC connection",
	"PV isolation low",
	"Residual current high",
	"Output DCI high",
	"PV voltage high",
	"AC voltage out of range",
	"AC frequency out of range",
	"Module too hot",
};

// Return Json string
int fnGrowErrCB(unsigned short *mData, int vIndex, int aLen, char *out)
{
	static short lastError = 0;
	int len;
	
	mData += vIndex;
	if (mData[7] == lastError) return 0;
	lastError = mData[7];
	
	if (lastError == 0)	// Recovered
	{
		strcpy(out+len, "\"Fault\":0");
		return strlen(out);
	}
	
	if (lastError >= 0 && lastError <= 23) 
		sprintf(out, "\"Fault:\":%d, \"FaultM\":\"%d\"", lastError, 99+lastError);
	else if (lastError <= 32) 
		sprintf(out, "\"Fault:\":%d, \"FaultM\":\"%s\"", lastError, kGrowErr[lastError-24]);
	else 
		sprintf(out, "\"Fault:\":%d, \"FaultM\":\"Unk%d\"", lastError, lastError);
	
	len = strlen(out);
	sprintf(out+len, ",\"FaultV\":\"%04X%04X%04X%04X%04X%04X%04X\"", 
			mData[0], mData[1], mData[2], mData[3], mData[4], mData[5], mData[6]);
	return strlen(out);
}
	
	
//============================ SAJ ===============================
int fnSAJErrCB(unsigned short *mData, int vIndex, int len, char *out);

const Mod2Mem glSAJM2M[] = {
	{ 0x8f00, 0, 29, 0, 0, 0},
};

const Mod2Mem grSAJM2M[] = {
	{ 0x100, 30, 59, 0, 0, 150},
};


const Mem2Lit glSAJM2L[] = {
	// 0: Type
	{ "SN",    3, 0x40, 20,  0, 0x01},
	{ "PC",   13, 0x40, 20,  0, 0x01},
    { "DSV",  23,    0,  0, -3, 0x01},
    { "MCSV", 24,    0,  0, -3, 0x01},
	{ "SCSV", 25,    0,  0, -3, 0  },
	{ "DHV",  26,    0,  0, -3, 0  },
	{ "CHV",  27,    0,  0, -3, 0  },
	{ "PHV",  28,    0,  0, -3, 0  },
};


// Stored in addr 30, start from addr 37
// From SAJ Modbus protocol Realtime
//	 dCond	MASK
//	B7
//	B6
//	B5
//	B4
//	B3  --  3-phrase
//	B2  --  Has 3rd PV
//	B1  --  Has 2nd PV
//	B0	--	Inverter
const Mem2Lit grSAJM2L[] = {
	// Addr30:    Mode
	//     31-36: Error info
	{ "PV1V",  0,    0, 0, -1, 0x01},
	{ "PV1I",  1,    0, 0, -2, 0},
	{ "PV1P",  2,    0, 0,  0, 0x01},
	{ "PV2V",  3,    0, 0, -1, 0x02},
	{ "PV2I",  4,    0, 0, -2, 0},
	{ "PV2P",  5,    0, 0,  0, 0x02},
	{ "PV3V",  6,    0, 0, -1, 0x04},
	{ "PV3I",  7,    0, 0, -2, 0},
	{ "PV3P",  8,    0, 0,  0, 0x04},
	{ "VBus",  9,    0, 0, -1, 0x01},
	{ "TInv", 10, 0x10, 0, -1, 0x01},
	{ "GFCI", 11, 0x10, 0,  0, 0x01},	// Leak current to ground
	{ "PAC",  12,    0, 0,  0, 0x01},
	{ "QPow", 13,    0, 0,  0, 0x01},
	{ "PF",   14, 0x10, 0, -3, 0},   // All PF illustration may go WRONG!
	{ "L1V",  15,    0, 0, -1, 0x01},
	{ "L1I",  16,    0, 0, -2, 0x01},
	{ "L1F",  17,    0, 0, -2, 0x01},
	{ "L1DCI",18, 0x10, 0,  0, 0x01},
	{ "L1P",  19,    0, 0,  0, 0x01},
	{ "L1PF", 20, 0x10, 0, -3, 0x01},

	{ "L2V",  21,    0, 0, -1, 0x08},
	{ "L2I",  22,    0, 0, -2, 0x08},
	{ "L2F",  23,    0, 0, -2, 0x08},
	{ "L2DCI",24, 0x10, 0,  0, 0x08},
	{ "L2P",  25,    0, 0,  0, 0x08},
	{ "L2PF", 26, 0x10, 0, -3, 0x08},

	{ "L3V",  27,    0, 0, -1, 0x08},
	{ "L3I",  28,    0, 0, -2, 0x08},
	{ "L3F",  29,    0, 0, -2, 0x08},
	{ "L3DCI",30, 0x10, 0,  0, 0x08},
	{ "L3P",  31,    0, 0,  0, 0x08},
	{ "L3PF", 32, 0x10, 0, -3, 0x08},
	
	{ "ISO1", 33,    0, 0,  0, 0},
	{ "ISO2", 34,    0, 0,  0, 0},
	{ "ISO3", 35,    0, 0,  0, 0},
	{ "ISO4", 36,    0, 0,  0, 0},	
	
	{ "EToday",37,   0, 0, -2, 0x01},
	{ "EMonth",38,   1, 0, -2, 0x01},
	{ "EYear", 40,   1, 0, -2, 0x01},
	{ "ETotal",42,   1, 0, -2, 0x01},
	
	{ "HToday",44,   0, 0, -1, 0x01},
	{ "HTotal",45,   1, 0, -1, 0x01},
	{ "ErrCnt",47,   0, 0,  0, 0},
	{ "ITime", 48,0x60, 0,  0, 0},
};

const V2Con   gVCSAJ[] = {
	{ 0x10, 0x01},
	{ 0x20, 0x0b},
	{ 0x11, 0x01},
	{ 0x12, 0x03},
	{ 0x21, 0x0b},
};
const TypeCon gTCSAJ = { 30, 0xffff, sizeof(gVCSAJ)/sizeof(V2Con), gVCSAJ };

const ModDevice gDeviceSAJ2 = { 
	90, 
	sizeof(glSAJM2M)/sizeof(Mod2Mem), glSAJM2M, 0, sizeof(glSAJM2L)/sizeof(Mem2Lit), glSAJM2L,
	sizeof(grSAJM2M)/sizeof(Mod2Mem), grSAJM2M, 37,sizeof(grSAJM2L)/sizeof(Mem2Lit), grSAJM2L,	
	&gTCSAJ,
	0, 0,
	fnSAJErrCB,
};

int fnSAJErrCB(unsigned short *mData, int vIndex, int len, char *out)
{
	UNUSED(mData);
	UNUSED(vIndex);
	UNUSED(len);
	UNUSED(out);
	return 0;
}

const ModSlave gModSlave[] = {
	{ 1, 1, &gDeviceSAJ2 },
	
};

const ModConfig gMConf = { BAUD115200, 0, sizeof(gModSlave)/sizeof(ModSlave), gModSlave };

