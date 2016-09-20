#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "utask.h"

#define MAX_TASK_NUM 10  

#define ARG_NUM     8
#define CMD_LEN     20
#define CMD_BUF_LEN 128
   
typedef struct {  
     char const *cmd_name;                  
     int32_t max_args;
     void (*handle)(int argc, void * cmd_arg);
     const char  *help; 
} CmdListStruct;

void PrintHello(int argc, void * cmd_arg);
void HandleArg(int argc, void * cmd_arg);
void HandleHelp(int argc, void * cmd_arg);
void HandleReset(int argc, void * cmd_arg);
void GetTaskState(int argc, void * cmd_arg);

void cTurnOn(int argc, void * cmd_arg);
void cTurnOff(int argc, void * cmd_arg);
void cMReset(int argc, void * cmd_arg);
void cLevel(int argc, void * cmd_arg);
void cShowModem(int argc, void * cmd_arg);
void cSendAT(int argc, void * cmd_arg);
void cFCmd(int argc, void * cmd_arg);


const CmdListStruct cmd_list[] = {  
	//{"SendAT",  1,  cSendAT,      "SendAT ATcmd   -Send AT to modem"},
  {"?",       0,  HandleHelp,   "?              -Print help"},                   
	//{"arg",     8, 	HandleArg,    "arg a1 a2...   -Test, print input paras"},
  {"hello",   0,  PrintHello,   "hello          -Print HelloWorld!"},  
  //{"reset",   0,  HandleReset,  "reset          -Reset controller"},  
  {"task",    0,  GetTaskState, "task           -Get task status"}, 
//===============
	//{"ton",		1,	cTurnOn,	  "ton            -Turn on module"},
	//{"toff",	1,	cTurnOff,	  "toff           -Turn off module"},
	//{"mreset",	1,	cMReset,	  "mreset         -Restart module"},
	//{"level",	0,	cLevel,	      "level          -Show module levels"},	
	//{"showm",   0,  cShowModem,   "showm          -Show modem status"},
	//{"cmdmode",	0,	cFCmd,        "cmdmode        -Force to command mode"},
};  


typedef struct {  
    char rec_buf[CMD_BUF_LEN]; 
    char processed_buf[CMD_BUF_LEN]; 
    int32_t cmd_arg[ARG_NUM];  
} cmd_analyze_struct;  

cmd_analyze_struct cmd_analyze;

void FillConsoleBuf(char data)  
{  
  static uint32_t rec_count = 0;  
     
  cmd_analyze.rec_buf[rec_count] = data;  
	
  if (0x0a==cmd_analyze.rec_buf[rec_count] && 0x0d == cmd_analyze.rec_buf[rec_count-1])
  {  
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  
    rec_count=0;  
         
    vTaskNotifyGiveFromISR(xCmdAnalyzeHandle, &xHigherPriorityTaskWoken);  
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);  
  }  
  else  
  {  
    rec_count++;  
    if(rec_count >= CMD_BUF_LEN)  
    {  
      rec_count=0;  
    }  
  }      
}  

static uint32_t get_true_char_stream(char *dest, const char *src)
{  
	uint32_t dest_count=0;  
	uint32_t src_count=0;  
     
    while(src[src_count]!=0x0d && src[src_count+1]!=0x0a)  
    {  
		if (isprint(src[src_count]))  dest[dest_count++] = src[src_count++];  
		else  
			switch(src[src_count])  
			{  
			case	0x08: 
				if (dest_count>0)	dest_count--;  
				src_count ++;  
				break;  
			case	0x1B:  
				if (src[src_count+1] == 0x5B)  
				{  
					if (src[src_count+2]==0x41 || src[src_count+2]==0x42)  
						src_count +=3;  // Up arrow or down arrow	 
					else if (src[src_count+2] == 0x43)
					{
						dest_count++;	// Right arrow
						src_count += 3;
					}
					else if(src[src_count+2] == 0x44)
					{
						if (dest_count >0) dest_count --;  // Left arrow
                        src_count +=3;  
					}
					else src_count +=3;  
				}  
				else src_count ++;  
                break;  
			default:  
				src_count++;  
				break;  
           }	// switch  
	   } // while
	dest[dest_count++] = src[src_count++];  
	dest[dest_count++] = src[src_count++];  
	dest[dest_count] = 0;  
  return dest_count;  
}  

