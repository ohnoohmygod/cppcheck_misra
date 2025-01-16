/********************************************************************************************/
/* CopyRight (C) 2010 Neusoft Corporation                                                   */
/* Made by EasyCode2010 ver0.1 for AVNC&IS Division (2010-1-13 11:40)                       */
/********************************************************************************************/
/*  File Name       : CallApp_Core.c                                                        */
/*  Model Name      : Geely4G                                                               */
/*  Module Name     : Call App                                                              */
/*  uCom            : Renesas RH850                                                         */
/*                                                                                          */
/*  Pre-Include File: -                                                                     */
/*  Create Date     : 2017-9-30                                                             */
/*  Author/Corp     :                                                                       */
/*                                                                                          */
/*  Description     : .                                                                     */
/*                                                                                          */
/*------------------------------Revision History--------------------------------------------*/
/*  No      Version     Date        Revised By      Item            Description             */
/*  01       001      9/30/17       ZC              ALL             Create.                 */
/*------------------------------------------------------------------------------------------*/
/********************************************************************************************/

/********************************************************************************************/
/*----------------------------------Begin VSS ----------------------------------------------*/
/*File  information:                                                                        */
/*1.   $Archive:    $                                                                       */
/*2.   $Workfile:   $                                                                       */
/*                                                                                          */
/*Modify information:                                                                       */
/*1.   $Revision:   $                                                                       */
/*2.   $Author:     $                                                                       */
/*3.   $Modtime:    $                                                                       */
/*                                                                                          */
/*$NoKeywords:$                                                                             */
/*----------------------------------End VSS ------------------------------------------------*/
/********************************************************************************************/

/********************************************************************************************/
/*                          Include File Section                                            */
/********************************************************************************************/
#include "apn_basic_type.h"
#include "model.h"
#include "ec_commac.h"
#include "DEV_IF.h"
#include "TimerDev_If.h"
#include "TimerDev_Ext.h"
#include "CallApp_core.h"
#include "CallApp_fn.h"
#include "CallApp_if.h"
#include "ClockDev_if.h"
#include "CallDev_if.h"
#if 0 /* Calling Drv API is not allowed, please use Dev_Hal_If.h */
#include "GpioDrv_Ext.h"
#endif
#include "RpcApp_If.h"
#include "Rte_CallApp.h"
#include "CallApp_Core.h"
#include "CanApp_If.h"
#include "CrashDev_If.h"
#include "StgApp_Setting_If.h"
#include "CallApp_AA_Service.h"
#include "DiagApp_If.h"
/********************************************************************************************/
/*                    Macro Definition Section                                        */
/********************************************************************************************/

/********************************************************************************************/
/*                    Type Definition Section                                         */
/********************************************************************************************/

/********************************************************************************************/
/*                    Enumeration Type Definition Section                              */
/********************************************************************************************/

/********************************************************************************************/
/*                    Structure/Union Type Definition Section                          */
/********************************************************************************************/

/********************************************************************************************/
/*                    Global Variable Definition Section                               */
/********************************************************************************************/

/********************************************************************************************/
/*                    Static Variable Definition Section                              */
/********************************************************************************************/
static CALL_APP_MODE_te nubCallAppMode  =   CALLAPP_MODE_UNLOAD;

#pragma ghs section sbss = ".DEEPSLEEP_RAM_DATA"

#pragma ghs section sbss = default

Type_uByte nubBtnBCall;
Type_uByte nubBtnECall;
static Type_uByte nubBtnResetFlag=CALL_APP_RESET_RELEASED;
/********************************************************************************************/
/*                    Prototype Declaration Section                                         */
/********************************************************************************************/
static void nvdCallApp_CheckMsg(CallApp_Event_Type_te eEvent, const Type_uByte *pubMsg);
/********************************************************************************************/
/** \function       wvdCallApp_SendMail
 *  \date           2017/09/30
 *  \author
 *  \description
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *
 *  <!------------- Return Code ------------------------------------------------------------->
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 *
 ********************************************************************************************/
void wvdCallApp_SendMail(Type_uHWord EventType, const void *pubData, Type_uWord uwSize)
{
    if (CALLAPP_MODE_LOAD == nubCallAppMode)
    {
        RTEStdMail_ts    astStdMail;

        astStdMail.hwEvent = EventType;
        wvdGen_MemCpy(&(astStdMail.tuEventOpt), pubData, uwSize);

        if(RTE_E_OK != Rte_Send_CallApp_Snd_Mail_Event((RTEStdMail_ts *)&astStdMail)) {
            /* send message failed, warning it here, need confirm in future */
            /* Added by smshzg 2019-04-26 */
            LOG_W(_MOUDLE_CALL_APP_, PARAMTER("Snd Mail Event failed!!!\r\n"));
        }
    }
    else
    {

    }
}
/********************************************************************************************/
/** \function       nvdCallAppCheckMsg
 *  \date           2017/09/30
 *  \author
 *  \description
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *
 *  <!------------- Return Code ------------------------------------------------------------->
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 *
 ********************************************************************************************/
