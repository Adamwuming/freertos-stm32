#ifndef __ZEN_MOD_ILL_
#define __ZEN_MOD_ILL_
// Modbus data definition 

// From Modbus to memory
typedef struct {
	unsigned short mReg;
	unsigned short vIndex;
	unsigned short length;
	char  batch;
	char  bInput;	// Input reg or Hold reg?
	unsigned short MInterval;	// 0 for one-time
} Mod2Mem;


// From memory to JSON

// Type:
// 0x00 - 0x3F: Numeric value
//   0x00 - 0x0F: Unsigned
//   0x10 - 0x1F: Signed
// 		(value + offset) * (10 ^ scale)
// 0x00: Value =  (vIndex) 
// 0x01: Value =  (vIndex) (vIndex+1)
// 0x02: Value =  (vIndex) (vIndex+1) (vIndex+2)
//  etc.
// 0x40	ASCII String, 
//		Offset: Length in bytes, Scale: byte order
// 0x50 HexDecimal String
// 0x60 Time: SAJ yyyy mm dd hh mm ss zz
//
//	All data defined here will be sent!
// |Scale| >= 0x10    // (value + offset) * scale / 256
typedef struct {
	const char 		*token;
	unsigned short 	vIndex;
	unsigned short	type;
	short 			offset;
	short 			scale;
	unsigned short	dCon;
	//unsigned short  invalid;
} Mem2Lit;

typedef struct {
	unsigned short value;
	unsigned short dCon;
} V2Con;

typedef struct {
	unsigned short vIndex;
	unsigned short vMask;
	int 		   len;
	const V2Con    *vcArr;
} TypeCon;

typedef int (*modErrHandler)(unsigned short *mData, int vIndex, int len, char *Illu);

typedef struct {
	int				memSize;	// In words!
	
	int				nInfoBatch;
	const Mod2Mem   *pInfoM2M;
	int				nInfoOffset;
	int				nInfoDisplay;
	const Mem2Lit   *pInfoM2L;
	
	int				nDataBatch;
	const Mod2Mem	*pDataM2M;
	int				nDataOffset;
	int				nDataDisplay;
	const Mem2Lit	*pDataM2L;
	
	const TypeCon	*pTC;
	
	int				errIndex;
	int				errLen;
	modErrHandler	fErr;
} ModDevice;

typedef struct {
	int start;
	int end;
	const ModDevice *dev;
} ModSlave;

typedef struct {
	int	modBaud;
	int modParity;
	int sTypeCount;
	const ModSlave *sl;
} ModConfig;

extern char gErrTmp[128];
#endif