static int32_t string_to_dec(uint8_t *buf, uint32_t len)  
{  
	uint32_t i=0;  
	uint32_t base=10;
	int32_t  neg=1;
	int32_t  result=0;  
     
    if ((buf[0]=='0') && (buf[1]=='x')) { base=16; neg=1; i=2; }
    else if(buf[0]=='-') { base=10; neg=-1; i=1; }
    for(; i<len; i++)  
    {  
       if (buf[i]==0x20 || buf[i]==0x0D) break;  
       result *= base;  
       if (isdigit(buf[i]))  result += buf[i]-'0';  
       else if(isxdigit(buf[i])) result += tolower(buf[i])-87;  
       else result += buf[i]-'0';  
    }  
	result *= neg;  
	return result;  
} 

static int32_t cmd_arg_analyze(char *rec_buf, unsigned int len)  
{  
	uint32_t i;  
	uint32_t blank_space_flag=0;   
	uint32_t arg_num=0;  
	uint32_t index[ARG_NUM];  
     
	for (i=0; i<len; i++)  
    {  
		if (rec_buf[i] == 0x20) { blank_space_flag=1; continue; }
		else if(rec_buf[i] == 0x0d) break; // CR
		else  
		{  
			if (blank_space_flag == 1)  
			{  
				blank_space_flag = 0;
				if (arg_num < ARG_NUM) { index[arg_num]=i; arg_num++; }
				else return -1;
			}
		} // else
	}
     
    for (i=0; i<arg_num; i++)  
		cmd_analyze.cmd_arg[i] = string_to_dec((unsigned char *)(rec_buf+index[i]), len-index[i]);  
	return arg_num;  
}  

/////////////////////
// Command handlers
void PrintHello(int argc, void * cmd_arg)  
{  
	Print("Hello world!\n");  
}  

//void HandleArg(int argc, void * cmd_arg)  
//{  
//	uint32_t i;  
//	int32_t  *arg = (int32_t *)cmd_arg;  
//    if (argc == 0)
//		MY_DEBUGF(CMD_LINE_DEBUG, ("No paras\n"));  
//	else  
//		for (i=0;i <argc; i++) 
//		{
//			MY_DEBUGF(CMD_LINE_DEBUG, ("Para %d: %d\n", i+1, arg[i]));
//			osDelay(5);
//		}
//}  

void HandleHelp(int argc, void * cmd_arg)  
{
	int i;
	for (i=0; i<sizeof(cmd_list)/sizeof(CmdListStruct); i++)
	{
		Print(cmd_list[i].help);
		Print("\n");
		osDelay(8);
	}
}

//void cSendAT(int argc, void * cmd_arg)  
//{
//	MdmSendwCR((const char *)cmd_arg);
//}

void HandleReset(int argc, void * cmd_arg)
{
}

TaskStatus_t pxTaskStatusArray[MAX_TASK_NUM];  

void GetTaskState(int argc, void *cmd_arg)  
{  
	const char task_state[]={'r','R','B','S','D'};
	volatile UBaseType_t uxArraySize, x;  
	uint32_t ulTotalRunTime, ulStatsAsPercentage;  
   
	uxArraySize = uxTaskGetNumberOfTasks();  
	if (uxArraySize > MAX_TASK_NUM) 
		Print("Too many active tasks!\n");  

	uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime );  
  
	/* For percentage calculations. */
	ulTotalRunTime /= 100UL;

	#if (configGENERATE_RUN_TIME_STATS==1)  
  
	Print("Task name  Status ID Prior Stack CPU usage\n");  
	osDelay(10);
	if ( ulTotalRunTime > 0 )  
	{  
		for( x = 0; x < uxArraySize; x++ )  
		{  
			/* What percentage of the total run time has the task used?
			This will always be rounded down to the nearest integer.
			ulTotalRunTimeDiv100 has already been divided by 100. */
			
			ulStatsAsPercentage = (uint64_t)(pxTaskStatusArray[x].ulRunTimeCounter) / ulTotalRunTime;  
			if ( ulStatsAsPercentage > 0UL )  
				sprintf(gTmp, "%-13s%-6c%-4ld%-5ld%-6d%d%%", 
							 pxTaskStatusArray[x].pcTaskName, task_state[pxTaskStatusArray[x].eCurrentState], 
							 pxTaskStatusArray[x].xTaskNumber, pxTaskStatusArray[x].uxCurrentPriority,
							 pxTaskStatusArray[x].usStackHighWaterMark, ulStatsAsPercentage);  
			else
				sprintf(gTmp, "%-13s%-6c%-4ld%-5ld%-6dt<1%%",
			        pxTaskStatusArray[x].pcTaskName, task_state[pxTaskStatusArray[x].eCurrentState],
			        pxTaskStatusArray[x].xTaskNumber, pxTaskStatusArray[x].uxCurrentPriority,
			        pxTaskStatusArray[x].usStackHighWaterMark); 
			Print(gTmp);
			Print("\n");
			osDelay(10);
		}  
	}  
	//Print("    run Ready Blocked Suspended Deleted\n"); 
	//osDelay(10);
	sprintf(gTmp, "Heap: %d/%d\n",  xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
	Print(gTmp);
#endif //#if (configGENERATE_RUN_TIME_STATS==1)  
} 