static void nvdCallApp_CheckMsg(CallApp_Event_Type_te eEvent, const Type_uByte *pubMsg) {
    switch(eEvent) {
        case CALLAPP_EVT_SOC_CALL_RES:
            wvdCallAppCheckXCallRes(pubMsg, CALLAPP_MSG_LENGTH);
            break;
        case CALLAPP_EVT_BTN_DEV:
            wvdCallAppCheckBtnCB(pubMsg, CALLAPP_MSG_LENGTH);
            break;
        case CALLAPP_EVT_CRASH_DEV:
            wvdCallApp_UpdateITOSts(*pubMsg);
            break;
        case CALLAPP_EVT_LTE_WKUP_NOTICE:
            wvdCallApp_SetSocSts(pubMsg[0]);
            break;
        case CALLAPP_EVT_CTRL:
            switch ((CaLLAPP_CTRL_CMD_et)(((CallApp_CtrlData_st*)(pubMsg))->cmd)) {
                case CTRL_CMD_SET_CAN_EXT_VAR:
                    wvbCallApp_UpdateCanSignalValue((CallApp_CanSignal_st*)(((CallApp_CtrlData_st*)(pubMsg))->arg));
                    break;
                case CTRL_CMD_SET_PM_EXT_VAR:
                    wvdCallApp_UpdateCarUsgModeValue((CarUsgMode_st*)(((CallApp_CtrlData_st*)(pubMsg))->arg));
                    break;
                default:
                    break;
            }  /* end switch of CALLAPP_EVT_CTRL */
            break;
        default:
            /* Do nothing */
            break;
    }
}
/********************************************************************************************/
/*                    Prototype Declaration Section                                   */
/********************************************************************************************/

/********************************************************************************************/
/** \function       TSK_CALL_APP
 *  \date           2017/09/30
 *  \author
 *  \description
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *
 *  <!------------- Return Code ------------------------------------------------------------->
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 *
 ********************************************************************************************/
void wvdCallApp_MainRunnable(void)
{
    RTEStdMail_ts   astMsg;
    Std_ReturnType  auRcvResult = RTE_E_OK;

    do
    {
        auRcvResult = Rte_Receive_CallApp_Rcv_Mail_Event((RTEStdMail_ts *)(&astMsg));   /* message receive */
        switch (auRcvResult)                                                            /* receive result check */
        {
            case RTE_E_LOST_DATA:
            case RTE_E_OK:                                                              /* message receive OK */
                if(CALLAPP_MODE_LOAD == nubCallAppMode)
                {
                    nvdCallApp_CheckMsg((CallApp_Event_Type_te)(astMsg.hwEvent), (Type_uByte *)(&(astMsg.tuEventOpt)));
                }
                else
                {
                    /*Nothing to do*/
                }
                break;
            case RTE_E_NO_DATA:
                break;
            default:                                                                    /* etc. result */
                 break;
        }
    }while(auRcvResult != RTE_E_NO_DATA);
}



/********************************************************************************************/
/*                    Prototype Declaration Section                                   */
/********************************************************************************************/

/********************************************************************************************/
/*                    Function Definition Section                                       */
/********************************************************************************************/
/********************************************************************************************/
/** \function       wvdApICallApp_Load
 *  \date           2017/09/30
 *  \author         CBOX
 *  \description
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *
 *  <!------------- Return Code ------------------------------------------------------------->
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 *
 ********************************************************************************************/
