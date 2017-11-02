/********************************** (C) COPYRIGHT *******************************
* File Name          : USBHostHUB_KM.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        :
*******************************************************************************/
#include ".\ComLib.C"
#include <stdio.h>
#include <string.h>
main( )
{  
	  UINT8 *com;
    UINT8 s,len;
    UINT8 buffer[64];
    UINT16  loc;
    CfgFsys( );
    mInitSTDIO( );                                                              //?aá?è??????úí¨1y′??ú?à???Yê?1y3ì
    initLight();//    初始化灯的状态
	printf( "Start @ChipID=%02X\n", (UINT16)CHIP_ID );
    InitUSB_Host( );
	printf( "Wait Device In\n" );
	INIT_MODE:
	while(1)
	{
	connestion_state = 1;
    FoundNewDev = 0;
	  Usb_Adb_status = NOT_CONNECT;   
		setLight("OFFS","OFFS");                                                 //初始状态式灯是关闭的
		s = ERR_SUCCESS;                                                         //操作成功
		if ( UIF_DETECT )                                                        // 如果USB主机检测到中断则进行处理
		{
				UIF_DETECT = 0;                                                      // 清除中断标志
				s = AnalyzeRootHub( );                                               // 分析中断的状态
				if ( s == ERR_USB_CONNECT )
				{
						FoundNewDev = 1;
						setLight("ONON","QUICK");
				}
		}
		if ( FoundNewDev )                                                       // 如果有新的USB设备插入
		{
				FoundNewDev = 0;                                                     // 等待稳定
				s = EnumAllRootDevice( );                                            // 枚举所有ROOT-HUB端口的USB设备
				if ( s != ERR_SUCCESS )
				{
						printf( "EnumAllRootDev err = %02X\n", (UINT16)s );
				}
		}
		loc = SearchTypeDevice( USB_DEV_SUBCLASS_ADB );
		if ( loc != 0xFFFF )                                                     // 找到了设备。
		{
			while(1)
			{
			switch( Usb_Adb_status ){
			 	case NOT_CONNECT:
					printf( "Query ADB @%04X\n", loc );
					printf("\n\r ==============\n\r");
					printf("\n START connection\n");
					ADB_OutputData(adb_connect_command,sizeof(adb_connect_command),0xFFFF );
					mDelaymS(5);
					ADB_OutputData(adb_connect_param,sizeof(adb_connect_param) ,0xFFFF);
				  //判断收到的指令是否是AUTH¨
				  com = ADB_InputData(buffer,&len,0xFFFF);
					if(!HostIsLink()) 
					{
						//Usb_Adb_status =NOT_CONNECT;
						goto INIT_MODE;
					}
					if(comper(com,"AUTH"))
					{
						Usb_Adb_status =AUTH;
						ADB_InputData(buffer,&len,0xFFFF);
					}
					else if(comper(com,"CNXN"))
					{
						Usb_Adb_status =DIFF;
					}									 
					printf("\n END connection\n");
					printf("\n\r ==============\n\r");
				break;
				case AUTH:
				  printf("\n\r ==============\n\r");
					printf("\n START Auth command and data\n");
					ADB_OutputData(adb_auth_command,sizeof(adb_auth_command),0xFFFF );
					mDelaymS(15);
					ADB_OutputData(adb_auth_param,sizeof(adb_auth_param),0xFFFF );
					if(!HostIsLink()) goto INIT_MODE;
					ADB_InputData(buffer,&len,0xFFFF);
					ADB_InputData(buffer,&len,0xFFFF);
					printf("\n END Auth command and data\n");
					printf("\n\r ==============\n\r");
					Usb_Adb_status = KEY_AUTH;
				break;
				case KEY_AUTH:
					printf("\n\r ==============\n\r");
					printf("\n START key command and data\n");
					ADB_OutputData(adb_key_command,sizeof(adb_key_command),0xFFFF );
					mDelaymS(15);
					ADB_OutputData(adb_public_key2,sizeof(adb_public_key2),0xFFFF );
					while(1){
					  mDelaymS(20);
						if(!HostIsLink()){ 
						connestion_state = 0;
						break;
						}
					  com = ADB_InputData(buffer,&len,0xFFFF);
						if(comper(com,"CNXN"))  //如果收到了产品信息，则跳出
						{	
								break;
							}
						}
					if(!connestion_state) goto INIT_MODE;
					printf("\n END key command and data\n");
					printf("\n\r ==============\n\r");
					Usb_Adb_status = DIFF;
				break;
				case DIFF:				
					printf("\n\r ==============\n\r");
					printf("\n\r START diff in\n\r");
					ADB_OutputData(adb_diff_command,sizeof(adb_diff_command),0xFFFF );
					mDelaymS(15);
					ADB_OutputData(cmd_diff,sizeof(cmd_diff),0xFFFF );
					recvAdb(adb_diff_command,buffer,&len,"COMN");
					if(!connestion_state) goto INIT_MODE;
					printf("\n\r END diff in\n\r");
					printf("\n\r ==============\n\r");
				break;
				case INSTALL_APP:				
					printf("\n\r ==============\n\r");
					printf("\n\r START INSTALL_APP\n\r");
					//send----->shell:am start -a android.intent.action.VIEW -d http://www.flydigi.com/Software/Motionelf.apk
					ADB_OutputData(adb_install_command,sizeof(adb_install_command),0xFFFF );
					mDelaymS(15);
					ADB_OutputData(cmd_installgamehall,sizeof(cmd_installgamehall),0xFFFF );
					recvAdb(adb_install_command,buffer,&len,"INST");
					if(!connestion_state) goto INIT_MODE;
					setLight("ONON","SLOW");
					printf("\n\r END dINSTALL_APP\n\r");
					printf("\n\r ==============\n\r");
					Usb_Adb_status = END;      //反正无论如何都要返回最开始的状态
				break;
				case CAT:				
					printf("\n\r ==============\n\r");
					printf("\n\r START diff in\n\r");
					ADB_OutputData(adb_cat_command,sizeof(adb_cat_command),0xFFFF );
					mDelaymS(15);
					ADB_OutputData(cmd_diff,sizeof(cmd_diff),0xFFFF );
					recvAdb(adb_diff_command,buffer,&len,"COMN");
					if(!connestion_state) goto INIT_MODE;
					printf("\n\r END diff in\n\r");
					printf("\n\r ==============\n\r");
				break;
				case CHMOD:
					printf("\n\r ==============\n\r");
					printf("\n START chmod\n");
					//send cmd---------->shell:chmod 777 ./data/local/tmp/motionelf_server
					ADB_OutputData(adb_chmod_command,sizeof(adb_chmod_command),0xFFFF );
					mDelaymS(15);
					ADB_OutputData(cmd_chmod111,sizeof(cmd_chmod111),0xFFFF );
					//recvCom(adb_chmod_command,buffer,&len);
					recvAdb(adb_chmod_command,buffer,&len,"COMN");
					if(!connestion_state) goto INIT_MODE;
					printf("\n END chmod\n");
					printf("\n\r ==============\n\r");
					Usb_Adb_status = START_DRIVE;
				break;
				
				case START_DRIVE:
			   	 	 printf("\n\r ==============\n\r");
					 printf("\n START driver\n");
					///////////step1,open shell///////////////////////////
					 ADB_OutputData(adb_shell_command,sizeof(adb_shell_command),0xFFFF );
					 mDelaymS(15);
					 ADB_OutputData(cmd_shell,sizeof(cmd_shell),0xFFFF );
					 recvAdb(adb_shell_command,buffer,&len,"SHEL");
					 if(!connestion_state) goto INIT_MODE;
					 if(comper(com,"CLSE")){
					 	Usb_Adb_status = START_DRIVE;
					 	continue;
					 }
					 ///////////step2,send 0x0A///////////////////////////
					 changeCom(adb_tmp,cmd_0a);	//修改输出指令
					 ADB_OutputData(cmd_0a,sizeof(cmd_0a),0xFFFF );
					 mDelaymS(15);
					 ADB_OutputData(&cmd,1,0xFFFF );
					 recvAdb(adb_shell_command,buffer,&len,"SHEL");
					 if(!connestion_state) goto INIT_MODE;
					 if(comper(com,"CLSE")){
					 	Usb_Adb_status = START_DRIVE;
					 	continue;
					 }
					 ////step:3,send command ---->./data/local/tmp/motionelf_server&
					 changeCom(adb_tmp,adb_drive_command);	//修改输出指令
					 ADB_OutputData(adb_drive_command,sizeof(adb_drive_command),0xFFFF );
					 mDelaymS(15);
					 ADB_OutputData(cmd_motionelf,sizeof(cmd_motionelf),0xFFFF );
					 recvAdb(adb_shell_command,buffer,&len,"DRIV");
					 if(!connestion_state) goto INIT_MODE;
					 Usb_Adb_status = ACTIVITY;								
	      		break;
		  
        		case ACTIVITY:
					printf("\n\r ==============\n\r");
					printf("\n START hall\n");
					//step 1:send open command,shell:am start -n com.game.motionelf/com.game.motionelf.activity.ActivityStart -e action active_driver
					ADB_OutputData(adb_activity_command,sizeof(adb_activity_command),0xFFFF );
					mDelaymS(15);
					ADB_OutputData(cmd_startactivity,sizeof(cmd_startactivity),0xFFFF );
					recvAdb(adb_activity_command,buffer,&len,"HALL");
					if(!connestion_state) goto INIT_MODE;
					printf("\n END hall\n");
					printf("\n\r ==============\n\r");
					//setLight("ONON","ONON");
					//Usb_Adb_status = END;		
				break;
				case END:
				break;
			}			
    }
	}
}
}

