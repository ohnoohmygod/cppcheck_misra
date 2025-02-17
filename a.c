/**
 * @file call_mip.c
 * @author Dong Ming (dong_ming@neusoft.com)
 * @brief 
 * @version 0.1
 * @date 2021-02-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

 #include "neu_xcall.h"
 #include "call_app.h"
 #include "call_audio.h"
 #include "call_mip.h"
 #include "call_rpc.h"
 #include "call_tsp.h"
 #include "neu_at_api.h"
 #include "geely_vehicle_mode.h"
 
 #include "Arbiter_auto.pb.h"
 #include "SuperVisor_auto.pb.h"
 #include "rtxsrc/rtxDiag.h"
 #include "CallManager_auto.pb.h"
 #include "Parameter.h"
 #include "EcuComm_def.h"
 #include "neu_utility_timer.h"
 
 Type_Bool  Backup_BatteryFlag= FALSE;
 E_IPLM_RG_Status_st nstRgStatus = IPLM_RG_STATUS_AVAILABLE;
 static OpXCallStatus_Notification gXCallStatus;
 
 
 static MIP_RETURN_CODE nswCall_mip_DialTestEcall(void);
 static MIP_RETURN_CODE nswCall_mip_DialManualEcall(void);
 static MIP_RETURN_CODE nswCall_mip_DialAutoEcall(void);
 static MIP_RETURN_CODE nswCall_mip_SetEcallNumber(st_debug_setecall_info* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_SetIcallNumber(st_debug_seticall_info* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_SetBcallNumber(st_debug_setbcall_info* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_SetTestEcallNumber(st_debug_settestcall_info* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_SetCallbackDuraion(st_debug_set_duration* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_CallAutoAnswerTime(st_did_callAutoAnswerTime* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_EcallAutoDialAttempts(st_did_ecallAutoDialAttempts* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_EcallCCFT(st_did_ecallCCFT* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_DialDuration(st_did_ecallDialDuration* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_SMS_FallbackNumber(st_did_ecallSMSfallbackNum* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_EcallTestNumber(st_did_ecallTestNum* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_EcallDialAttempts(st_did_ecallTestDialAttempts* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_TestModeEndDistance(st_did_testModeEndDistance* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_Set_DID_RegistrationPeriod(st_did_regiPeriod* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_GetCallNumber(st_mip_xcall_info *pstOutputData);
 static Type_sWord nswCall_mip_IPCP_UnwrapPackage(Type_uByte* pubBuffer, void* pPayload, E_ASN_OPERATION_ID* operationId, E_ASN_OPERATION_TYPE* operationType);
 static Type_sWord nswCall_mip_IHUStatusReport(OpIHUStatusReport_Notification* pstStatus);
 static Type_sWord nswCall_mip_XCallControl(OpXCallControl_Notification* pstXCallCtrl);
 static Type_sWord wswCall_mip_PremiumAudio(OpPremiumAudio_Response* pstPreAudio);
 static Type_sWord wswCall_mip_CrashInfo(OpCrashInfo_Notification* pstCrashInfo);
 static Type_sWord wswCall_mip_IpcpStreamCallback(Type_sWord ServiceID,void *pubRecvStreamData,Type_sWord DataLen);
 static Type_sWord nswCall_mip_IpLinkStreamCallback(Type_sWord ServiceID,void *pubRecvStreamData,Type_sWord DataLen);
 static Type_sWord wswCall_mip_IpLinkRGStsCallbackRegister(VOID);
 static Type_sWord wswCall_mip_IpLinkRGStsCallback(void *pubRecvStreamData,Type_sWord DataLen);
 static Type_sWord wswCall_mip_XcallStatusRequest();
 static MIP_RETURN_CODE nswCall_mip_rcv_SOS_dtc_state(st_sos_dtc_state* pstInputData);
 static MIP_RETURN_CODE nswCall_mip_network_state_noti(st_bc_nw_conn_status* pstInputData);
 
 typedef struct{
     uint8_t mipEventID;
     uint8_t mipModuleID;
     uint8_t mipServiceID;
     uint8_t reserved;
 }XCALL_Update_Status_Info;
 
 static XCALL_Update_Status_Info nstXcallStatusInfo_a[XCALL_COUNT_MAX];
 
 Type_sWord nswCall_mip_Init(void)
 {
     Type_sWord nswRet = RES_OK;
     ST_MIP_REG_INFO  stMipRegInfo;
     memset(&stMipRegInfo,0,sizeof(ST_MIP_REG_INFO));
 
     stMipRegInfo.async_response_handler = wvdCall_mip_AsyncCallBack;
     stMipRegInfo.broadcast_handler = wvdCall_mip_BroadcastCallBack;
     stMipRegInfo.eWorkMode = E_MIP_CLIENT_WORK_MODE_SINGLE_WORKER;
     //stMipRegInfo.eWorkMode = E_MIP_CLIENT_WORK_MODE_MULTI_WORKERS;
     stMipRegInfo.WorkerNum = 1; //it is 1 while eWorkMode is SINGLE
     //stMipRegInfo.WorkerNum = 2;
     stMipRegInfo.ModuleId = MID_SOC_XCALL_APP;
     snprintf((char *)(stMipRegInfo.ModuleName),D_MIP_MODULE_NAME_LEN,"%s","XcallMngr");
     stMipRegInfo.ServiceProvider_Handler = wstCall_mip_ServiceProvider;
     stMipRegInfo.Stream_Handler = wswCall_mip_StreamCallback;
 
     nswRet = wswMip_Init(&stMipRegInfo);
     NEU_LOG_CRIT("XcallMngr Init nswRet:%d\n", nswRet);
 
     nswRet = wswCall_mip_IpLinkRGStsCallbackRegister();
     NEU_LOG_CRIT("RG Status callback register nswRet:%d\n", nswRet);
 
     memset(&gXCallStatus,0,sizeof(gXCallStatus));
     gtm_eta = 0;
 
     return nswRet;
 }
 
 VOID nvdCall_mip_Exit(VOID)
 {
     return;
 }
 
 void wvdCall_mip_AsyncCallBack(NEU_MODULE_ID SrcMid,Type_sWord ServiceID,void *pstRespData)
 {
     return;
 }
 
 void wvdCall_mip_BroadcastCallBack(NEU_MODULE_ID SrcMid,Type_sWord TopicId,void *pstBcstData)
 {
     NEU_LOG_CRIT( "SrcMid:[%04x] ServiceID:[%x]\n", SrcMid, TopicId); 
     Type_sWord nswRet = RES_OK;
     Type_sWord nswCallState =0;
     switch(TopicId)
     {
         case BROADCAST_TOPIC_ID_EVERY_MODULE_SLEEP:
         {
             st_bc_sleep_msg *pstBcSleepMsg = (st_bc_sleep_msg *)pstBcstData;            
             NEU_LOG_CRIT("Sleep Ctrl: Layer=%d DeepSleep=%d",pstBcSleepMsg->Layer,pstBcSleepMsg->bDeepSleep);
             
             if(e_bc_sw_layer_e_bc_sw_layer_Application == pstBcSleepMsg->Layer)
             {
                 ST_NEU_QUEUE_DATA queueData;
                 bzero(&queueData, sizeof(ThreadQueueData));
                 ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
                 pData->srcThreadID = wswNue_call_getThreadID();
                 pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
                 pData->funcID = E_XCALL_PROC_SLEEP_NOTIFY;
                 queueData.uwDataLen = sizeof(ThreadQueueData);
                 wswNeuQueue_Push(&stCallQue, &queueData);
 
                 st_spv_collect_sleep_result_ntf stSpvSleepResultNtf;
                 memset(&stSpvSleepResultNtf, 0, sizeof(stSpvSleepResultNtf));
                 stSpvSleepResultNtf.Mid = MID_SOC_XCALL_APP;
                 stSpvSleepResultNtf.result = e_spv_pm_ctrl_result_e_spv_pm_ctrl_result_ok;
                 nswRet = wswMip_SendNotification(MID_SOC_XCALL_APP,MID_SOC_SUPERVISOR,SUPERVISOR_SERVICE_ID_COLLECT_SLEEP_RESULT,&stSpvSleepResultNtf);
             }
             break;
         }
         case BROADCAST_TOPIC_ID_EVERY_MODULE_WAKEUP:
         {
             st_bc_wakeup_msg *pstBcWakeupMsg = (st_bc_wakeup_msg *)pstBcstData;
             NEU_LOG_CRIT("Wakeup Ctrl: Layer=%d ",pstBcWakeupMsg->Layer);
             
             if(e_bc_sw_layer_e_bc_sw_layer_Application == pstBcWakeupMsg->Layer)
             {
                 // 休眠唤醒后通知APP点亮绿色LED
                 ST_NEU_QUEUE_DATA queueData;
                 bzero(&queueData, sizeof(ThreadQueueData));
                 ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
                 pData->srcThreadID = wswNue_call_getThreadID();
                 pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
                 pData->funcID = E_XCALL_PROC_WAKEUP_NOTIFY;
                 queueData.uwDataLen = sizeof(ThreadQueueData);
                 wswNeuQueue_Push(&stCallQue, &queueData);
 
                 st_spv_collect_wakeup_result_ntf stSpvWakeupResultNtf;
                 memset(&stSpvWakeupResultNtf, 0, sizeof(stSpvWakeupResultNtf));
                 stSpvWakeupResultNtf.Mid = MID_SOC_XCALL_APP;
                 stSpvWakeupResultNtf.result = e_spv_pm_ctrl_result_e_spv_pm_ctrl_result_ok;
                 nswRet = wswMip_SendNotification(MID_SOC_XCALL_APP,MID_SOC_SUPERVISOR,SUPERVISOR_SERVICE_ID_COLLECT_WAKEUP_RESULT,&stSpvWakeupResultNtf);
                 if(RES_OK != nswRet)
                 {
                     NEU_LOG_ERROR("Ntf Wakeup Result(%d) Failed!",stSpvWakeupResultNtf.result);
                 }
             }
             break;
         }
         case BROADCAST_TOPIC_ID_VEHICLE_MODE_CHANGE:
         {
             st_bc_vehicle_mode_change *pstVehicleMode = (st_bc_vehicle_mode_change *)pstBcstData;
             if(pstVehicleMode->powermode_valid)
             {
                 //wvdNetPMChange(pstVehicleMode->powermode);
                 if((gstXcallGlobalInfo.ubPowerMode != pstVehicleMode->powermode) && (POWERMODE_SLEEP == pstVehicleMode->powermode))
                 {
 #ifdef SOC_PLATFORM_QUECTEL
                     //Set Codec to mute
                     wswNeuSetAtCmd(93, 0);
 #endif
                 }
                 else if((gstXcallGlobalInfo.ubPowerMode != pstVehicleMode->powermode) && (POWERMODE_NORMAL == pstVehicleMode->powermode))
                 {
 #ifdef SOC_PLATFORM_QUECTEL
                     //Set Codec to unmute
                     wswNeuSetAtCmd(93, 9);
 #endif
                 }
                 gstXcallGlobalInfo.ubPowerMode = pstVehicleMode->powermode;
                 NEU_LOG_CRIT("PowerMode=[%d],PowerMode=[%d]\n",pstVehicleMode->powermode,gstXcallGlobalInfo.ubPowerMode);
             }
             if(pstVehicleMode->carmode_valid)
             {
                 gstXcallGlobalInfo.ubCarMode = pstVehicleMode->carmode;
                 NEU_LOG_CRIT("CarMode=[%d],CarMode=[%d]\n",pstVehicleMode->carmode,gstXcallGlobalInfo.ubCarMode);
                 if ((CARMODE_TRANSPORT == gstXcallGlobalInfo.ubCarMode) || (CARMODE_FACTORY == gstXcallGlobalInfo.ubCarMode))
                 {
                     nswCallState = nswCall_data_CallStateGet();
                     NEU_LOG_CRIT("nswCallstate:%d\n", nswCallState);
                     if(nswCallState != E_XCALL_STATE_IDLE)
                     {
                         ThreadQueueData nthData;
                         memset(&nthData, 0, sizeof(ThreadQueueData));
                         nthData.funcID = E_XCALL_PROC_XCALL_SYSTEM_RESET;
                         //Push queue
                         nswNeu_call_PushQueue(&stCallQue, &nthData);
                     }
                 }
             }
             if(pstVehicleMode->usagemode_valid)
             {
                 gstXcallGlobalInfo.ubUsageMode = pstVehicleMode->usagemode;	
                 NEU_LOG_CRIT("UsageMode=[%d],UsageMode=[%d\n",pstVehicleMode->usagemode,gstXcallGlobalInfo.ubUsageMode);
             }
             break;
         } 
         case BROADCAST_TOPIC_ID_BB_STATUS:
             {
                 st_bc_bb_status *pstBcBBStatus = (st_bc_bb_status *)pstBcstData;;
                 if(pstBcBBStatus->status == e_bc_bb_status_e_bc_bb_status_Backup_Battery)
                 {
                     //用来校验备电触发B,Icall触发检查标志
                     Backup_BatteryFlag=TRUE;
                     // 切替到备电时，音源也要切换
                     if((nstXcallSource == E_XCALL_SOURCE_STATUS_MAIN_IHU) && (glxcall_status.Calltype != E_XCALL_TYPE_NO_CALL))
                     {
                         //Set codec
                         nswCall_app_CodecPrepareSet(E_XCALL_SOURCE_STATUS_BACKUP);
                     }
                 }
                 else
                 {
                     Backup_BatteryFlag=FALSE;
                 }
             }
             break;
         case BROADCAST_TOPIC_ID_NETWORK_CONNECTION_STATUS:
             nswCall_mip_network_state_noti((st_bc_nw_conn_status *)pstBcstData);
             break;
         case BROADCAST_TOPIC_ID_SYS_TIME_STATUS:
             NEU_LOG_CRIT("System Time is Updated Ok!");
             // 休眠唤醒后通知APP点亮绿色LED
             ST_NEU_QUEUE_DATA queueData;
             bzero(&queueData, sizeof(ThreadQueueData));
             ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
             pData->srcThreadID = wswNue_call_getThreadID();
             pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
             pData->funcID = E_XCALL_PROC_TIME_UPDATE_NOTIFY;
             queueData.uwDataLen = sizeof(ThreadQueueData);
             wswNeuQueue_Push(&stCallQue, &queueData);
             break;
         case BROADCAST_TOPIC_ID_ETH_CONNECTION_STATUS:
             NEU_LOG_CRIT("ethernet notify!");
                         ST_Call_XcallSosState nstSosState = {0};
                 st_bc_eth_conn_status *pstEthConnStatus;
                     pstEthConnStatus =(st_bc_eth_conn_status *)pstBcstData;
 
                         if(pstEthConnStatus->status == e_bc_eth_conn_status_e_bc_eth_conn_status_connected)
             {
                 NEU_LOG_CRIT("ethernet OK!");
                 glxcall_status.Ethernet= E_XCALL_CALL_SOS_STATUS_VALID;
                                 nstSosState.StEthernet = E_XCALL_CALL_SOS_STATUS_VALID;
             }
             else if(pstEthConnStatus->status ==  e_bc_eth_conn_status_e_bc_eth_conn_status_disconnect)
             {
                 NEU_LOG_CRIT("ethernet NG!");
                 glxcall_status.Ethernet= E_XCALL_CALL_SOS_STATUS_INVALID;
                 nstSosState.StEthernet = E_XCALL_CALL_SOS_STATUS_INVALID;
 
             }
                         nvdXcall_app_SetSosState(E_XCALL_SOS_LIM_SOURCE_ETHERNET, nstSosState);
             break;
         case BROADCAST_TOPIC_ID_VEHICLESTS_UPDATE_GDPRSTATE:
             {
             st_bc_tbox_gdprsts *pstGDRPStatus;
             pstGDRPStatus =(st_bc_tbox_gdprsts *)pstBcstData;
             if(1 == pstGDRPStatus->GDPRStsPassenger)
             {
                 if(TRUE == pstGDRPStatus->GDPRSwitchSts[8])
                 {
                     NEU_LOG_CRIT("set GDPR Bcall On");
                     isBcallGDPRStatusOpen = TRUE;
                 }
                 else
                 {
                     NEU_LOG_CRIT("set GDPR Bcall Off");
                     isBcallGDPRStatusOpen = FALSE;
                 }
             }
             break;
             }
             case BROADCAST_TOPIC_ID_FACTRESET_PREPARE:
             {
                NEU_LOG_CRIT("hmi reset broadcast");
                st_hmis_factreset_ready_in  factreset;
                memset(&factreset, 0, sizeof(factreset));
 
                nswCallState = nswCall_data_CallStateGet();
                    NEU_LOG_CRIT("nswCallState:%d\n", nswCallState);
 
                if((E_XCALL_TYPE_MANUAL_ECALL_CN == glxcall_status.Calltype)  || (E_XCALL_TYPE_AUTO_ECALL_CN  == glxcall_status.Calltype))
                    {
                   if((nswCallState!=E_XCALL_STATE_IDLE)&&(nswCallState!=E_XCALL_STATE_ECALL_CALLBACK)&&(nswCallState!=E_XCALL_STATE_AUTO_ECALL_CALLBACK))
                   {
                     NEU_LOG_CRIT("reset Ng");
                     factreset.result = 1;
                     //此时不允许重启的情况下,需要标记,等ecall动作结束再立即重启,等同于手动按键8-45秒重启；
                     nswXcall_module_Trigger(E_XCALL_STATE_ECALL_BTN_RELEASED_8_45S);
                   }
                   else
                   {
                     NEU_LOG_CRIT("reset Ok");
                     factreset.result = 0;
                     //Notice to Arbiter
                     nswCall_mip_ArbiterPermissionReq(glxcall_status.Calltype, E_XCALL_ARBITER_PERMISSION_ACTION_CANCEL);
 
                     //Clear sleep lock
                     ST_Call_McuSleepLockReq nstSleepLockReq = {0};
                     nstSleepLockReq.nstSleepLock = COM_PM_STANDBYOUT_FACTOR_ECALL;
                     nstSleepLockReq.nubAction = TBOX_SLEEP_LOCK_ACTION_CANCEL;
                     nstSleepLockReq.nubType = SLEEP_LOCK_ACTION_SRC_INVALID;
                     nswRet = nswCall_rpc_SleepLockS2M(nstSleepLockReq);
                   }
                   nswRet = wswMip_SendNotification(MID_SOC_XCALL_APP,MID_SOC_HMI_SETTING_APP,HMISETTING_SERVICE_ID_FACTRESET_READY,&factreset);
                }
                else
                {
                 NEU_LOG_CRIT("reset Ok");
                 factreset.result = 0;
                 nswRet = wswMip_SendNotification(MID_SOC_XCALL_APP,MID_SOC_HMI_SETTING_APP,HMISETTING_SERVICE_ID_FACTRESET_READY,&factreset);
                }
             }
                         break;
         default:
             break;
     }
     
     return;
 }
 
 MIP_RETURN_CODE wstCall_mip_ServiceProvider(NEU_MODULE_ID SrcMid,Type_sWord ServiceID,void *pstInputData,void *pstOutputData)
 {
     MIP_RETURN_CODE Ret = MIP_RETURN_CODE_SUCCESS;
     const Type_uByte *inputData = NULL;
     NEU_LOG_CRIT("Receive From ModuleID(%04x) ServiceId(%d)\n",SrcMid,ServiceID);
 
     switch(ServiceID)
     {
         case CALLMANAGER_SERVICE_ID_DIAL_TESTECALL:
             Ret = nswCall_mip_DialTestEcall();
             break;
         case CALLMANAGER_SERVICE_ID_DIAL_MANUALECALL:
             Ret = nswCall_mip_DialManualEcall();
             break;
         case CALLMANAGER_SERVICE_ID_DIAL_AUTOECALL:
             Ret = nswCall_mip_DialAutoEcall();
             break;
         case CALLMANAGER_SERVICE_ID_SET_ECALL_NUMBER:
             Ret = nswCall_mip_SetEcallNumber((st_debug_setecall_info *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_BCALL_NUMBER:
             Ret = nswCall_mip_SetBcallNumber((st_debug_setbcall_info *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_ICALL_NUMBER:
             Ret = nswCall_mip_SetIcallNumber((st_debug_seticall_info *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_TESTCALL_NUMBER:
             Ret = nswCall_mip_SetTestEcallNumber((st_debug_settestcall_info *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_CALLBACK_DURATION:
             Ret = nswCall_mip_SetCallbackDuraion((st_debug_set_duration *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_GETCALLNUMBER:
             Ret = nswCall_mip_GetCallNumber((st_mip_xcall_info *)pstOutputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_CALL_AUTO_ANSWER_TIME:
             Ret = nswCall_mip_Set_DID_CallAutoAnswerTime((st_did_callAutoAnswerTime *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_ECALL_AUTO_DIAL_ATTEMPTS:
             Ret = nswCall_mip_Set_DID_EcallAutoDialAttempts((st_did_ecallAutoDialAttempts *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_ECALL_CCFT:
             Ret = nswCall_mip_Set_DID_EcallCCFT((st_did_ecallCCFT *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_ECALL_DIAL_DURATION:
             Ret = nswCall_mip_Set_DID_DialDuration((st_did_ecallDialDuration *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_ECALL_SMS_FALLBACK_NUMBER:
             Ret = nswCall_mip_Set_DID_SMS_FallbackNumber((st_did_ecallSMSfallbackNum *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_ECALL_TEST_NUMBER:
             Ret = nswCall_mip_Set_DID_EcallTestNumber((st_did_ecallTestNum *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_ECALL_TEST_DIAL_ATTEMPTS:
             Ret = nswCall_mip_Set_DID_EcallDialAttempts((st_did_ecallTestDialAttempts *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_TEST_MODE_END_DISTANCE:
             Ret = nswCall_mip_Set_DID_TestModeEndDistance((st_did_testModeEndDistance *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SET_DID_TEST_REGISTRATION_PERIOD:
             Ret = nswCall_mip_Set_DID_RegistrationPeriod((st_did_regiPeriod *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_SOS_DTC_STATE_NOTI:
             Ret = nswCall_mip_rcv_SOS_dtc_state((st_sos_dtc_state *)pstInputData);
             break;
         case CALLMANAGER_SERVICE_ID_XCALL_RUNNING_STATUS:
             inputData = (const char*)pstInputData;
             nsw_Xcall_subscibeXcallStatus(inputData[0], inputData[1], inputData[2]);
             break;
         default:
             NEU_LOG_ERROR("Unsupport Service Id=%d\n",ServiceID);
             Ret = MIP_RETURN_CODE_OPERATION_ID_UNSUPPORT;
             break;
     }
 
     return Ret;
 }
 
 Type_sWord wswCall_mip_StreamCallback(NEU_MODULE_ID SrcMid,Type_sWord ServiceID,void *pubRecvStreamData,Type_sWord DataLen)
 {
     NEU_LOG_CRIT( "SrcMid:[%04x] ServiceID:[%x] DataLen=%d\n", SrcMid, ServiceID,DataLen);
 #if 0
     switch(ServiceID)
     {
         case CALLMANAGER_SERVICE_ID_XCALL_IPCP_STREAM_SERVER:
         {
             E_ASN_OPERATION_ID opID;
             E_ASN_OPERATION_TYPE opType;
             Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
             if(RES_OK == nswCall_mip_IPCP_UnwrapPackage(pubRecvStreamData, aubStream, &opID, &opType)){
                 switch(opID){
                     case IHUStatusReport_IPCP: nswCall_mip_IHUStatusReport((void*)aubStream); break;
                     case XCallControl_IPCP: nswCall_mip_XCallControl((void*)aubStream); break;
                     case PremiumAudio_IPCP: wswCall_mip_PremiumAudio((void*)aubStream); break;
                     case CrashInfo_IPCP: wswCall_mip_CrashInfo((void*)aubStream); break;
                     default: 
                         NEU_LOG_CRIT("Unsupport IPCP server = %x\n", opID);
                     break;
                 }
             }
         }
         break;
         default:
             NEU_LOG_CRIT("Unsupport Service Id=%d\n",ServiceID);
             break;
     }
 #else
     switch(SrcMid)
     {
         case MID_SOC_IPCP_APP:
             wswCall_mip_IpcpStreamCallback(ServiceID, pubRecvStreamData, DataLen);
             break;
         case MID_SOC_RPC_SERVICE:
             wvdRpc_ReceiveMcuMessageComplete(MID_SOC_XCALL_APP, pubRecvStreamData, DataLen);
             break;
         case MID_SOC_NETPROT_APP:
             wswCall_tsp_StreamCallback(ServiceID, pubRecvStreamData, DataLen);
             break;
         case MID_SOC_IP_LINK_APP:
             nswCall_mip_IpLinkStreamCallback(ServiceID, pubRecvStreamData, DataLen);
             break;
         default:
             NEU_LOG_CRIT("Unsupport SrcMid:[%04x]  ServiceId=%d\n", SrcMid, ServiceID);
             break;
     }
 #endif
 
     return RES_OK;
 }
 
 //,IPLink_RG_Priority_st astRequestPrio,Wake_Up_Type wakeUpType)
 Type_sWord wswCall_mip_IPLinkRequest(Type_uWord astRequestRG)
 {
     Type_sWord nswRet = RES_FAIL;
     st_iplm_operation operation;
     if((E_XCALL_TYPE_MANUAL_ECALL_CN == glxcall_status.Calltype) ||
         (E_XCALL_TYPE_AUTO_ECALL_CN == glxcall_status.Calltype))
     {
         operation.auwLSCId = IPLM_LSC_XcallEmergencyAssist;
     }
     else if(E_XCALL_TYPE_BCALL == glxcall_status.Calltype)
     {
         operation.auwLSCId = IPLM_LSC_XcallRoadsideAssistance;
     }
     else if(E_XCALL_TYPE_ICALL == glxcall_status.Calltype)
     {
         operation.auwLSCId = IPLM_LSC_XcallConciergeServices;
     }
 
     operation.auwOperationType = IPLM_OPERATION_TYPE_REQUEST;
     operation.auwRequestRG = astRequestRG;
     nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_IP_LINK_APP, IPLINKMANAGER_SERVICE_ID_IPLINK_OPERATION, (void *)&operation, NULL, 1000);
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("Regist resourceGroup:[%d] fail\n", astRequestRG);
     }
 
     return nswRet;
 }
 
 //,IPLink_RG_Priority_st astRequestPrio,Wake_Up_Type wakeUpType)
 Type_sWord wswCall_mip_IPLinkRelease(Type_uWord astRequestRG)
 {
     Type_sWord nswRet = RES_FAIL;
     st_iplm_operation operation;
     if((E_XCALL_TYPE_MANUAL_ECALL_CN == glxcall_status.Calltype) ||
         (E_XCALL_TYPE_AUTO_ECALL_CN == glxcall_status.Calltype))
     {
         operation.auwLSCId = IPLM_LSC_XcallEmergencyAssist;
     }
     else if(E_XCALL_TYPE_BCALL == glxcall_status.Calltype)
     {
         operation.auwLSCId = IPLM_LSC_XcallRoadsideAssistance;
     }
     else if(E_XCALL_TYPE_ICALL == glxcall_status.Calltype)
     {
         operation.auwLSCId = IPLM_LSC_XcallConciergeServices;
     }
     operation.auwOperationType = IPLM_OPERATION_TYPE_RELEASE;
     operation.auwRequestRG = astRequestRG;
     nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_IP_LINK_APP, IPLINKMANAGER_SERVICE_ID_IPLINK_OPERATION, (void *)&operation, NULL, 1000);
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("Release resourceGroup:[%d] fail\n", astRequestRG);
     }
 
     return nswRet;
 }
 
 Type_sWord wswCall_mip_IPLinkFunc(Type_uWord astRequestRG, Type_uWord auwOperationType)
 {
     static Type_uWord orgOperationType = IPLM_OPERATION_TYPE_RELEASE;
     static Type_uWord orgRequestRG = XCALL_IPLM_REQUEST_RG_1;
     Type_sWord nswRet = RES_FAIL;
 
     if(auwOperationType == orgOperationType && astRequestRG == orgRequestRG){
         NEU_LOG_CRIT("IPLink already done.\n");
     }else{
         orgOperationType = auwOperationType;
         orgRequestRG = astRequestRG;
         if(IPLM_OPERATION_TYPE_REQUEST == auwOperationType){
             NEU_LOG_CRIT("IPLink group[%d] release to request.\n", astRequestRG);
             wswCall_mip_IPLinkRequest(astRequestRG);
         }else{
             NEU_LOG_CRIT("IPLink group[%d] request to release.\n", astRequestRG);
             wswCall_mip_IPLinkRelease(astRequestRG);
         }
     }
 
     return nswRet;
 }
 
 Type_sWord nswCallSendToLinkManage(Type_uWord ServiceID,void *Payload, Type_uWord DataLen)
 {
     Type_sWord nswRet = RES_OK;
     //NEU_LOG_CRIT( "wswMip_SendStream ServiceID:[%x],DataLen[%d]\n", ServiceID,DataLen);
     //log
 
     /*nswRet = wswMip_SendStream(MID_SOC_XCALL_APP,MID_SOC_IP_LINK_APP,ServiceID,Payload,DataLen);
     if(RES_OK != nswRet)
     {
         MIP_LOG_ERROR("Send IPCP ServiceID(%d) Failed!",ServiceID);
     }
     else
     {
         NEU_LOG_CRIT("Send IPCP ServiceID(%d) Success!",ServiceID);
     }*/
     return nswRet;
 }
 
 
 Type_sWord nswCallSendToIPCP(Type_uWord ServiceID,void *Payload, Type_uWord DataLen)
 {
     Type_sWord nswRet = RES_OK;
     //NEU_LOG_CRIT( "wswMip_SendStream ServiceID:[%x],DataLen[%d]\n", ServiceID,DataLen);
     //log
 
     /*nswRet = wswMip_SendStream(MID_SOC_XCALL_APP,MID_SOC_IPCP_APP,ServiceID,Payload,DataLen);
     if(RES_OK != nswRet)
     {
         MIP_LOG_ERROR("Send IPCP ServiceID(%d) Failed!",ServiceID);
     }
     else
     {
         NEU_LOG_CRIT("Send IPCP ServiceID(%d) Success!",ServiceID);
     }*/
     return nswRet;	
 }
 
 Type_sWord wswCall_mip_GetVehReserv(Type_uWord ServiceID,void *Payload, Type_uWord DataLen)
 {
     Type_sWord nswRet = RES_OK;
     //nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP,MID_SOC_VEHICLE_STS_APP,ServiceID,pstResponseMsg,TimeOut);
     /*if(RES_OK != nswRet)
     {
         MIP_LOG_ERROR("Send IPCP ServiceID(%d) Failed!",ServiceID);
     }
     NEU_LOG_CRIT("Send IPCP ServiceID(%d) Success!",ServiceID);*/
     return nswRet;	
 }
 
 /**
  * @brief Wake up tcam
  * 
  * @param reason [in]
  * @param len [in]
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_WakeUpTcam(Type_uByte *reason, Type_sWord len)
 {
     Type_sWord nswRet = RES_OK;
     st_spv_wakeup_tbox_ntfy nstReq = {0};
     st_spv_wakeup_tbox_result nstResp = {0};
 
     nstReq.Mid = MID_SOC_XCALL_APP;
     memcpy(nstReq.Reason, reason, len);
 
     nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_SUPERVISOR, SUPERVISOR_SERVICE_ID_WAKEUP_TBOX,
         &nstReq ,&nstResp, 100);
     //e_spv_pm_ctrl_result_e_spv_pm_ctrl_result_ok is 1
     NEU_LOG_CRIT("Wake up result is %d\n", nstResp.result);
     if(e_spv_pm_ctrl_result_e_spv_pm_ctrl_result_ok == nstResp.result)
     {
         nswRet = RES_OK;
     }
     else
     {
         nswRet = RES_FAIL;
     }
 
     return nswRet;
 }
 
 /**
  * @brief 
  * 
  * @param CallType [in]XCall_CallType type
  * @param Action [in]XCall_ArbiterAction type
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_ArbiterPermissionReq(Type_uWord CallType, Type_uWord Action)
 {
 #ifdef XCALL_LOCAL_TEST
     return RES_OK;
 #endif
 
 
     NEU_LOG_CRIT("CallType:%d Action:%d\n", CallType, Action);
     Type_sWord nswRet = RES_FAIL;
     st_arbiter_start_biz_in nstStartIn;
     st_arbiter_start_biz_out nstStartOut;
     memset(&nstStartIn, 0x00, sizeof(nstStartIn));
     memset(&nstStartOut, 0x00, sizeof(nstStartOut));
 
     if(Action == E_XCALL_ARBITER_PERMISSION_ACTION_ARBITRATION)
     {
         //Set request value
         if(E_XCALL_TYPE_AUTO_ECALL_CN == CallType)
         {
             nstStartIn.eBizId = e_arbiter_biz_id_e_arbiter_biz_id_autoEcall;
         }
         else if(E_XCALL_TYPE_MANUAL_ECALL_CN == CallType)
         {
             nstStartIn.eBizId = e_arbiter_biz_id_e_arbiter_biz_id_manualEcall;
         }
         else	// need ibcall T.B.D
         {
                 NEU_LOG_CRIT("Unknow CallType:%d!\n",CallType);
             return RES_OK;
         }
         nstStartIn.u32MaxExecTime = 5*60;	// 5min
 
         //Send oncall permission request to Arbiter
         nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_ARBITER_APP,
             ARBITER_SERVICE_ID_REQ_START_BIZ, &nstStartIn, &nstStartOut, 1000);
         if(RES_OK != nswRet)
         {
             NEU_LOG_ERROR("Send request sync Error:%d!\n",nswRet);
             return RES_FAIL;
         }
 
         if(nstStartOut.stBizResult.eRes != e_arbiter_result_e_arbiter_result_OK)
         {
             // 已经获取权限
             if(nstStartOut.stBizResult.eRes == e_arbiter_result_e_arbiter_result_NG_HasbeenRunning)
             {
                 return RES_OK;
             }
             NEU_LOG_ERROR("Action:%d failed:%d!\n",Action, nstStartOut.stBizResult.eRes);
             return RES_FAIL;
         }
 
         //Cteate arbiter delay timer
         nswRet = nswNeu_call_TimerCreate(E_XCALL_TIMER_ARBITER_DELAY);
         if(RES_OK != nswRet)
         {
             NEU_LOG_ERROR("Timer Create Failed[%d]\r\n", nswRet);
         }
 
         return RES_OK;
     }
 
 
     st_arbiter_stop_biz_in nstStoptIn;
     st_arbiter_stop_biz_out nstStoptOut;
     memset(&nstStoptIn, 0x00, sizeof(nstStoptIn));
     memset(&nstStoptOut, 0x00, sizeof(nstStoptOut));
     if(Action == E_XCALL_ARBITER_PERMISSION_ACTION_CANCEL)
     {
         //Set request value
         if(E_XCALL_TYPE_AUTO_ECALL_CN == CallType)
         {
             nstStoptIn.eBizId = e_arbiter_biz_id_e_arbiter_biz_id_autoEcall;
         }
         else if(E_XCALL_TYPE_MANUAL_ECALL_CN == CallType)
         {
             nstStoptIn.eBizId = e_arbiter_biz_id_e_arbiter_biz_id_manualEcall;
         }
         else
         {
             NEU_LOG_CRIT("Unknow CallType:%d!\n",CallType);
             return RES_OK;
         }
 
         //Send oncall permission request to Arbiter
         nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_ARBITER_APP,
             ARBITER_SERVICE_ID_REQ_STOP_BIZ, &nstStoptIn, &nstStoptOut, 1000);
         if(RES_OK != nswRet)
         {
             NEU_LOG_ERROR("Send request sync Error:%d!\n",nswRet);
             return RES_FAIL;
         }
 
         if(nstStoptOut.stBizResult.eRes != e_arbiter_result_e_arbiter_result_OK)
         {
             NEU_LOG_ERROR("Action:%d failed:%d!\n",Action, nstStoptOut.stBizResult.eRes);
         }
 
         //Delete arbiter delay timer
         nswRet = nswNeu_call_TimerDelete(E_XCALL_TIMER_ARBITER_DELAY);
         if(RES_OK != nswRet)
         {
             NEU_LOG_ERROR("Timer Delete Failed[%d]\r\n", nswRet);
         }
 
         return RES_OK;
     }
 
     st_arbiter_delay_biz_in nstDelayIn;
     st_arbiter_delay_biz_out nstDelayOut;
     memset(&nstDelayIn, 0x00, sizeof(nstDelayIn));
     memset(&nstDelayOut, 0x00, sizeof(nstDelayOut));
     if(Action == E_XCALL_ARBITER_PERMISSION_ACTION_DELAY)
     {
         //Set request value
         if(E_XCALL_TYPE_AUTO_ECALL_CN == CallType)
         {
             nstDelayIn.eBizId = e_arbiter_biz_id_e_arbiter_biz_id_autoEcall;
         }
         else if(E_XCALL_TYPE_MANUAL_ECALL_CN == CallType)
         {
             nstDelayIn.eBizId = e_arbiter_biz_id_e_arbiter_biz_id_manualEcall;
         }
         else
         {
             NEU_LOG_CRIT("Unknow CallType:%d!\n",CallType);
             return RES_OK;
         }
         nstDelayIn.u32DelayTime = 5*60;	// 5min
 
         //Send oncall permission request to Arbiter
         nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_ARBITER_APP,
             ARBITER_SERVICE_ID_REQ_DELAY_BIZ, &nstDelayIn, &nstDelayOut, 1000);
         if(RES_OK != nswRet)
         {
             NEU_LOG_ERROR("Send request sync Error:%d!\n",nswRet);
             return RES_FAIL;
         }
 
         if(nstDelayOut.stBizResult.eRes != e_arbiter_result_e_arbiter_result_OK)
         {
             NEU_LOG_ERROR("Action:%d failed:%d!\n",Action, nstDelayOut.stBizResult.eRes);
             return RES_FAIL;
         }
 
         return RES_OK;
     }
 
     
     NEU_LOG_ERROR("Action :%d Error!\n",Action);
     return RES_FAIL;
 }
 
 /**
  * @brief Send Message to Ecucomm
  * 
  * @param ServiceID 
  * @param Payload 
  * @param Len 
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_EcuCommNotify(Type_sWord ServiceID, Type_uByte *Payload, Type_uWord Len)
 {
     return RES_OK;
 }
 
 /**
  * @brief Send Message to NetComm
  * 
  * @param ServiceID 
  * @param Payload 
  * @param Len 
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_NetCommNotify(Type_sWord ServiceID, Type_uByte *Payload, Type_uWord Len)
 {
     return RES_OK;
 }
 
 /**
  * @brief Mute Common function
  * 
  * @param MuteState [in]_XCall_IPCP_EcallMute state
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_MuteCommonFunc(Type_uWord MuteState)
 {
     NEU_LOG_CRIT("MuteState:%d nstXcallSource:%d nstRgStatus:%d\n", MuteState, nstXcallSource, nstRgStatus);
     Type_sWord nswRet = RES_OK;
 
     //if((IPLM_RG_STATUS_AVAILABLE == nstRgStatus) && (E_XCALL_SOURCE_STATUS_MAIN_IHU == nstXcallSource))
     {
         OpMUTECommand_Notification nstMuteCmd = {0};
         nstMuteCmd.ecallMute = MuteState;
         nswRet = wswCall_mip_MUTECommandNotify(&nstMuteCmd);
         if(RES_OK != nswRet)
         {
             NEU_LOG_ERROR("Mute command notify falied:%d\n", nswRet);
         }
     }
 
     return nswRet;
 }
 
 /**
  * @brief XcallStatus function
  * 
  * @param MuteState [in]_XCall_IPCP_EcallMute state
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_XcallStatusFunc(Type_uWord type, Type_uWord status, Type_Bool callBackMode)
 {
     Type_sWord nswRet;
     OpXCallStatus_Notification nstState = {0};
     nstState.type = type;
     nstState.status = status;
     nstState.callBackMode = callBackMode;
     nswRet = wswCall_mip_XcallStatusNotify(&nstState);
     if((E_XCALL_TYPE_AUTO_ECALL_CN == glxcall_status.Calltype) && (CallStatus_callStart == status))
     {
          nswCall_mip_XcallRunningStatusCallback(5, 2, FALSE);
     }
     nswCall_mip_XcallRunningStatusCallback(type, status, callBackMode);
     
     if((CallType_eCall==nstState.type)&&(CallStatus_callEnded==nstState.status)&&(callBackMode_CallBack==nstState.callBackMode))
     {
         OpXCallStatus_Notification nstPOPStateEnd = {0};
         nstPOPStateEnd.type = CallType_noCall;
         nstPOPStateEnd.status = CallStatus_callEnded;
         nstPOPStateEnd.callBackMode = callBackMode;
         nswRet = wswCall_mip_XcallStatusNotify(&nstPOPStateEnd);
     }
     
     return nswRet;
 }
 
 /**
  * @brief Restart TCAM
  * 
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_ReStartTCAM(VOID)
 {
     Type_sWord nswRet = RES_FAIL;
     st_spv_tbox_reset_req nstReq = {0};
 
     nstReq.Mid = MID_SOC_XCALL_APP;
     memcpy(nstReq.Reason, XCALL_CALL_RESET_REASION_BTN8_45, strlen(XCALL_CALL_RESET_REASION_BTN8_45));
     NEU_LOG_CRIT("Mid:0x%x Reason:%s\n", nstReq.Mid, nstReq.Reason);
 
     nswRet = wswMip_SendRequest_Async(MID_SOC_XCALL_APP, MID_SOC_SUPERVISOR,
         SUPERVISOR_SERVICE_ID_TBOX_RESET, (VOID *)&nstReq);
     if(RES_OK != nswRet)
     {
         NEU_LOG_ERROR("Restart tbox falied:%d\n", nswRet);
     }
 
     return nswRet;
 }
 
 /**
  * @brief Get value from parameter
  * 
  * @param Request 
  * @param Response 
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_ParamSelectReq(st_param_select_request *Request, st_param_select_response *Response)
 {
     Type_sWord nswRet = RES_FAIL;
 
     nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_PARAMETER_SERVICE,
         PARAMAPP_SERVICE_ID_PARAM_SELECT_REQUEST, (VOID *)Request, (VOID *)Response, 1000);
     if(nswRet != RES_OK)
     {
         NEU_LOG_ERROR("Update parameter fail[%d]\n", nswRet);
     }
 
     return nswRet;
 }
 
 /**
  * @brief Update value into parameter
  * 
  * @param Request 
  * @param Response 
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_ParamUpdateReq(st_param_update_request *Request, st_param_update_response *Response)
 {
     Type_sWord nswRet = RES_FAIL;
     nswRet = wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_PARAMETER_SERVICE,
         PARAMAPP_SERVICE_ID_PARAM_UPDATE_REQUEST, (VOID *)Request, (VOID *)Response, 1000);
     if(nswRet != RES_OK)
     {
         NEU_LOG_ERROR("Update parameter fail[%d]\n", nswRet);
     }
 
     return nswRet;
 }
 
 /**
  * @brief DTC Setting function
  * 
  * @param result [in]
  * @return Type_sWord 
  */
 Type_sWord nswCall_mip_DTCSet(st_uds_dtc_test_result_in *result)
 {
     Type_sWord nswRet = RES_FAIL;
 
     NEU_LOG_CRIT("dtc_id:0x%x test_result:%d\n", result->dtc_id, result->test_result);
 
     nswRet = wswMip_SendRequest_Async(MID_SOC_XCALL_APP, MID_SOC_UDSCTL_APP,
         UDSCTRL_SERVICE_ID_UDS_DTC_TEST_RESULT_UPDATE, (VOID *)result);
     if(RES_OK != nswRet)
     {
         NEU_LOG_ERROR("Set DTC falied:%d\n", nswRet);
     }
 
     return nswRet;
 }
 
 static MIP_RETURN_CODE nswCall_mip_DialTestEcall(void)
 {
     // 国内无需求
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_DialManualEcall(void)
 {
     // 国内无需求
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_DialAutoEcall(void)
 {
     // 国内无需求
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_SetEcallNumber(st_debug_setecall_info* pstInputData)
 {
     Type_sWord nswRet;
 
     nswRet = wsw_cfg_update_date(ECALL_NUMBER, E_CBOX_CFG_TYPE_STRING, pstInputData->call_number, sizeof(pstInputData->call_number));
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("failed!\n");
         return MIP_RETURN_CODE_MODULE_SERVICE_PROVIDER_ERR;
     }
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_SetIcallNumber(st_debug_seticall_info* pstInputData)
 {
     Type_sWord nswRet;
     
     nswRet = wsw_cfg_update_date(ICALL_NUMBER, E_CBOX_CFG_TYPE_STRING, pstInputData->call_number, sizeof(pstInputData->call_number));
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("failed!\n");
         return MIP_RETURN_CODE_MODULE_SERVICE_PROVIDER_ERR;
     }
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_SetBcallNumber(st_debug_setbcall_info* pstInputData)
 {
     Type_sWord nswRet;
     
     nswRet = wsw_cfg_update_date(BCALL_NUMBER, E_CBOX_CFG_TYPE_STRING, pstInputData->call_number, sizeof(pstInputData->call_number));
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("failed!\n");
         return MIP_RETURN_CODE_MODULE_SERVICE_PROVIDER_ERR;
     }
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_SetTestEcallNumber(st_debug_settestcall_info* pstInputData)
 {
     // 国内无需求
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_SetCallbackDuraion(st_debug_set_duration* pstInputData)
 {
     Type_sWord nswRet;
     Type_sWord callback_duration;
     
     callback_duration = (Type_sWord)pstInputData->callback_duration;
     nswRet = wsw_cfg_update_date(SERVICE_STANDBY_TIME_EA, E_CBOX_CFG_TYPE_INT, &callback_duration, sizeof(callback_duration));
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("failed!\n");
         return MIP_RETURN_CODE_MODULE_SERVICE_PROVIDER_ERR;
     }
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_CallAutoAnswerTime(st_did_callAutoAnswerTime* pstInputData)
 {
     Type_sWord nswRet;
     Type_sWord callAutoAnswerTime;
     
     callAutoAnswerTime = (Type_sWord)pstInputData->callAutoAnswerTime;
     nswRet = wsw_cfg_update_date(AUTOMATIC_ANSWER_TIME, E_CBOX_CFG_TYPE_INT, &callAutoAnswerTime, sizeof(callAutoAnswerTime));
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("failed!\n");
         return MIP_RETURN_CODE_MODULE_SERVICE_PROVIDER_ERR;
     }
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_EcallAutoDialAttempts(st_did_ecallAutoDialAttempts* pstInputData)
 {
     Type_sWord nswRet;
     Type_sWord ecallAutoDialAttempts;
     
     ecallAutoDialAttempts = (Type_sWord)pstInputData->ecallAutoDialAttempts;
     nswRet = wsw_cfg_update_date(CALL_AUTO_DIAL_ATTEMPTS, E_CBOX_CFG_TYPE_INT, &ecallAutoDialAttempts, sizeof(ecallAutoDialAttempts));
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("failed!\n");
         return MIP_RETURN_CODE_MODULE_SERVICE_PROVIDER_ERR;
     }
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_EcallCCFT(st_did_ecallCCFT* pstInputData)
 {
     Type_sWord nswRet;
     Type_sWord ecallCCFT;
     
     ecallCCFT = (Type_sWord)pstInputData->ecallCCFT;
     nswRet = wsw_cfg_update_date(SERVICE_STANDBY_TIME_EA, E_CBOX_CFG_TYPE_INT, &ecallCCFT, sizeof(ecallCCFT));		// callback时间
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("failed!\n");
         return MIP_RETURN_CODE_MODULE_SERVICE_PROVIDER_ERR;
     }
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_DialDuration(st_did_ecallDialDuration* pstInputData)
 {
     // 不明
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_SMS_FallbackNumber(st_did_ecallSMSfallbackNum* pstInputData)
 {
     Type_sWord nswRet;
     
     nswRet = wsw_cfg_update_date(ECALL_NUMBER_B, E_CBOX_CFG_TYPE_STRING, pstInputData->ecallSMSfallbackNum, sizeof(pstInputData->ecallSMSfallbackNum));	// ecall备用号码
     if(nswRet != RES_OK)
     {
         NEU_LOG_CRIT("failed!\n");
         return MIP_RETURN_CODE_MODULE_SERVICE_PROVIDER_ERR;
     }
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_EcallTestNumber(st_did_ecallTestNum* pstInputData)
 {
     // 国内无需求
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_EcallDialAttempts(st_did_ecallTestDialAttempts* pstInputData)
 {
     // 国内无需求
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_TestModeEndDistance(st_did_testModeEndDistance* pstInputData)
 {
     // 不明
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_Set_DID_RegistrationPeriod(st_did_regiPeriod* pstInputData)
 {
     // 不明
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 static MIP_RETURN_CODE nswCall_mip_GetCallNumber(st_mip_xcall_info *pstCallInfo)
 {
     // 国内无需求
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 
 
 //=============IPCP=================
 
 
 /**  Common IPCP Encode Function.
  *   @param  pubDataToEncode        [in]   -- Point to the data to be encoded.
  *   @param  eFuncID                [in]   -- The Notification funcID.
  *   @param  pEncodedData           [out]  -- Point to Encoded data.
  *   @param  puwDataLen             [in/out]  The size of data.
  *   @return  RES_FAIL/RES_OK.
  */
 static Type_sWord nswCall_mip_IPCP_Encode(Type_uByte *pubDataToEncode, const E_ASN_OPERATION_ID eFuncID, E_ASN_OPERATION_TYPE operationType,
                                             void *pEncodedData, Type_uWord *puwEncodedDataLen)
 {
     Type_sWord aswRst = RES_FAIL;
     OSCTXT stOSCTXT;
 
     NEU_LOG_CRIT("eFuncID = %d\n", eFuncID);
 
     if (NULL == pubDataToEncode || NULL == pEncodedData || NULL == puwEncodedDataLen)
     {
         NEU_LOG_CRIT("Parameter error\n");
         return aswRst;
     }
 
     aswRst = rtInitContext(&stOSCTXT);
     if (RES_OK == aswRst)
     {
         rtxSetDiag(&stOSCTXT, TRUE);
         pu_setBuffer(&stOSCTXT, (OSOCTET*)pEncodedData, (size_t)(*puwEncodedDataLen), FALSE);
         pu_setTrace(&stOSCTXT, TRUE);
 
         switch(eFuncID){
             case PremiumAudio_IPCP: aswRst = asn1PE_OpGeneric_Request_(&stOSCTXT, (void*)pubDataToEncode); break;
             case XCallStatus_IPCP: 
                 if(operationType == ASN_Notification)
                 {
                     aswRst = asn1PE_OpXCallStatus_Notification(&stOSCTXT, (void*)pubDataToEncode);
                 }
                 else
                 {
                     aswRst = asn1PE_OpXCallStatus_Response(&stOSCTXT, (void*)pubDataToEncode);
                 }
                 break;
             case MUTECommand_IPCP: aswRst = asn1PE_OpMUTECommand_Notification(&stOSCTXT, (void*)pubDataToEncode); break;
             case POIInfoPush_IPCP: aswRst = asn1PE_OpPOIInfoPush_Notification(&stOSCTXT, (void*)pubDataToEncode); break;
             case CallBackSts_IPCP: aswRst = asn1PE_OpCallBackSts_Response(&stOSCTXT, (void*)pubDataToEncode); break;
             case ETAInfo_IPCP: aswRst = asn1PE_OpETAInfo_Response(&stOSCTXT, (void*)pubDataToEncode); break;
 #ifdef TBOX_GEEA20_BASELINE_22R4
             case ECallStatus_IPCP: aswRst = asn1PE_OpECallStatus_Notification(&stOSCTXT, (void*)pubDataToEncode);break;
 #endif
             default: aswRst = RES_FAIL; break;
         }
 
         if(RES_OK == aswRst){
             *puwEncodedDataLen = pu_getMsgLen(&stOSCTXT);
         }else{
             rtxErrPrint(&stOSCTXT);
             OSRTErrInfo* errInfo = (OSRTErrInfo*)(stOSCTXT.errInfo.list.head->data);
             NEU_LOG_CRIT("asn1PE failed: err[%d][%s]\n", aswRst,errInfo->elemName);
         }
 
         rtFreeContext(&stOSCTXT);
     }
     else 
     {
         rtxErrPrint(&stOSCTXT);
         NEU_LOG_CRIT("rtInitContext failed.\n");
     }
 
     return aswRst;
 }
 
 /**  Common IPCP Decode Function.
  *   @param  pubData        [in]   -- Point to the buffer to be encoded.
  *   @param  puhDataLen     [in]   -- The size of buffer.
  *   @param  eFuncID        [in]   -- The Notification funcID.
  *   @param  pNoti          [out]  -- Point to Notification structure.
  *   @return  RES_FAIL/RES_OK.
  */
 static Type_sWord nswCall_mip_IPCP_Decode(Type_uByte *pubDataToDecode, const Type_uHWord uhDataToDecodeLen, 
                                                 const E_ASN_OPERATION_ID eFuncID, void *pDecodedData)
 {
     Type_sWord aswRst = RES_FAIL;
     OSCTXT stOSCTXT;
 
     NEU_LOG_CRIT("eFuncID = %d\n", eFuncID);
 
     aswRst = rtInitContext(&stOSCTXT);
     if (RES_OK == aswRst)
     {
         rtxSetDiag(&stOSCTXT, TRUE);
         pu_setBuffer(&stOSCTXT, (OSOCTET*)pubDataToDecode, uhDataToDecodeLen, FALSE);
         pu_setTrace(&stOSCTXT, TRUE);
 
         switch(eFuncID){
             case IHUStatusReport_IPCP: aswRst = asn1PD_OpIHUStatusReport_Notification(&stOSCTXT, pDecodedData); break;
             case XCallControl_IPCP: aswRst = asn1PD_OpXCallControl_Notification(&stOSCTXT, pDecodedData); break;
             case CallcenterPre_IPCP: aswRst = asn1PD_OpCallcenterPre_Notification(&stOSCTXT, pDecodedData); break;
             case PremiumAudio_IPCP: aswRst = asn1PD_OpPremiumAudio_Response(&stOSCTXT, pDecodedData); break;
             case CrashInfo_IPCP: aswRst = asn1PD_OpCrashInfo_Notification(&stOSCTXT, pDecodedData); break;
             case XCallStatus_IPCP: aswRst = asn1PD_OpGeneric_Request_(&stOSCTXT, pDecodedData); break;
             case CallBackSts_IPCP: aswRst = asn1PD_OpGeneric_Request_(&stOSCTXT, pDecodedData); break;
             case ETAInfo_IPCP: aswRst = asn1PD_OpGeneric_Request_(&stOSCTXT, pDecodedData); break;
             default: aswRst = RES_FAIL; break;
         }
 
         if(RES_OK != aswRst){
             NEU_LOG_CRIT("asn1PD failed.\n");
         }
         rtFreeContext(&stOSCTXT);
     }
     else 
     {
         rtxErrPrint(&stOSCTXT);
         NEU_LOG_CRIT("rtInitContext failed.\n");
     }
 
     return aswRst;
 }
 
 static Type_uWord nuwCall_mip_IPCP_WrapPackage(Type_uByte* pubBuffer, void* pPayload, E_ASN_OPERATION_ID operationId, E_ASN_OPERATION_TYPE operationType)
 {
     Type_uWord payLoadSize = 0;
     Type_uWord bufLen = 0;
 
     if(pubBuffer == NULL)goto xcall_out;
     
     //set dest ECU_NAME
     pubBuffer[bufLen++] = 1; //Enum_IPCP_ECU_NAME_IHU;
 
     //set ipcpheader
     GEEAPduHeader* pstHead = (GEEAPduHeader*)(pubBuffer + bufLen);
     memset(pstHead, 0, sizeof(GEEAPduHeader));
     pstHead->serviceId = XCALL_ASN_SERVICE_ID;
     pstHead->operationId = operationId;
     pstHead->msgLength = 0;
     //pstHead->senderHandle;
     pstHead->protocolVersion = XCALL_ASN_SERVICE_VERSION;
     pstHead->operationType = operationType;
     //pstHead->dataType ;
     //pstHead->proc;
     bufLen += sizeof(GEEAPduHeader);
 
     // no need to encode
     if(pPayload == NULL)
     {
         pstHead->dataType= 0x01;
         pstHead->msgLength += 8;				// msgLength后面成员的大小
         return bufLen;
     }
     
     //asn1 encode
     payLoadSize = XCALL_MIP_PAYLOAD_SIZE - bufLen;
     if(RES_OK != nswCall_mip_IPCP_Encode(pPayload, operationId, operationType, pubBuffer + bufLen, &payLoadSize)){
         memset(pubBuffer, 0, bufLen);
         bufLen = 0;
         goto xcall_out;
     }
 
     pstHead->msgLength += (payLoadSize + 8);	// msgLength后面成员的大小
     bufLen += payLoadSize;
     
 xcall_out:
     return bufLen;
 }
 
 static Type_sWord nswCall_mip_IPCP_UnwrapPackage(Type_uByte* pubBuffer, void* pPayload, E_ASN_OPERATION_ID* operationId, E_ASN_OPERATION_TYPE* operationType)
 {
     Type_sWord aswRet = RES_FAIL;
     if(pubBuffer == NULL || pPayload == NULL)goto xcall_out;
 
     GEEAPduHeader* pstHead = (GEEAPduHeader*)(pubBuffer + sizeof(ST_IPCP_MIP_STREAM_HEADER_INFO)); 
 
     aswRet = nswCall_mip_IPCP_Decode(pubBuffer + sizeof(GEEAPduHeader) + sizeof(ST_IPCP_MIP_STREAM_HEADER_INFO), pstHead->msgLength, pstHead->operationId, pPayload);
 
     if(operationId)*operationId = pstHead->operationId;
     if(operationType)*operationType = pstHead->operationType;
 xcall_out:
     return aswRet;
 }
 
 Type_sWord nswCall_mip_XcallRunningStatusCallback(Type_uWord type, Type_uWord status, Type_Bool callBackMode)
 {
     Type_sWord nswRet = RES_OK;
 
     if(TRUE == callBackMode)
     {
         callBackMode = 2;
     }
     else
     {
         callBackMode = 1;
     }
 
     Type_sWord idx =0;
     static uint8_t payload[3] = {0};
     for(idx = 0; idx < XCALL_COUNT_MAX; idx++)
     {
         if(0 != nstXcallStatusInfo_a[idx].mipEventID)
         {
             payload[0] = (uint8_t)type;
             payload[1] = (uint8_t)status;
             payload[2] = (uint8_t)callBackMode;
 
             NEU_LOG_CRIT("[%d]-[%d]-[%d]\n", payload[0], payload[1], payload[2]);
 
             Type_sWord nswRet = wswMip_SendStream(MID_SOC_XCALL_APP, nstXcallStatusInfo_a[idx].mipModuleID, nstXcallStatusInfo_a[idx].mipServiceID, payload, 3);
             if(RES_OK != nswRet)
             {
                 NEU_LOG_ERROR("wswMip_SendStream error!")
             }
         }
     }
     return nswRet;
 }
 
 
 Type_sWord wswCall_mip_PremiumAudioRequest()
 {
     // 获取主音源状态
     //  OperationID: PremiumAudio(0x0009)
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, NULL, PremiumAudio_IPCP, ASN_Request);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 }
 
 Type_sWord wswCall_mip_XcallStatusNotify(OpXCallStatus_Notification* astXCallStatus)
 {
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
 
     NEU_LOG_CRIT("type: %d, status: %d, callBackMode: %d\n", astXCallStatus->type, astXCallStatus->status, astXCallStatus->callBackMode);
     memcpy(&gXCallStatus, astXCallStatus, sizeof(gXCallStatus));
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, astXCallStatus, XCallStatus_IPCP, ASN_Notification);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 }
 
 Type_sWord wswCall_mip_XcallStatusResponse()
 {
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
 
     OpXCallStatus_Response astXCallStatus;
     astXCallStatus.type = CallType_noCall;
     astXCallStatus.status = CallStatus_callEnded;
     astXCallStatus.callBackMode = callBackMode_Normal;
 
     NEU_LOG_CRIT("type: %d, status: %d, callBackMode: %d\n", astXCallStatus.type, astXCallStatus.status, astXCallStatus.callBackMode);
     
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, &astXCallStatus, XCallStatus_IPCP, ASN_Response);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 }
 
 static Type_sWord wswCall_mip_XcallStatusRequest()
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_XCALL_STATUS_REQ;
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return RES_OK;
 }
 
 Type_sWord wswCall_mip_MUTECommandNotify(OpMUTECommand_Notification* astMuteCmd)
 {
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, astMuteCmd, MUTECommand_IPCP, ASN_Notification);
     NEU_LOG_CRIT("MUTECommand: %d\n", astMuteCmd->ecallMute);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 }
 
 Type_sWord wswCall_mip_POIInfoPushNotify(OpPOIInfoPush_Notification* astPOI)
 {
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, astPOI, POIInfoPush_IPCP, ASN_Notification);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 }
 
 Type_sWord wswCall_mip_CallBackStsNotify(OpCallBackSts_Response* astRemain)
 {
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, astRemain, CallBackSts_IPCP, ASN_Notification_Cyclic);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 }
 
 Type_sWord wswCall_mip_CallBackStsResponse()
 {
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
     Type_uWord remain = 0;
     OpCallBackSts_Response astRemain;
 
     if(E_XCALL_TYPE_AUTO_ECALL_CN == glxcall_status.Calltype)
     {
            remain = wswNeuTimer_GetRest(E_XCALL_TIMER_AUTO_ECALL_CALLBACK);	
         astRemain.remainTime = remain / 100;
     }
     else if(E_XCALL_TYPE_MANUAL_ECALL_CN == glxcall_status.Calltype)
     {
         remain = wswNeuTimer_GetRest(E_XCALL_TIMER_ECALL_CALLBACK); 
         astRemain.remainTime = remain / 100;
     }
     else if(E_XCALL_TYPE_BCALL == glxcall_status.Calltype)
     {
            remain = wswNeuTimer_GetRest(E_XCALL_TIMER_BCALL_CALLBACK);	
         astRemain.remainTime = remain / 100;
     }
     else
     {
         astRemain.remainTime =0;
     }
 
     NEU_LOG_CRIT("remainTime: %d\n", astRemain.remainTime);
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, &astRemain, CallBackSts_IPCP, ASN_Response);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 }
 
 static Type_sWord wswCall_mip_CallBackStsRequest()
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_CALLBACK_STS_REQ;
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return RES_OK;
 }
 
 Type_sWord wswCall_mip_ETAInfoNotify(OpETAInfo_Response* astETAInfo)
 {	
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, astETAInfo, ETAInfo_IPCP, ASN_Notification);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 }
 
 Type_sWord wswCall_mip_ETAInfoResponse()
 {
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
     OpETAInfo_Response astETAInfo;
     TimeStamp tm;
     nvdCall_tsp_getTimeStamp(&tm);
     Type_uWord sec = gtm_eta - tm.seconds;
     memset(&astETAInfo, 0, sizeof(astETAInfo));
     if(gtm_eta > tm.seconds)
     {
         astETAInfo.eTAInfo.hour = sec/3600;
         sec %= 3600; 
         astETAInfo.eTAInfo.minute = sec/60;
         astETAInfo.eTAInfo.second = sec%60;
         //wswCall_mip_ETAInfoNotify(&astETAInfo);
         NEU_LOG_CRIT("ETA = %d-%d-%d", astETAInfo.eTAInfo.hour, astETAInfo.eTAInfo.minute, astETAInfo.eTAInfo.second);
         auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, &astETAInfo, ETAInfo_IPCP, ASN_Response);
         return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
     }
     else
     {
         NEU_LOG_CRIT("ETA sec= %d <= tm=%d, NG!!!", gtm_eta, tm.seconds);
     }
     return RES_OK;
 }
 
 static Type_sWord wswCall_mip_ETAInfoRequest()
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_ETA_INFO_REQ;
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return RES_OK;
 }
 
 Type_sWord wswCall_mip_ECallStatusNotify(Type_uWord uwECallType)
 {	
 #ifndef TBOX_GEEA20_BASELINE_22R4
     NEU_LOG_CRIT("22r2 without ecalltype");
     return RES_OK;
 #else
     Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
     Type_uWord auwLength = 0;
     OpECallStatus_Response astECallStatus;
     astECallStatus.eCallType = uwECallType;
     NEU_LOG_CRIT("ECalltype: %d", astECallStatus.eCallType);
     auwLength = nuwCall_mip_IPCP_WrapPackage(aubStream, &astECallStatus, ECallStatus_IPCP, ASN_Notification);
     return wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IPCP_APP, IPCPCTRL_SERVICE_ID_UPLOAD_IPCP_DATA_FOR_XCALL_00A9, aubStream, auwLength);
 #endif
 }
 
 static Type_sWord nswCall_mip_IHUStatusReport(OpIHUStatusReport_Notification* pstStatus)
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_IHU_STATUS_REPORT_NOTIFY;
     pData->dataLen = sizeof(OpIHUStatusReport_Notification);
     memcpy(pData->data, pstStatus, sizeof(OpIHUStatusReport_Notification));
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return RES_OK;
 }
 
 static Type_sWord nswCall_mip_XCallControl(OpXCallControl_Notification* pstXCallCtrl)
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_XCALL_CONTRL_NOTIFY;
     pData->dataLen = sizeof(OpXCallControl_Notification);
     memcpy(pData->data, pstXCallCtrl, sizeof(OpXCallControl_Notification));
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return RES_OK;
 }
 
 static Type_sWord wswCall_mip_PremiumAudio(OpPremiumAudio_Response* pstPreAudio)
 {
     ThreadQueueData data;
     memset(&data, 0, sizeof(ThreadQueueData));
     data.funcID = E_XCALL_PROC_IPCP_PREMIUM_AUDIO;
     data.dataLen = sizeof(AudioStatus);
     memcpy(data.data, &pstPreAudio->premiumAudioStatus, data.dataLen);
     return nswNeu_call_PushQueue(&stCallQue, &data);
 }
 
 static Type_sWord wswCall_mip_CrashInfo(OpCrashInfo_Notification* pstCrashInfo)
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     NEU_LOG_CRIT("received CrashInfo");
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_PREMIUM_CRASHINFO_NOTIFY;
     pData->dataLen = sizeof(OpCrashInfo_Notification);
     memcpy(pData->data, pstCrashInfo, sizeof(OpCrashInfo_Notification));
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return RES_OK;
 }
 
 static Type_sWord wswCall_mip_CallcenterPre(OpCallcenterPre_Notification* pstCallcenterPre)
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     NEU_LOG_CRIT("received CallcenterPre");
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_CallcenterPre_NOTIFY;
     pData->dataLen = sizeof(OpCallcenterPre_Notification);
     memcpy(pData->data, pstCallcenterPre, sizeof(OpCallcenterPre_Notification));
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return RES_OK;
 
  
 }
 
 static Type_sWord wswCall_mip_IpcpStreamCallback(Type_sWord ServiceID,void *pubRecvStreamData,Type_sWord DataLen)
 {
     NEU_LOG_CRIT("ServiceID:%d DataLen:%d\n", ServiceID, DataLen);
     switch(ServiceID)
     {
         case CALLMANAGER_SERVICE_ID_XCALL_IPCP_STREAM_SERVER:
         {
             E_ASN_OPERATION_ID opID;
             E_ASN_OPERATION_TYPE opType;
             Type_uByte aubStream[XCALL_MIP_PAYLOAD_SIZE] = {0};
             if(RES_OK == nswCall_mip_IPCP_UnwrapPackage(pubRecvStreamData, aubStream, &opID, &opType))
             {
                 NEU_LOG_CRIT("opID = 0x%04x, opType= 0x%x\n", opID, opType);
                 if(opType == ASN_Request) 
                 {
                     switch(opID){
                         case XCallStatus_IPCP: wswCall_mip_XcallStatusRequest(); break;
                         case CallBackSts_IPCP: wswCall_mip_CallBackStsRequest(); break;
                         case ETAInfo_IPCP: wswCall_mip_ETAInfoRequest(); break;
                         default: 
                             NEU_LOG_CRIT("Unsupport Request = %x\n", opID);
                         break;
                     }
                 }
                 else if((opType == ASN_Response) || (opType == ASN_Notification)) 
                 {
                     switch(opID){
                         case IHUStatusReport_IPCP: nswCall_mip_IHUStatusReport((void*)aubStream); break;
                         case XCallControl_IPCP: nswCall_mip_XCallControl((void*)aubStream); break;
                         case PremiumAudio_IPCP: wswCall_mip_PremiumAudio((void*)aubStream); break;
                         case CrashInfo_IPCP: wswCall_mip_CrashInfo((void*)aubStream); break;
                         case CallcenterPre_IPCP:  wswCall_mip_CallcenterPre((void*)aubStream); break;
                         default: 
                             NEU_LOG_CRIT("Unsupport Response/Notification = %x\n", opID);
                         break;
                     }
                 }
                 else
                 {
                     NEU_LOG_CRIT("Unsupport opID = 0x%04x, opType= 0x%x\n", opID, opType);
                 }
             }
         }
         break;
         default:
             NEU_LOG_CRIT("Unsupport Service Id=%d\n",ServiceID);
             break;
     }
 
     return RES_OK;
 }
 
 static Type_sWord nswCall_mip_IpLinkStreamCallback(Type_sWord ServiceID,void *pubRecvStreamData,Type_sWord DataLen)
 {
     NEU_LOG_CRIT("ServiceID:%d DataLen:%d\n", ServiceID, DataLen);
     Type_sWord nswRet = RES_FAIL;
     switch (ServiceID)
     {
     case CALLMANAGER_SERVICE_ID_RG_STS_CALLBACK_REGISTER:
         nswRet = wswCall_mip_IpLinkRGStsCallback(pubRecvStreamData, DataLen);
         break;
     default:
         NEU_LOG_ERROR("Unknow Service Id:%d\n", ServiceID);
         break;
     }
 
     return nswRet;
 }
 
 /**
  * @brief Register ip link rg status callback
  * 
  * @return Type_sWord 
  */
 static Type_sWord wswCall_mip_IpLinkRGStsCallbackRegister(VOID)
 {
     Type_sWord nswRet = RES_FAIL;
     Type_uByte aaubRegInfo[4] = {0};
     aaubRegInfo[0] = XCALL_IPLM_REQUEST_RG_1;
     aaubRegInfo[1] = MID_SOC_XCALL_APP;
     aaubRegInfo[2] = CALLMANAGER_SERVICE_ID_RG_STS_CALLBACK_REGISTER;
 
     nswRet = wswMip_SendStream(MID_SOC_XCALL_APP, MID_SOC_IP_LINK_APP,
         IPLINKMANAGER_SERVICE_ID_IPLINK_REGISTER_RG_UPDATE_CALLBACK, (void *)aaubRegInfo, 4);
     if(RES_OK != nswRet)
     {
         NEU_LOG_ERROR("Register RG status callback failed[%d]\n", nswRet);
     }
 
     return nswRet;
 }
 
 /**
  * @brief IP link callback function
  * 
  * @param pubRecvStreamData [in]
  * @param DataLen [in]
  * @return Type_sWord 
  */
 static Type_sWord wswCall_mip_IpLinkRGStsCallback(void *pubRecvStreamData,Type_sWord DataLen)
 {
     Type_sWord nswRet = RES_FAIL;
     Type_uByte nubRGStatus[2] = {0};
 
     if((NULL == pubRecvStreamData) || (2 != DataLen))
     {
         NEU_LOG_ERROR("RG status parameter error with pubRecvStreamData:%p DataLen:%d\n", pubRecvStreamData, DataLen);
         return RES_ERR_PARAMETER;
     }
     memcpy(nubRGStatus, pubRecvStreamData, DataLen);
     NEU_LOG_CRIT("nstRgStatus:%d, nubRGStatus[0]:%d, nubRGStatus[1]:%d\n", nstRgStatus, nubRGStatus[0], nubRGStatus[1]);
 
     if(XCALL_IPLM_REQUEST_RG_1 == nubRGStatus[0])
     {
         if(nstRgStatus != nubRGStatus[1])
         {
             ST_Call_XcallSosState nstSosState = {0};
             if(IPLM_RG_STATUS_AVAILABLE == nubRGStatus[1])
             {
                 nstSosState.StEthernet = E_XCALL_CALL_SOS_STATUS_VALID;
             }
             else
             {
                 nstSosState.StEthernet = E_XCALL_CALL_SOS_STATUS_INVALID;
             }
             nvdXcall_app_SetSosState(E_XCALL_SOS_LIM_SOURCE_ETHERNET, nstSosState);
         }
         nstRgStatus = nubRGStatus[1];
         nswRet = RES_OK;
     }
 
     return nswRet;
 }
 
 static MIP_RETURN_CODE nswCall_mip_rcv_SOS_dtc_state(st_sos_dtc_state* pstInputData)
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_RCV_SOS_DTC_NOTI;
     pData->dataLen = sizeof(st_sos_dtc_state);
     memcpy(pData->data, pstInputData, sizeof(st_sos_dtc_state));
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 VOID wvdCall_mip_CheckRgAvailable(Type_Bool* pResult)
 {
     st_iplm_rg_info astRgReq = {.auwRGId = 1};
     st_iplm_rg_available_info astRgResp = {.auwRGAvaiInfo = 0};	// 0:fully-available,1:partly-available,2:unavailable
 
      if (RES_OK == wswMip_SendRequest_Sync(MID_SOC_XCALL_APP, MID_SOC_IP_LINK_APP, IPLINKMANAGER_SERVICE_ID_IPLINK_GET_RG_AVAILABLE,
                                           &astRgReq, &astRgResp, 1000))
     {
         NEU_LOG_CRIT("RG RES_OK\n");
     }
     //callback期间可能为1,故修改
     *pResult = (2 != astRgResp.auwRGAvaiInfo) ? TRUE : FALSE;
     NEU_LOG_CRIT("RG1 state = %d\n", *pResult);									  
 }
 
 static MIP_RETURN_CODE nswCall_mip_network_state_noti(st_bc_nw_conn_status* pstInputData)
 {
     ST_NEU_QUEUE_DATA queueData;
     bzero(&queueData, sizeof(ThreadQueueData));
     
     ThreadQueueData* pData = (ThreadQueueData*)queueData.ubData;
     pData->srcThreadID = wswNue_call_getThreadID();
     pData->dstThreadID = E_XCALL_TASK_CALL_THREAD;
     pData->funcID = E_XCALL_PROC_NETWORK_STATE_NOTI;
     pData->dataLen = sizeof(st_sos_dtc_state);
     memcpy(pData->data, pstInputData, sizeof(st_sos_dtc_state));
     
     queueData.uwDataLen = sizeof(ThreadQueueData);
     wswNeuQueue_Push(&stCallQue, &queueData);
 
     return MIP_RETURN_CODE_SUCCESS;
 }
 
 Type_sWord nsw_Xcall_subscibeXcallStatus(uint8_t aswMipEventID, uint8_t aswMipModuleID, uint8_t aswMipServiceID)
 {
     NEU_LOG_CRIT( "IN[%02X][%02X][%02X]\n", aswMipModuleID, aswMipServiceID, aswMipServiceID)
     if(aswMipEventID < 1 || aswMipModuleID >= MID_SOC_END)
     {
         return RES_FAIL;
     }
 
     Type_sWord aswRet = RES_FAIL;
 
     //save callbackId & callbackHandle to local array
     int idx = 0;
     for(idx = 0; idx < XCALL_COUNT_MAX; idx++)
     {
         if(0 == nstXcallStatusInfo_a[idx].mipEventID)
         {
             nstXcallStatusInfo_a[idx].mipEventID   = aswMipEventID;
             nstXcallStatusInfo_a[idx].mipModuleID  = aswMipModuleID;
             nstXcallStatusInfo_a[idx].mipServiceID = aswMipServiceID;
             aswRet = RES_OK;
             break;
         }
     }
 
     if(XCALL_COUNT_MAX == idx)
     {
         NEU_LOG_ERROR( "call me too much times!!\n")
     }
 
     NEU_LOG_CRIT("OUT");
     return aswRet;
 }

 Type_sWord nsw_Xcall_subscibeXcallStatussssss(uint8_t aswMipEventID, uint8_t aswMipModuleID, uint8_t aswMipServiceID)
 {
     NEU_LOG_CRIT( "IN[%02X][%02X][%02X]\n", aswMipModuleID, aswMipServiceID, aswMipServiceID)
     if(aswMipEventID < 1 || aswMipModuleID >= MID_SOC_END)
     {
         return RES_FAIL;
     }
 
     Type_sWord aswRet = RES_FAIL;
 
     //save callbackId & callbackHandle to local array
     int idx = 0;
     for(idx = 0; idx < XCALL_COUNT_MAX; idx++)
     {
         if(0 == nstXcallStatusInfo_a[idx].mipEventID)
         {
             nstXcallStatusInfo_a[idx].mipEventID   = aswMipEventID;
             nstXcallStatusInfo_a[idx].mipModuleID  = aswMipModuleID;
             nstXcallStatusInfo_a[idx].mipServiceID = aswMipServiceID;
             aswRet = RES_OK;
             break;
         }
     }
 
     if(XCALL_COUNT_MAX == idx)
     {
         NEU_LOG_ERROR( "call me too much times!!\n")
     }
 
     NEU_LOG_CRIT("OUT");
     return aswRet;
 }