Type_uByte wvdAPICallApp_Load(void)
{
    Type_uByte aubRet = RTE_E_OK;
    DEV_TIMER_Event_ts  astTimerCtrl;


    nubBtnBCall = BTN_RELEASED;
    nubBtnECall = BTN_RELEASED;
	nubBtnResetFlag=CALL_APP_RESET_RELEASED;

    nubCallAppMode = CALLAPP_MODE_LOAD;

    wvdCallApp_AA_Service_Init();
	wvdCallApp_VariableInit();

    Rte_Call_CanApp_SetModuleInitSts_Operation(CallAppInit);

    astTimerCtrl.hwTimeID        = TIMER_ID_NO_CALL_BTN_TIMEOUT;
    astTimerCtrl.bStartMode      = DEV_TIMER_MODE_ONESHOOT;
    astTimerCtrl.uwPrivateData   = WORD_CLEAR;
    astTimerCtrl.nuwTimerTimeout = 5*1000;
    wubAPIDevCTL_TIMER(DEV_CTL_RESET, &astTimerCtrl);
    return aubRet;
}
/********************************************************************************************/
/** \function       wvdApiCallApp_Unload
 *  \date           2018/09/29
 *  \author         cheng_yj[NEU]
 *  \description    Rpc App Callback Function
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *  \param[in]      auhCallback
 *  \param[in]      aubOptionCode
 *  \param[in]      avdOptionCode_p
    \param[in]      auhDataSize
 *  <!------------- Return Code ------------------------------------------------------------->
 *  \return         void
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 ********************************************************************************************/
Type_uByte wvdAPICallApp_Unload(void)
{
    Type_uByte aubRet = RTE_E_OK;
    wvdCallApp_AA_Service_Unload();
    Rte_Call_CallDev_Close_Operation(&aubRet);
    nubCallAppMode = CALLAPP_MODE_UNLOAD;
    return aubRet;
}
/********************************************************************************************/
/** \function       wvdAPICallApp_Control
 *  \date           2018/09/29
 *  \author         cheng_yj[NEU]
 *  \description    Rpc App Callback Function
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *  \param[in]      auhCallback
 *  \param[in]      aubOptionCode
 *  \param[in]      avdOptionCode_p
    \param[in]      auhDataSize
 *  <!------------- Return Code ------------------------------------------------------------->
 *  \return         void
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 ********************************************************************************************/
Type_uByte wvdAPICallApp_Control(Type_uByte aubEvent, const void* vpArg)
{
    Type_uByte aubRet = RTE_E_OK;
    CallApp_CtrlData_st astMsg;

    astMsg.cmd = (Type_uWord)(aubEvent);

    switch (aubEvent)
    {
        case CTRL_CMD_SET_CAN_EXT_VAR:
            {
                static CallApp_CanSignal_st ntCanArgBuf = {0xFF, 0xFF, 0xFF, 0XFF};

                if (GEN_CMP_EQUAL != wswGen_MemCmp(vpArg, &ntCanArgBuf, sizeof(CallApp_CanSignal_st)))
                {
                    wvdGen_MemCpy(&ntCanArgBuf, vpArg, sizeof(CallApp_CanSignal_st));
                    wvdGen_MemCpy(astMsg.arg, vpArg, sizeof(CallApp_CanSignal_st));
                    wvdCallApp_SendMail(CALLAPP_EVT_CTRL, &astMsg, sizeof(astMsg));
                }
            }
            break;
       case CTRL_CMD_SET_PM_EXT_VAR:
            {
                static CarUsgMode_st ntUsgArgBuf = {0xFF, 0xFF};

                if (GEN_CMP_EQUAL != wswGen_MemCmp(vpArg, &ntUsgArgBuf, sizeof(CarUsgMode_st)))
                {
                    wvdGen_MemCpy(&ntUsgArgBuf, vpArg, sizeof(CarUsgMode_st));
                    wvdGen_MemCpy(astMsg.arg, vpArg, sizeof(CarUsgMode_st));
                    wvdCallApp_SendMail(CALLAPP_EVT_CTRL, &astMsg, sizeof(astMsg));
                }
            }
            break;
       default:
            break;
    }

    return aubRet;
}

/********************************************************************************************/
/** \function       wvdAPICallApp_Query
 *  \date           2016/11/28
 *  \author
 *  \description
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *
 *  <!------------- Return Code ------------------------------------------------------------->
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 *
 ********************************************************************************************/
Type_uByte wvdAPICallApp_Query(Type_uByte ubType, Type_uByte* pubParameter)
{
    Type_uByte aubRtn = RTE_E_OK;

    switch(ubType)
    {
        case CALL_APP_TYPE_BCALL_ATTRIBUTE:
            *pubParameter = nubBtnBCall;
            break;
        case CALL_APP_TYPE_ECALL_ATTRIBUTE:
            *pubParameter = nubBtnECall;
            break;
        case CALL_APP_BUTTON_RESET_FLAG:
            *pubParameter = nubBtnResetFlag;
			break;
        default:
            break;
    }
    return aubRtn;
}

Type_uByte wvdAPICallApp_Set(Type_uByte ubType, Type_uByte pubParameter)
{
    Type_uByte aubRtn = RTE_E_OK;

    switch(ubType)
    {
        case CALL_APP_BUTTON_RESET_FLAG:
            nubBtnResetFlag = pubParameter;
            break;
        default:
            break;
    }
    return aubRtn;
}

