#include "..\..\Public\CH554.H"
#include "..\..\Public\Debug.H"
#include "..\USB_LIB\USBHOST.H"
#include "..\..\adb_data.h"
#include ".\GPIO.c"
#include <stdio.h>
#include <string.h>
#pragma  NOAREGS
#define NOT_CONNECT 0
#define AUTH 1
#define KEY_AUTH 2
#define DIFF 3
#define ACTIVITY 4
#define CAT 5
#define CHMOD 6
#define START_DRIVE 7
#define INSTALL_APP 8
#define END 100
/*»ñÈ¡Éè±¸ÃèÊö·ûº*/
UINT8C  SetupGetDevDescr[] = { USB_REQ_TYP_IN, USB_GET_DESCRIPTOR, 0x00, USB_DESCR_TYP_DEVICE, 0x00, 0x00, sizeof( USB_DEV_DESCR ), 0x00 };
/*»ñÈ¡ÅäÖÃÃèÊö·û*/
UINT8C  SetupGetCfgDescr[] = { USB_REQ_TYP_IN, USB_GET_DESCRIPTOR, 0x00, USB_DESCR_TYP_CONFIG, 0x00, 0x00, 0x04, 0x00 };
/*ÉèÖÃUSBµØÖ·¡¤*/
UINT8C  SetupSetUsbAddr[] = { USB_REQ_TYP_OUT, USB_SET_ADDRESS, USB_DEVICE_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00 };
/*ÉèÖÃUSBÅäÖÃ*/
UINT8C  SetupSetUsbConfig[] = { USB_REQ_TYP_OUT, USB_SET_CONFIGURATION, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
/*Çå³ý¶Ïµã*/
UINT8C  SetupClrEndpStall[] = { USB_REQ_TYP_OUT | USB_REQ_RECIP_ENDP, USB_CLEAR_FEATURE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
/*»ñÈ¡HUBÃèÊö·û*/
UINT8X  Usb_Adb_status;
UINT8X  UsbDevEndp0Size;                                                       
UINT8X  RxBuffer[ MAX_PACKET_SIZE ] _at_ 0x0000 ;                              // IN, must even address
UINT8X  TxBuffer[ 250 ] _at_ 0x0040 ;                                          // OUT, must even address
UINT8 Set_Port = 0;
UINT8C cmd = 0x0A;
UINT8 SHOULD_INSTALL = 0;
_RootHubDev xdata ThisUsbDev;                                                   //ROOT
_DevOnHubPort xdata DevOnHubPort[HUB_MAX_PORTS];                                // 
bit     RootHubId;                                                              // 
bit     FoundNewDev;
int connestion_state=1;
int preLen=0;
int strart_drive_times = 0;
int diff_times = 0;
/*******************************************************************************
* Function Name  : detectConnection()
* Description    : ¼ì²âUSBÊÇ·ñ¶Ï¿ª,·µ»Ø0±íÊ¾³ÌÐò¶Ï¿ª£¬·µ»Ø1±íÊ¾»¹¼ÌÐø±£³ÖÁ¬½Ó
* Input          : None
* Output         : INT
* Return         : None
*******************************************************************************/
int HostIsLink(void)
{
	int s;
	s = ERR_SUCCESS;//²Ù×÷³É¹¦
	if (UIF_DETECT )// Èç¹ûUSBÖ÷»ú¼ì²âµ½ÖÐ¶ÏÔò½øÐÐ´¦Àí
	{
			UIF_DETECT = 0;// Çå³ýÖÐ¶Ï±êÖ¾
			s = AnalyzeRootHub( );// ·ÖÎöÖÐ¶ÏµÄ×´Ì¬
			if ( s == ERR_USB_DISCON )
			{
				  return 0;
			}
	}
	return 1;
}
/*******************************************************************************
* Function Name  : initLight()
* Description    : ³õÊ¼»¯µÆ¶¨Ê±Æ÷
* Input          : UINT8 *com
* Output         : int
* Return         : None
*******************************************************************************/
void initLight()
{
		TMOD = TMOD & ~bT0_M1 | bT0_M0;			//½ö½öÉèÖÃ¶¨Ê±Æ÷0¶ø²»ÉèÖÃ¶¨Ê±Æ÷1£¬ÒòÎª¶¨Ê±Æ÷1ÒÑ¾­ÓÃÓÚ´®¿Ú
		TH0=0xd8;
		TL0=0xf0;
		TR0=1;      //ÔÊÐí¿ªÆô¶¨Ê±Æ÷ÖÐ¶Ï
		ET0=1;
		EA = 1;
}


void printData(UINT8 *dat,UINT16 length)
{
	UINT16 i = 0;
	printf("\n out:");
    for ( i = 0; i < length; i ++ )
    {
        printf("x%02X ",(UINT16)dat[i]);	 
    }		
	printf("\n");
}


void ADB_OutputData(UINT8 *dat,UINT16 length,UINT16 timeout)
{
    UINT8 len,endp,s,i = 0;
    SelectHubPort( 0 );
	printData(dat,length);                                             // 
	while(length)
	{
    endp = ThisUsbDev.GpVar[1];
    if ( endp & USB_ENDP_ADDR_MASK )                                  // 
    {
        if(length >= 64) len = 64;
        else len = length;			
        memcpy(TxBuffer,dat,len);
        UH_TX_LEN = len;
        s = USBHostTransact( USB_PID_OUT << 4 | endp & 0x7F, endp & 0x80 ? bUH_R_TOG | bUH_T_TOG : 0, timeout );// CH554¡ä?¨º?¨º???,??¨¨?¨ºy?Y,NAK2???¨º?
        if ( s == ERR_SUCCESS )
        {
            endp ^= 0x80;                                             // 
            ThisUsbDev.GpVar[1] = endp;
            dat += len;					
            length -= len;					
        }
    }
	  }
    SetUsbSpeed( 1 );                                                 // 
}

UINT8 * ADB_InputData(UINT8 *dat,UINT8 *length,UINT16 timeout)
{ 
		UINT8 endp,s,i = 0;
    UINT8 *p = dat;
    int len;
    SelectHubPort( 0 );                                             // Ñ¡Ôñ²Ù×÷Ö¸¶¨µÄROOT-HUB¶Ë¿Ú,ÉèÖÃµ±Ç°USBËÙ¶ÈÒÔ¼°±»²Ù×÷Éè±¸µÄUSBµØÖ·
    endp = ThisUsbDev.GpVar[0];
    if ( endp & USB_ENDP_ADDR_MASK )                                  // 
    {
        s = USBHostTransact( USB_PID_IN << 4 | endp & 0x7F, endp & 0x80 ? bUH_R_TOG | bUH_T_TOG : 0, timeout );// CH554´«ÊäÊÂÎñ
        {
            endp ^= 0x80;                                             // Í¬²½±êÖ¾·­×ª
            ThisUsbDev.GpVar[0] = endp;
            len = USB_RX_LEN;
            *length = len;
            if ( len )
            {
                printf("in: ");
                for ( i = 0; i < len; i ++ )
                {
                    p[i] = RxBuffer[i];
                    printf("x%02X ",(UINT16)(RxBuffer[i]) );
                }
                printf("\n");
				mDelaymS(15);
            }
        }
    }
    SetUsbSpeed( 1 );                                                 // ÉèÖÃÎªÈ«ËÙµÄUSB
	return p;
}

UINT8 * ADB_Receive(UINT8 *dat,UINT8 length)
{
	UINT8 offset=0;
	UINT8 len;
	UINT8 *p = dat;
	while(1){
		ADB_InputData(&dat[offset], &len, 0xffff);
		if (len!=0){
			offset += len;
			if (offset >= length)
				break;
		}
		else{
			// check error
			// if usb detach
		}
	}
	return p;
}
/*******************************************************************************
* Function Name  : sendOK(UINT8 *a)
* Description    : ¸ù¾Ý½ÓÊÕµ½µÄÃüÁî¸Ä±äÊý¾ÝÊÕ·¢·½µÄID£¬Êä³öCLSEÖ¸Áî½øÐÐ»ØÓ¦
* Input          : UINT8 *com
* Output         : int
* Return         : None
*******************************************************************************/
void sendClose(UINT8 *com)
{
	int i;
	for(i =4;i<8;i++)
	{
		adb_host_close[i]=com[i+4];
	}
  for(i =4;i<8;i++)
	{
		adb_host_close[i+4]=com[i];
	}
	ADB_OutputData(adb_host_close,sizeof(adb_host_close),0xFFFF );
	//printf("\n clse:");
	//printData(adb_host_close,24);
}
/*******************************************************************************
* Function Name  : sendOK(UINT8 *a)
* Description    : ¸ù¾Ý½ÓÊÕµ½µÄÃüÁî¸Ä±äÊý¾ÝÊÕ·¢·½µÄID£¬Êä³öOKAYÖ¸Áî½øÐÐ»ØÓ¦
* Input          : UINT8 *com
* Output         : int
* Return         : None
*******************************************************************************/
void sendOK(UINT8 *com)
{
  int i;
	for(i =4;i<8;i++)
	{
		adb_okay_command[i]=com[i+4];
	}
  for(i =4;i<8;i++)
	{
		adb_okay_command[i+4]=com[i];
	}
	ADB_OutputData(adb_okay_command,sizeof(adb_okay_command),0xFFFF );
	//printf("\n okay:");
	//printData(adb_okay_command,24);
}
/*******************************************************************************
* Function Name  : comper(UINT8 * a,UINT8 * b)
* Description    : ÅÐ¶ÏÁ½¸ö×Ö·û´®µÄÇ°ËÄ¸ö×ÖÄ¸ÊÇ·ñÍêÈ«ÏàÍ¬¡£ÓÃÓÚÅÐ¶ÏÖ¸ÁîµÄÀàÐÍ
* Input          : UINT8 * a,UINT8 * b
* Output         : int
* Return         : None
*******************************************************************************/
UINT8* comper(UINT8 * a,UINT8 * b)
{
	int i = 0;
	for(i=0;i<4;i++)
	{
		//printf("comper ,i=[%d],p:%x,d:%x\n",i,(UINT16)*(a+i),(UINT16)*(b+i));
		if(a[i]!=b[i]) return 0;
	}
	return strstr(a,b);
}
/*******************************************************************************
* Function Name  : comstr(UINT8 * a,UINT8 * b)
* Description    : ÅÐ¶ÏÇ°Ò»¸ö×Ö·û´®ÊÇ·ñ°üº¬ÓÐºóÒ»¸ö×Ö·û´®
* Input          : UINT8 * a,UINT8 * b
* Output         : int
* Return         : None
*******************************************************************************/
int comstr(UINT8 *a,UINT8 *b)
{
	int i,n;
	int length = 0;
	UINT8 *p=a;	 
	length = preLen-4;
	//printf("\n ---------------------%d----------------",length);	
//		printf("comper dest:%x,%x,%x,%x\n",(unsigned char)b[0],(unsigned char)b[1],(unsigned char)b[2],(unsigned char)b[3]);
	for(n=0;n<length;n++)
	{
		for(i=0;i<4;i++)
		{
		//printf("comstr [%d],i=[%d],p:%x,d:%x\n",n,i,(UINT16)*(p+i),(UINT16)*(b+i));
		if(p[i]!=b[i]) break ;
		}
	  if(i==4) return 1;
	  p++;
	}
  return 0;	
}
/*******************************************************************************
* Function Name  : setLight(UINT8 *power,UINT8 *status)
* Description    : ÉèÖÃµçÔ´µÆºÍ×´Ì¬µÆµÄ×´Ì¬
* Input          : UINT8* 
* Output         : None
* Return         : None
*******************************************************************************/
void setLight(UINT8 *power,UINT8 *status)
{
		if(comper(power,"ONON")) power_light =3;  //µÆÁÁ
		if(comper(power,"OFFS")) power_light =0;  //¹Ø±ÕµÆ
	/////////////////////////////////////////////////////
		if(comper(status,"ONON")) status_light =3;
		if(comper(status,"OFFS")) status_light =0;
		if(comper(status,"QUICK")) status_light =2;
		if(comper(status,"SLOW")) status_light =1;	
}
 /*******************************************************************************
* Function Name  : checkRes(UINT8 * a,UINT8 * b)
* Description    : ÅÐ¶ÏÊÕµ½µÄÖ¸ÁîÊÇ·ñÊÇ·¢ËÍÖ¸ÁîµÄÏìÓ¦
* Input          : UINT8 *id,UINT8 *command
* Output         : int
* Return         : None
*******************************************************************************/
int checkRes(UINT8 * com,UINT8 * id)
{
	int i = 0;
	for(i=4;i<8;i++)
	{
		if(com[i+4]!=id[i]) return 0;
	}
	return 1;
}
 /*******************************************************************************
* Function Name  : changeCom(UINT8 *id,UINT8 *command)
* Description    : ¸ù¾ÝÊÕµ½µÄadbÖ¸Áî¸Ä±ä·¢ËÍÖ¸ÁîÖÐµÄ½ÓÊÕ·½ID
* Input          : UINT8 *id,UINT8 *command
* Output         : UINT8 *
* Return         : None
*******************************************************************************/
void changeCom(UINT8 *id,UINT8 *command)
{
	int i;
    for(i =4;i<8;i++)
	{
		command[i+4]=id[i];
	}
	printData(command,24);	
}
 /*******************************************************************************
* Function Name  : getId(UINT8 *com)
* Description    : ½ÓÊÜadb·µ»ØÖ¸Áî£¬²¢½«½ÓÊÕµ½µÄÊý¾Ý¿½±´µ½Êý×éadb_tmpÖÐ¡£ÓÃÓÚ»ñÈ¡Êý¾ÝµÄ·¢ËÍIDºÍ½ÓÊÕID
* Input          : UINT8 *com
* Output         : None
* Return         : None
*******************************************************************************/
void getId(UINT8 *com)
{
	int i;
	for(i=0;i<24;i++)
	{
		adb_tmp[i] = com[i];
	}
}
 /*******************************************************************************
* Function Name  : copyBuffer(UINT8 *com)
* Description    : ¸ÃÃüÁî½«Êý¾Ý¿½±´µ½buffer_tempÖÐ£¬ÓÃÓÚÆ´½Ó×Ö·û´®
* Input          : UINT8 *com
* Output         : None
* Return         : None
*******************************************************************************/
void copyBuffer(UINT8 *com,int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		buffer_temp[i+preLen] = com[i];
		
	}
	preLen = preLen + len;
	printf("\n ================= \n");
	printData(buffer_temp,preLen);
	printf("\n ================= \n");
}
 /*******************************************************************************
* Function Name  : recvCom(UINT8 *ID,UINT8 *buffer,UINT8 *len)
* Description    : ½ÓÊÜadb·µ»ØÖ¸Áî£¬²¢ÅÐ¶ÏÊÇ²»ÊÇµ±Ç°·¢ËÍÖ¸ÁîµÄÏìÓ¦£¬¶ÔWRTE£¬OKAY£¬CLSE½øÐÐÅÐ¶Ï
									¸Ãº¯ÊýÓÃÓÚÈ¡´úÖ÷³ÌÐòÖÐ³öÏÖµÄADB_InputData
* Input          : UINT8 *ID,UINT8 *buffer,UINT8 *len
* Output         : None
* Return         : None
*******************************************************************************/
void recvCom(UINT8 *ID,UINT8 *buffer,UINT8 *len)
{		
UINT8 *com;
UINT8 *full;
int buffer_len=0;
preLen=0;	
while(1)
{
	if(!HostIsLink())
	{
		connestion_state = 0;
		return;
	}
	mDelaymS(120);
		//com =  ADB_InputData(buffer,&len,0xFFFF);
		com = ADB_Receive(buffer,24);
		if(checkRes(com,ID))  //Èç¹ûÊÕµ½µÄÖ¸ÁîÊÇ¶Ô·¢³öÖ¸ÁîµÄ»ØÓ¦
		{
			if(comper(com,"OKAY"))  //ÊÕµ½µÄÊÇOKAYÖ¸Áî
				{
					getId(com);	
					printf("---------okay \n");
				}
			if(comper(com,"WRTE"))  //ÊÕµ½µÄÊÇWRTEÖ¸Áî
				{
					printf("---------wrte \n");
					buffer_len = com[12];
					while(preLen<buffer_len){
						com = ADB_InputData(buffer,&len,0xFFFF);
						copyBuffer(com,USB_RX_LEN);//½«Ã¿Ò»´ÎwrteÐ´ÈëµÄÄÚÈÝ´æÈëµ½buffer_tempÀïÃæ¡£
					}
					if(comstr(buffer_temp,"No such file"))
					{
						printf("\n--------------should install app first------------\n");
						SHOULD_INSTALL = 1;
						mDelaymS(15);
						sendOK(adb_tmp);
						Usb_Adb_status = ACTIVITY;
						setLight("ONON","SLOW");
						break;
					}else if(comstr(buffer_temp,"busy")){//has been usded,only open hall
						Usb_Adb_status = ACTIVITY;
						printf("\n--------------busy------------\n");
					} else if( comstr( buffer_temp, "denied") || comstr( buffer_temp, "cannot open for write")){
						Usb_Adb_status = CAT;  
					} else if(comstr(buffer_temp,"Flydigi")){//this is for dirve command
						strart_drive_times = 1;
						mDelaymS(15);
						sendOK(adb_tmp);
						printf("\n---------drive done----------- \n");
						Usb_Adb_status = ACTIVITY;
						break;
					}else{//diff passed,ready to chmod
						Usb_Adb_status = CHMOD;
					}
					mDelaymS(15);
					sendOK(adb_tmp);
					preLen = 0;
				}
			}
			if(comper(com,"CLSE"))  //ÊÕµ½µÄÊÇCLSEÖ¸Áî
			{
			  printf("---------clse \n");
				//getId(com);
				mDelaymS(15);
				sendClose(adb_tmp);	
				break;
			}
		}
}

/*******************************************************************************
* Function Name  : recvInstall(UINT8 *ID,UINT8 *buffer,UINT8 *len)
* Description    : ½ÓÊÜadb·µ»ØÖ¸Áî£¬²¢ÅÐ¶ÏÊÇ²»ÊÇµ±Ç°·¢ËÍÖ¸ÁîµÄÏìÓ¦£¬¶ÔWRTE£¬OKAY£¬CLSE½øÐÐÅÐ¶Ï
									ÌØ±ðÕë¶ÔµÄÊÇInstall-app×´Ì¬ÏÂµÄÅÐ±ð
* Input          : UINT8 *ID,UINT8 *buffer,UINT8 *len
* Output         : None
* Return         : None
*******************************************************************************/
void recvInstall(UINT8 *ID,UINT8 *buffer,UINT8 *len)
{	

UINT8 *com;	
while(1)
{
	if(!HostIsLink())
	{
		connestion_state = 0;
		return;
	}
	mDelaymS(100);
		//com =  ADB_InputData(buffer,&len,0xFFFF);
		com = ADB_Receive(buffer,24);
		if(checkRes(com,ID))  //Èç¹ûÊÕµ½µÄÖ¸ÁîÊÇ¶Ô·¢³öÖ¸ÁîµÄ»ØÓ¦
		{
			if(comper(com,"OKAY"))  //ÊÕµ½µÄÊÇOKAYÖ¸Áî
				{
					getId(com);	
					printf("---------okay \n");
				}
			if(comper(com,"WRTE"))  //ÊÕµ½µÄÊÇWRTEÖ¸Áî
				{
					printf("------------wrte \n");
					com = ADB_InputData(buffer,&len,0xFFFF);
					printf("---------installing app");
					break;
				}
		}
					
}
}
void recvShell(UINT8 *ID,UINT8 *buffer,UINT8 *len)
{
UINT8 *com;				
while(1)
{

	if(!HostIsLink())
	{
		connestion_state = 0;
		return;
	}
	mDelaymS(100);
		//com =  ADB_InputData(buffer,&len,0xFFFF);
		com = ADB_Receive(buffer,24);
		if(checkRes(com,ID))  //Èç¹ûÊÕµ½µÄÖ¸ÁîÊÇ¶Ô·¢³öÖ¸ÁîµÄ»ØÓ¦
		{
			if(comper(com,"OKAY"))  //ÊÕµ½µÄÊÇOKAYÖ¸Áî
				{
					getId(com);	
					printf("---------okay \n");
				}
			if(comper(com,"WRTE"))  //ÊÕµ½µÄÊÇWRTEÖ¸Áî
				{
					printf("---------wrte \n");
					com = ADB_InputData(buffer,&len,0xFFFF);
						mDelaymS(15);
						sendOK(adb_tmp);
						printf("shell done \n");
						break;
					
				}
			if(comper(com,"CLSE"))  //ÊÕµ½µÄÊÇCLSEÖ¸Áî
			{
			  printf("---------clse \n");
				mDelaymS(15);
				sendClose(adb_tmp);	
				break;
			}
		}
}
}

 void recvHall(UINT8 *ID,UINT8 *buffer,UINT8 *len)
{
UINT8 *com;
UINT8 *full;
int buffer_len=0;
preLen=0;				
while(1)
{
	if(!HostIsLink())
	{
		connestion_state = 0;
		return;
	}		
	mDelaymS(100);
		//com =  ADB_InputData(buffer,&len,0xFFFF);
		com = ADB_Receive(buffer,24);
		if(checkRes(com,ID))  //Èç¹ûÊÕµ½µÄÖ¸ÁîÊÇ¶Ô·¢³öÖ¸ÁîµÄ»ØÓ¦
		{
			if(comper(com,"OKAY"))  //ÊÕµ½µÄÊÇOKAYÖ¸Áî
				{
					getId(com);	
					printf("---------okay \n");
				}
			if(comper(com,"WRTE"))  //ÊÕµ½µÄÊÇWRTEÖ¸Áî
				{
					printf("---------wrte \n");
					buffer_len = com[12];
					while(preLen<buffer_len){
						com = ADB_InputData(buffer,&len,0xFFFF);
						copyBuffer(com,USB_RX_LEN);//½«Ã¿Ò»´ÎwrteÐ´ÈëµÄÄÚÈÝ´æÈëµ½buffer_tempÀïÃæ¡£
					}
					if(comstr(buffer_temp,"Error"))
					{
						printf("\n--------------go to install app now------------\n");
						SHOULD_INSTALL = 1;
						mDelaymS(15);
						sendOK(adb_tmp);
						setLight("ONON","SLOW");
						Usb_Adb_status = INSTALL_APP;
						break;
					}else if(strart_drive_times==1){//has been usded,only open hall
						Usb_Adb_status = END;
						setLight("ONON","ONON");
						mDelaymS(15);
						sendOK(adb_tmp);
						printf("\n--------------go to end------------\n");
						break;
					}else {
						Usb_Adb_status = END;
						setLight("ONON","ONON");
						mDelaymS(15);
						sendOK(adb_tmp);
					}
					preLen = 0;
				}
				if(comper(com,"CLSE")){
					printf("---------clse \n");
					mDelaymS(15);
					sendClose(adb_tmp);	
					break;
				}
		}
}
}
 /*******************************************************************************
* Function Name  : recvAdb(UINT8 *ID,UINT8 *buffer,UINT8 *len,UINT8 *type)
* Description    : ÓÃÓÚ¸ù¾Ý²»Í¬µÄÇé¿öµ÷ÓÃ²»Í¬µÄ½ÓÊÕº¯Êý¡£·ÖÎªCOMN,SHEL,DRIV,INDTËÄÖÖÇé¿ö
* Input          : UINT8 *ID,UINT8 *buffer,UINT8 *len,UINT8 *type
* Output         : None
* Return         : None
*******************************************************************************/
void recvAdb(UINT8 *ID,UINT8 *buffer,UINT8 *len,UINT8 *type)
{	
	if(comper(type,"COMN")) recvCom(ID,buffer,len);
	if(comper(type,"SHEL")) recvShell(ID,buffer,len);
	if(comper(type,"DRIV")) recvCom(ID,buffer,len);
	if(comper(type,"HALL")) recvHall(ID,buffer,len);
	if(comper(type,"INST")) recvInstall(ID,buffer,len);
}