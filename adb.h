#ifndef _ADB_H_
#define _ADB_H_

#define A_SYNC 0x53594E43//0x434e5953
#define A_CNXN 0x434E584E//0x4e584e43
#define A_OPEN 0x4F50454E//0x4e45504f
#define A_OKAY 0x4F4B4159//0x59414b4f
#define A_CLSE 0x434C5345//0x45534c43
#define A_WRTE 0x57525445//0x45545257
#define A_AUTH 0x41555448//0x48545541
#define MCU_ID 0x00003456

#define EDIAN_CHANGE(DATA)    ( ( (DATA&0xFF000000)>>24 ) + ((DATA&0x00FF0000)>>8) + ((DATA&0x0000FF00)<<8)	 +((DATA&0x000000FF)<<24))
//#define EDIAN_CHANGE(DATA)		 DATA
struct amessage {
    unsigned long  command;       /* command identifier constant      */
    unsigned long  arg0;          /* first argument                   */
    unsigned long  arg1;          /* second argument                  */
    unsigned long  data_length;   /* length of payload (0 is allowed) */
    unsigned long  data_check;    /* checksum of data payload         */
    unsigned long  magic;         /* command ^ 0xffffffff             */
};



#define DETEC_USB_CONNECT()      do{\
										if(  !HostIsLink()){\
											Usb_Adb_status=NOT_CONNECT; \																				
											goto START;\
										}\
								    }while(0)

																	
#define USB_RECV_PARAM()         do{ \
                                     adb_message.data_length = EDIAN_CHANGE(adb_message.data_length);\
									 memset( adb_buffer, 0x00, sizeof( adb_buffer) );\
									 while(1){\									  									                                      					  
									      if( HostRcvPacket(&gBulkInPipe, adb_buffer,(WORD)(adb_message.data_length)))\
										       break;\
									      DETEC_USB_CONNECT();\
								      }\										
								    }while(0)

#define USB_RECV_ADB()       do{\
                                   while(1){\
										if( HostRcvPacket(&gBulkInPipe, (uint8_t*)&adb_message, sizeof(adb_message)) )\										    
											   break;\											
										DETEC_USB_CONNECT();\									    
									}\
								}while(0)									 

#define USB_RECV_ADB_CHECK()       do{\
                                   while(1){\
										if( HostRcvPacket(&gBulkInPipe, (uint8_t*)&adb_message, sizeof(adb_message)) )\
										    if(  adb_message.arg1 == localID )\
											   break;\											
										DETEC_USB_CONNECT();\									    
									}\
								}while(0)

/*#deinfe USB_RECV_ADB()
																 while(1){
										 if( UsbHostRcvPacket(&gBulkInPipe, (uint8_t*)&adb_message, sizeof(adb_message), 10) ){
											 for( i = 0 ; i < 24; i++)
											   printf(" 0x%02x ", ((uint8_t*)&adb_message)[i]);
											 break;
										 }
									 }*/	

#define PRINTF_ADB_HEADER()      do{\
										printf("\n adb header");\
							            for( i = 0 ; i < 24; i++)\
							                 printf(" %bx ", ((uint8_t*)&adb_message)[i]);\
							            printf("\n");\
								   }while(0)

#define SEND_CLSE_COMMAND()  do{\
	                                  adb_send = (struct amessage*)adb_host_close;\
									  adb_send->arg0 = adb_message.arg1;\
									  adb_send->arg1 = adb_message.arg0;\
									  adb_send->data_length = 0;\
									  adb_send->data_check =0;\
	                                  HostSendPacket(&gBulkOutPipe, adb_host_close, sizeof(adb_host_close));\
                               }while(0)

#define SEND_OKAY_COMMAND()    do{\
									adb_send = (struct amessage*)adb_okay_command;\
									adb_send->arg0 = localID;\
									adb_send->arg1 = adb_message.arg0;\
									adb_send->data_length = 0;\
									adb_send->data_check =0;\
                                    HostSendPacket(&gBulkOutPipe, adb_okay_command, sizeof(adb_okay_command));\
								}while(0)

/*#define RECV_ADB_WRTE(buffer)         buffer = adb_buffer;\
									  do{\
                                           HostRcvPacket(&gBulkInPipe, (uint8_t*)&adb_message, sizeof(adb_message));\
								           if(adb_message.command == A_WRTE ){\ 
								             while(1){\									  									                                      					  
									           if( HostRcvPacket(&gBulkInPipe, adb_buffer,(WORD)(adb_message.data_length)))\
										   break;\
									      }else if( adb_message.command == A_CLSE )\
									    break;\
								 }while(1)\
							 */
#define RECV_ADB_WRTE()   do{\
								buf = buffer;\
								memset(buffer, 0x00,sizeof( buffer));\
								do{\
									USB_RECV_ADB();\
									if(adb_message.arg1 != localID)\
									     continue;\							      
									if( adb_message.command == A_CLSE){\ 								       								        
											SEND_CLSE_COMMAND();\												 
											break;\
									}else if( adb_message.command == A_WRTE ){\
											USB_RECV_PARAM();\									  									  									  
											memcpy(buf, adb_buffer,(WORD)(adb_message.data_length ));\
											buf += (adb_message.data_length );\
											SEND_OKAY_COMMAND();\
									}\
								}while(1);\
						    }while(0)

#endif
	 