/********************************************************************************************/
/** \function       wvdAPICallApp_CallDevCallback
 *  \date           2018/09/29
 *  \author         cheng_yj[NEU]
 *  \description    Rpc App Callback Function
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *  \param[in]      auhCallback
 *  \param[in]      aubOptionCode
 *  \param[in]      avdOptionCode_p
    \param[in]      auhDataSize
 *  <!------------- Return Code ------------------------------------------------------------->
 *  \return         void
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 ********************************************************************************************/
void wvdAPICallApp_CallDevCallback(Type_uByte aubEvent, Type_uWord auwResult)
{
    BTN_Event_Msg_ts    astMsg;
    Type_uByte aubRteRes;

    wvdGen_MemSet(&astMsg, 0, sizeof(astMsg));
    if ((TRUE == wubDiagGetFactoryMode()) || (DIAG_AGE_STATUSE_TRUE == DiagApp_IfGetAgeStatus()))
    {
        LOG_W(_MOUDLE_CALL_APP_, PARAMTER("FTM Or Age Ignore EVT[%d].\r\n"), aubEvent , wubDiagGetFactoryMode(), DiagApp_IfGetAgeStatus());
        return;
    }
    switch(aubEvent){
        case CALL_DEV_REPORT_INIT:
            LOG_E(_MOUDLE_CALL_APP_, PARAMTER("DevCBError[%d]!\r\n"),aubEvent);
            break;
        case CALL_DEV_REPORT_ECALL_PRESSED:
            astMsg.ubButton =   CallApp_Btn_EButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Pressed;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_ECALL_RELEASED:
            astMsg.ubButton =   CallApp_Btn_EButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Released;
            astMsg.uhOpt = (Type_uHWord)auwResult;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_ECALL_SHORT:
            astMsg.ubButton =   CallApp_Btn_EButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Short_Pressed;
            astMsg.uhOpt    =   (Type_uHWord)auwResult;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_ECALL_LONG:
            if (STD_FALSE == auwResult)
            {
                astMsg.ubButton =   CallApp_Btn_EButton;
                astMsg.ubEvent  =   CallApp_Btn_Event_Long_Pressed;
                wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            }
            else
            {
                Rte_Call_PmApp_SetStandbyOutFactor_Operation(&aubRteRes, COM_PM_STANDBYOUT_FACTOR_EBUTTON, COM_PM_STANDBYOUT_FACTOR_CLR);
                LOG_E(_MOUDLE_CALL_APP_, PARAMTER("Ecall stuck not release\r\n"));
            }
            break;
        case CALL_DEV_REPORT_ECALL_STUCK:
            astMsg.ubButton =   CallApp_Btn_EButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Stuck;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_ECALL_STUCK_RELEASE:
            astMsg.ubButton =   CallApp_Btn_EButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_StuckReleased;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_BCALL_PRESSED:
            astMsg.ubButton =   CallApp_Btn_BButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Pressed;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_BCALL_RELEASED:
            astMsg.ubButton =   CallApp_Btn_BButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Released;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_BCALL_SHORT:
            astMsg.ubButton =   CallApp_Btn_BButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Short_Pressed;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_BCALL_LONG:
            if (STD_FALSE == auwResult)
            {
                astMsg.ubButton =   CallApp_Btn_BButton;
                astMsg.ubEvent  =   CallApp_Btn_Event_Long_Pressed;
                wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            }
            else
            {
                Rte_Call_PmApp_SetStandbyOutFactor_Operation(&aubRteRes, COM_PM_STANDBYOUT_FACTOR_BBUTTON, COM_PM_STANDBYOUT_FACTOR_CLR);
                LOG_E(_MOUDLE_CALL_APP_, PARAMTER("Bcall stuck not release\r\n"));
            }
            break;
        case CALL_DEV_REPORT_BCALL_STUCK:
            astMsg.ubButton =   CallApp_Btn_BButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Stuck;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_BCALL_STUCK_RELEASE:
            astMsg.ubButton =   CallApp_Btn_BButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_StuckReleased;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_ECALL_NOTICE:
            astMsg.ubButton =   CallApp_Btn_EButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Notice;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_BCALL_NOTICE:
            astMsg.ubButton =   CallApp_Btn_BButton;
            astMsg.ubEvent  =   CallApp_Btn_Event_Notice;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_CONNECT:
            astMsg.ubButton =   CallApp_Btn_DevStatus;
            astMsg.ubEvent  =   CallApp_Btn_Event_sts_Connect;
            astMsg.uhOpt    = (Type_uHWord)auwResult;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_UNCONNECT:
            astMsg.ubButton =   CallApp_Btn_DevStatus;
            astMsg.ubEvent  =   CallApp_Btn_Event_sts_Unconnect;
            astMsg.uhOpt    = (Type_uHWord)auwResult;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_SHORT_GND:
            astMsg.ubButton =   CallApp_Btn_DevStatus;
            astMsg.ubEvent  =   CallApp_Btn_Event_sts_ShortToGnd;
            astMsg.uhOpt    = (Type_uHWord)auwResult;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        case CALL_DEV_REPORT_SHORT_BATT:
            astMsg.ubButton =   CallApp_Btn_DevStatus;
            astMsg.ubEvent  =   CallApp_Btn_Event_sts_ShortToBatt;
            astMsg.uhOpt    = (Type_uHWord)auwResult;
            wvdCallApp_SendMail(CALLAPP_EVT_BTN_DEV,&astMsg,sizeof(astMsg));
            break;
        default:
            LOG_E(_MOUDLE_CALL_APP_, PARAMTER("DevCBError [evt, result]=[%d, %d]!\r\n"), aubEvent, auwResult);
            break;
    }
}

