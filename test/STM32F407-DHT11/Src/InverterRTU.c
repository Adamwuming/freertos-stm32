#include "utask.h"
#include "user_mb_app.h"



void vMBPTask(void *argu)
{
	eMBMasterInit(MB_RTU, 19200, MB_PAR_EVEN);
	eMBMasterEnable();
	for(;;)
	{
		eMBMasterPoll();
		osDelay(0);
	}
}

void ReadInvSN(void)
{
  switch (eMBMasterReqReadHoldingRegister(1, 500, 79, -1))
  {
    case MB_MRE_NO_ERR:
      sprintf(gTmp, "MBReq: 5->EV_MASTER_PROCESS_SUCESS -> MB_MRE_NO_ERR\n"); 
      Print(gTmp);
      break;
			
    case MB_MRE_TIMEDOUT:
      sprintf(gTmp, "MBReq: 6->EV_MASTER_ERROR_RESPOND_TIMEOUT -> MB_MRE_TIMEDOUT\n"); 
      Print(gTmp);
      break;

    case MB_MRE_REV_DATA:
      sprintf(gTmp, "MBReq: 7->EV_MASTER_ERROR_RECEIVE_DATA -> MB_MRE_REV_DATA\n"); 
      Print(gTmp);
      break;
			
    case MB_MRE_EXE_FUN:
      sprintf(gTmp, "MBReq: 8->EV_MASTER_ERROR_EXECUTE_FUNCTION -> MB_MRE_EXE_FUN\n"); 
      Print(gTmp);
      break;
			
    default:
      break;
  }
}