void ProcessGPS(cmd_analyze_struct *cmd)
{
	Print("GPS commands\n");
	Print(PROMPT);
}
  
void vTaskCmdAnalyze(void *pvParameters)
{
	uint32_t i;
	int32_t rec_arg_num;
	char cmd_buf[CMD_LEN];     
	
	while (1)  
	{
		uint32_t rec_num;
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		rec_num = get_true_char_stream(cmd_analyze.processed_buf, cmd_analyze.rec_buf);  
		
		if (*cmd_analyze.rec_buf == 0x0d) goto NextCmd;
		
		if (*cmd_analyze.rec_buf == '$')
		{
			ProcessGPS(&cmd_analyze);
			goto NextCmd;
		}
         
		for (i=0; i<CMD_LEN; i++)
		{
			if ((i>0) && ((cmd_analyze.processed_buf[i]==' ') || (cmd_analyze.processed_buf[i]==0x0d)))
			{	cmd_buf[i]='\0'; break; }
			else cmd_buf[i] = cmd_analyze.processed_buf[i];  
		}
		rec_arg_num = cmd_arg_analyze(&cmd_analyze.processed_buf[i], rec_num);
		for (i=0; i<sizeof(cmd_list)/sizeof(cmd_list[0]); i++)  
		{
			if(!strcmp(cmd_buf, cmd_list[i].cmd_name))
			{  
				if (i == 0) 
					cmd_list[0].handle(1, cmd_analyze.processed_buf+7); // Special for SendAT
                else if (rec_arg_num<0 || rec_arg_num>cmd_list[i].max_args)  
					Print("Too many paras!\n");
                else cmd_list[i].handle(rec_arg_num, (void *)cmd_analyze.cmd_arg);
				break;
			}
		}
		if (i >= sizeof(cmd_list)/sizeof(cmd_list[0]))
			Print("Unknown command!\n");  
	NextCmd:
		Print(PROMPT);
	}
}  

//
/*
void vDaemonConsole(void *pvPara)
{
	char *s;
	int len;
	portBASE_TYPE ret;
	
	while(1)
	{
		ret = xSemaphoreTake(xConSema, 5000);
		if (ret == pdPASS) {
			ret = GetConsoleInput(&s, &len);
			if (ret > 0) {
				// TODO: More work in future
				s[len-1] = 0;
				printf("%s", s);
			}
			else
				printf("Error in input: %ld\n", ret);
			EnableConsole();
		}
	}
}
*/


//void cTurnOn(int argc, void * cmd_arg)
//{
//	int ret = MTURNON;
//	if (argc != 0) ret = *((int32_t *)cmd_arg);
//	ret = TurnOnModule(ret);
//	sprintf(gTmp, "TurnOn: %d\n", ret);
//	Print(gTmp);
//}

//void cTurnOff(int argc, void * cmd_arg)
//{
//	int ret = MTURNOFF;
//	if (argc != 0) ret = *((int32_t *)cmd_arg);
//	ret = TurnOffModule(ret);
//	sprintf(gTmp, "TurnOff: %d\n", ret);
//	Print(gTmp);
//}

//void cMReset(int argc, void * cmd_arg)
//{
//	int ret = MRESTART;
//	if (argc != 0) ret = *((int32_t *)cmd_arg);
//	ret = RestartModule(ret);
//	sprintf(gTmp, "RestartM: %d\n", ret);
//	Print(gTmp);
//}

extern int GetMStatus(void);
extern int GetVdd(void);
extern int GetDCD(void);
//void cLevel(int argc, void * cmd_arg)
//{
//	sprintf(gTmp, "STA: %d   Vd28: %d   DCD: %c\n", GetMStatus(), GetVdd(), GetDCD()?'A':'-');
//	Print(gTmp);
//}

//void cShowModem(int argc, void * cmd_arg)
//{
//	sprintf(gTmp, "Modem status: %d;%d\n", gModem, gModemS);
//	Print(gTmp);
//}

//void cFCmd(int argc, void * cmd_arg)
//{
//	int ret = MdmCmdMode();
//	sprintf(gTmp, "Disconnect: %d\n", ret);
//	Print(gTmp);
//}