/********************************************************************************************/
/** \function       wvdAPICallApp_CrashDevCallback
 *  \date           2018/09/29
 *  \author         cheng_yj[NEU]
 *  \description    Rpc App Callback Function
 *
 *  <!------------- Argument Code ----------------------------------------------------------->
 *  \param[in]      auhCallback
 *  \param[in]      aubOptionCode
 *  \param[in]      avdOptionCode_p
    \param[in]      auhDataSize
 *  <!------------- Return Code ------------------------------------------------------------->
 *  \return         void
 *
 *********************************************************************************************
 *  \par    Revision History:
 *  <!----- No.     Date        Revised by      Details ------------------------------------->
 ********************************************************************************************/
void wvdAPICallApp_CrashDevCallback(Type_uByte aubEvent, Type_uWord auwResult)
{
    Type_uByte    aubArg;

    aubArg = (Type_uByte)auwResult;
    switch(aubEvent)
    {
        case CRASH_DEV_CRASH_STS:
            wvdCallApp_SendMail(CALLAPP_EVT_CRASH_DEV, &aubArg, sizeof(aubArg));
            break;
        default:
            break;
    }
}


Type_uByte wubCallApp_LteWakeupStatus(Type_uByte ubStatus)
{
    Type_uByte  aubRtn = RES_OK;
    wvdCallApp_SendMail(CALLAPP_EVT_LTE_WKUP_NOTICE,&ubStatus,sizeof(ubStatus));
    return aubRtn;
}

void wvdCallAppTimeoutHandler(Type_uHWord hwTimeID,Type_uWord uwParam)
{
    Type_uByte aubRteRet;
    UNUSED(uwParam);
    switch(hwTimeID)
    {
        case TIMER_ID_RCV_CALL_STS_TIMEOUT:
            LOG_E(_MOUDLE_CALL_APP_, PARAMTER("TIMER_ID_RCV_CALL_STS_TIMEOUT!\r\n"));
            Rte_Call_PmApp_SetStandbyOutFactor_Operation(&aubRteRet, COM_PM_STANDBYOUT_FACTOR_EBUTTON, COM_PM_STANDBYOUT_FACTOR_CLR);
            Rte_Call_PmApp_SetStandbyOutFactor_Operation(&aubRteRet, COM_PM_STANDBYOUT_FACTOR_BBUTTON, COM_PM_STANDBYOUT_FACTOR_CLR);
            Rte_Call_PmApp_SetStandbyOutFactor_Operation(&aubRteRet, COM_PM_STANDBYOUT_FACTOR_BUTTON, COM_PM_STANDBYOUT_FACTOR_CLR);
            break;
        case TIMER_ID_AAS_ECALL_TIMEOUT:
            /* AAS crash ECALL no response, so re-dailing */
            LOG_E(_MOUDLE_CALL_APP_, PARAMTER("TIMER_ID_AAS_ECALL_TIMEOUT!\r\n"));
            wvdCallApp_BtnReqCrashDialing(STD_NULL, WORD_CLEAR);
            break;
        case TIMER_ID_NO_CALL_BTN_TIMEOUT:
            Rte_Call_PmApp_SetStandbyOutFactor_Operation(&aubRteRet, COM_PM_STANDBYOUT_FACTOR_BUTTON, COM_PM_STANDBYOUT_FACTOR_CLR);
            break;
        default:
            /* do nothing */
            break;
    }
}

