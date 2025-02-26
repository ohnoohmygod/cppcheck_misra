/**
 * @file call_audio.c
 * @author Dong Ming (dong_ming@neusoft.com)
 * @brief 
 * @version 0.1
 * @date 2021-02-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "neu_xcall.h"
#include "call_audio.h"
#include "audio_api.h"
#include "neu_at_api.h"
#include "call_data.h"

//#define NEU_RTP_ADDRESS ("172.20.30.3")
#define NEU_RTP_TBOX_PORT (53248)
#define NEU_RTP_IHU_PORT (53248)
#define NEU_RTP_IHU_FROM_ADDR    ("198.18.32.17")
#define NEU_RTP_IHU_ADDR    ("198.18.34.15")
#define NEU_QL_MANAGER_SERVER_PID_PATH ("/run/ql_manager_server.pid")

Type_sWord nswQlMSPId = 0;
Type_Bool nbTuenrRtpConfSetState = FALSE;
XCall_SourceSt nstXcallSource;

//Bussiness Function
Type_sWord nswCall_audio_RtpConfSet(VOID);
Type_sWord nswCall_audio_QueneDispatch(ST_NEU_QUEUE_DATA *stQueData);
Type_Bool nblCall_audio_QMSReadyCheck(VOID);

/**
 * @brief Initialize function
 * 
 * @return Type_sWord 
 */
Type_sWord nswCall_audio_Init(VOID)
{
    //nstXcallSource = nswCall_data_AudioSourceGet();
    nstXcallSource = E_XCALL_SOURCE_STATUS_INVALID;
    return RES_OK;
}

/**
 * @brief Exit function
 * 
 * @return VOID 
 */
VOID nvdCall_audio_Exit(VOID)
{
    return;
}

/**
 * @brief 
 * 
 * @param arg 
 * @return void* 
 */
void * wvdpThread_Call_Audio_Function(void *arg)
{
    Type_sWord aswRet = RES_FAIL;
    ST_NEU_THREAD_INFO *pstThreadInfo;
    PST_NEU_QUEUE pstQue;
    ST_NEU_QUEUE_DATA stQueData;
    char s[100] = "小火车托马斯";
    if(NULL == arg)
    {
        NEU_LOG_ERROR("arg is NULL\r\n");
        return NULL;
    }
    else
    {
        pstThreadInfo = (ST_NEU_THREAD_INFO* )arg;
    }

    if(NULL == pstThreadInfo->pstQue)
    {
        NEU_LOG_ERROR("The pstQue is NULL!\n");
        return NULL;
    }
    else
    {
        pstQue = pstThreadInfo->pstQue;
    }

    //RTP config set
#ifdef SOC_PLATFORM_QUECTEL
    aswRet = nswNeu_call_TimerCreate(E_XCALL_TIMER_QL_MANAGER_SERVER_READY_CHK);
    if(RES_OK != aswRet)
    {
        NEU_LOG_ERROR("Timer Create Failed[%d]\r\n", aswRet);
    }
#endif
    nswCall_audio_RtpConfSet();

    while(1)
    {
        memset(&stQueData, 0x00, sizeof(stQueData));
        aswRet = wswNeuQueue_Pop(pstQue, &stQueData);
        if(RES_OK == aswRet)
        {
            aswRet = nswCall_audio_QueneDispatch(&stQueData);
        }
    }

    return NULL;
}

/**
 * @brief Audio thread queue dispatch function
 * 
 * @param stQueData 
 * @return Type_sWord 
 */
Type_sWord nswCall_audio_QueneDispatch(ST_NEU_QUEUE_DATA *stQueData)
{
    Type_sWord aswRet = RES_FAIL;
    ThreadQueueData nstQueueData;
    memset(&nstQueueData, 0, sizeof(ThreadQueueData));
    if(sizeof(ThreadQueueData) != stQueData->uwDataLen)
    {
        NEU_LOG_ERROR("ThreadQueueData size:%d uwDataLen:%d\n", sizeof(ThreadQueueData), stQueData->uwDataLen);
        return RES_ERR_PARAMETER;
    }
    memcpy(&nstQueueData, stQueData->ubData, stQueData->uwDataLen);

    switch(nstQueueData.funcID)
    {
        case E_XCALL_PROC_QL_MANAGER_SERVER_READY_CHK:
        {
            NEU_LOG_CRIT("Recv E_XCALL_PROC_QL_MANAGER_SERVER_READY_CHK\r\n");
            aswRet = nswCall_audio_RtpConfSet();
            break;
        }
        case E_XCALL_PROC_QL_MANAGER_SERVER_RESTARTED_CHK:
        {
            Type_sWord nswCurrentQlMSPId = wswNeuAtGetQlManagerServerPID();
            if((0 != nswCurrentQlMSPId) && (nswCurrentQlMSPId != nswQlMSPId))
            {
                NEU_LOG_CRIT("nswQlMSPId:%d :nswCurrentQlMSPId:%d\r\n", nswQlMSPId, nswCurrentQlMSPId);
                nswQlMSPId = nswCurrentQlMSPId;
                nbTuenrRtpConfSetState = FALSE;
                aswRet = nswCall_audio_RtpConfSet();
                if(RES_OK != aswRet)
                {
                    NEU_LOG_CRIT("Reset RTP config failed, creat timer\r\n");
					aswRet = nswNeu_call_TimerCreate(E_XCALL_TIMER_QL_MANAGER_SERVER_READY_CHK);
                    if(RES_OK != aswRet)
                    {
                        NEU_LOG_ERROR("Timer Create Failed[%d]\r\n", aswRet);
                    }
                }
            }
            else
            {
                aswRet = RES_OK;
            }
            break;
        }
        default:
            NEU_LOG_ERROR("Unknow funcID:%d\r\n", nstQueueData.funcID)
            break;
    }

    return RES_OK;
}

/**
 * @brief RTP config set function
 * 
 * @return Type_sWord 
 */
Type_sWord nswCall_audio_RtpConfSet(VOID)
{
    Type_sWord aswRet = RES_FAIL;

#ifdef SOC_PLATFORM_QUECTEL
    if((!nbTuenrRtpConfSetState) && nblCall_audio_QMSReadyCheck())
#elif defined SOC_PLATFORM_LINKSCI
    if(!nbTuenrRtpConfSetState)
#endif
    {		
        nbTuenrRtpConfSetState = TRUE;
        //Set RTP config
        neu_rtp_config_t ntRtpCfg = {0};
        Type_uByte FromnubAddr[20] = {0};
        Type_uByte nubAddr[20] = {0};

        strcpy(FromnubAddr, NEU_RTP_IHU_FROM_ADDR);
        strcpy(nubAddr, NEU_RTP_IHU_ADDR);

        ntRtpCfg.auwSrcIp    = inet_addr(FromnubAddr);
        ntRtpCfg.ahSrcPort   = NEU_RTP_TBOX_PORT;
        ntRtpCfg.auwDstIp    = inet_addr(nubAddr);
        ntRtpCfg.ahDstPort   = NEU_RTP_IHU_PORT;
        ntRtpCfg.astChanType = NEU_RTP_AUDIO_CHAN_VOICE;
#ifdef SOC_PLATFORM_QUECTEL
        ntRtpCfg.pltype      = NEU_RTP_PAYLOAD_TYPE_G711A;
#elif  defined SOC_PLATFORM_LINKSCI
	    ntRtpCfg.pltype      = NEU_RTP_PAYLOAD_TYPE_PCM;
#endif
        ntRtpCfg.sample_rate = 8000;
        ntRtpCfg.chanel_num  = 1;
        ntRtpCfg.psize       = 1024;
        ntRtpCfg.socket_ttl  = 20;
        ntRtpCfg.ssrc        = 0xFFFF0000;
	strcpy(ntRtpCfg.eth_name,"eth0.4");

        aswRet = wswAudioRtpConfig(&ntRtpCfg);
        if(RES_OK != aswRet)
        {
            NEU_LOG_ERROR("Set RTP config failed[%d]!\r\n", aswRet);
            return aswRet;
        }
    }

    return aswRet;
}

/**
 * @brief Check whether ql_manager_server is raedy
 * 
 * @return Type_Bool 
 */
Type_Bool nblCall_audio_QMSReadyCheck(VOID)
{
    Type_Bool ablRet = FALSE;

    if(0 == access(NEU_QL_MANAGER_SERVER_PID_PATH, F_OK))
    {
        nswNeu_call_TimerDelete(E_XCALL_TIMER_QL_MANAGER_SERVER_READY_CHK);
        ablRet = TRUE;
        //Save Current Pid
        nswQlMSPId = wswNeuAtGetQlManagerServerPID();
        Type_sWord aswRet = nswNeu_call_TimerCreate(E_XCALL_TIMER_QL_MANAGER_SERVER_RESTARTED_CHK);
        if(RES_OK != aswRet)
        {
            NEU_LOG_ERROR("Timer Create Failed[%d]\r\n", aswRet);
        }
    }
    else
    {
        NEU_LOG_ERROR("File[%s] is not exist!!!\r\n", NEU_QL_MANAGER_SERVER_PID_PATH);
    }

    return ablRet;
}


Type_sWord nswCall_audio_QueneDispatch(ST_NEU_QUEUE_DATA *stQueData)
{
    Type_sWord aswRet = RES_FAIL;
    ThreadQueueData nstQueueData;
    memset(&nstQueueData, 0, sizeof(ThreadQueueData));
    if(sizeof(ThreadQueueData) != stQueData->uwDataLen)
    {
        NEU_LOG_ERROR("ThreadQueueData size:%d uwDataLen:%d\n", sizeof(ThreadQueueData), stQueData->uwDataLen);
        return RES_ERR_PARAMETER;
    }
    memcpy(&nstQueueData, stQueData->ubData, stQueData->uwDataLen);

    switch(nstQueueData.funcID)
    {
        case E_XCALL_PROC_QL_MANAGER_SERVER_READY_CHK:
        {
            NEU_LOG_CRIT("Recv E_XCALL_PROC_QL_MANAGER_SERVER_READY_CHK\r\n");
            aswRet = nswCall_audio_RtpConfSet();
            break;
        }
        case E_XCALL_PROC_QL_MANAGER_SERVER_RESTARTED_CHK:
        {
            Type_sWord nswCurrentQlMSPId = wswNeuAtGetQlManagerServerPID();
            if((0 != nswCurrentQlMSPId) && (nswCurrentQlMSPId != nswQlMSPId))
            {
                NEU_LOG_CRIT("nswQlMSPId:%d :nswCurrentQlMSPId:%d\r\n", nswQlMSPId, nswCurrentQlMSPId);
                nswQlMSPId = nswCurrentQlMSPId;
                nbTuenrRtpConfSetState = FALSE;
                aswRet = nswCall_audio_RtpConfSet();
                if(RES_OK != aswRet)
                {
                    NEU_LOG_CRIT("Reset RTP config failed, creat timer\r\n");
					aswRet = nswNeu_call_TimerCreate(E_XCALL_TIMER_QL_MANAGER_SERVER_READY_CHK);
                    if(RES_OK != aswRet)
                    {
                        NEU_LOG_ERROR("Timer Create Failed[%d]\r\n", aswRet);
                    }
                }
            }
            else
            {
                aswRet = RES_OK;
            }
            break;
        }
        default:
            NEU_LOG_ERROR("Unknow funcID:%d\r\n", nstQueueData.funcID)
            break;
    }

    return RES_OK;
}


Type_sWord aaanswCall_audio_QueneDispatch(ST_NEU_QUEUE_DATA *stQueData)
{
    Type_sWord aswRet = RES_FAIL;
    ThreadQueueData nstQueueData;
    memset(&nstQueueData, 0, sizeof(ThreadQueueData));
    if(sizeof(ThreadQueueData) != stQueData->uwDataLen)
    {
        NEU_LOG_ERROR("ThreadQueueData size:%d uwDataLen:%d\n", sizeof(ThreadQueueData), stQueData->uwDataLen);
        return RES_ERR_PARAMETER;
    }
    memcpy(&nstQueueData, stQueData->ubData, stQueData->uwDataLen);

    switch(nstQueueData.funcID)
    {
        case E_XCALL_PROC_QL_MANAGER_SERVER_READY_CHK:
        {
            NEU_LOG_CRIT("Recv E_XCALL_PROC_QL_MANAGER_SERVER_READY_CHK\r\n");
            aswRet = nswCall_audio_RtpConfSet();
            break;
        }
        case E_XCALL_PROC_QL_MANAGER_SERVER_RESTARTED_CHK:
        {
            Type_sWord nswCurrentQlMSPId = wswNeuAtGetQlManagerServerPID();
            if((0 != nswCurrentQlMSPId) && (nswCurrentQlMSPId != nswQlMSPId))
            {
                NEU_LOG_CRIT("nswQlMSPId:%d :nswCurrentQlMSPId:%d\r\n", nswQlMSPId, nswCurrentQlMSPId);
                nswQlMSPId = nswCurrentQlMSPId;
                nbTuenrRtpConfSetState = FALSE;
                aswRet = nswCall_audio_RtpConfSet();
                if(RES_OK != aswRet)
                {
                    NEU_LOG_CRIT("Reset RTP config failed, creat timer\r\n");
					aswRet = nswNeu_call_TimerCreate(E_XCALL_TIMER_QL_MANAGER_SERVER_READY_CHK);
                    if(RES_OK != aswRet)
                    {
                        NEU_LOG_ERROR("Timer Create Failed[%d]\r\n", aswRet);
                    }
                }
            }
            else
            {
                aswRet = RES_OK;
            }
            break;
        }
        default:
            NEU_LOG_ERROR("Unknow funcID:%d\r\n", nstQueueData.funcID)
            break;
    }

    return RES_OK;
}

Type_sWord bbbbbbbbbbbbbaaanswCall_audio_QueneDispatch(ST_NEU_QUEUE_DATA *stQueData)
{
    Type_sWord aswRet = RES_FAIL;
    ThreadQueueData nstQueueData;
    memset(&nstQueueData, 0, sizeof(ThreadQueueData));
    if(sizeof(ThreadQueueData) != stQueData->uwDataLen)
    {
        NEU_LOG_ERROR("ThreadQueueData size:%d uwDataLen:%d\n", sizeof(ThreadQueueData), stQueData->uwDataLen);
        return RES_ERR_PARAMETER;
    }
    memcpy(&nstQueueData, stQueData->ubData, stQueData->uwDataLen);

    switch(nstQueueData.funcID)
    {
        case E_XCALL_PROC_QL_MANAGER_SERVER_READY_CHK:
        {
            NEU_LOG_CRIT("Recv E_XCALL_PROC_QL_MANAGER_SERVER_READY_CHK\r\n");
            aswRet = nswCall_audio_RtpConfSet();
            break;
        }
        case E_XCALL_PROC_QL_MANAGER_SERVER_RESTARTED_CHK:
        {
            Type_sWord nswCurrentQlMSPId = wswNeuAtGetQlManagerServerPID();
            if((0 != nswCurrentQlMSPId) && (nswCurrentQlMSPId != nswQlMSPId))
            {
                NEU_LOG_CRIT("nswQlMSPId:%d :nswCurrentQlMSPId:%d\r\n", nswQlMSPId, nswCurrentQlMSPId);
                nswQlMSPId = nswCurrentQlMSPId;
                nbTuenrRtpConfSetState = FALSE;
                aswRet = nswCall_audio_RtpConfSet();
                if(RES_OK != aswRet)
                {
                    NEU_LOG_CRIT("Reset RTP config failed, creat timer\r\n");
					aswRet = nswNeu_call_TimerCreate(E_XCALL_TIMER_QL_MANAGER_SERVER_READY_CHK);
                    if(RES_OK != aswRet)
                    {
                        NEU_LOG_ERROR("Timer Create Failed[%d]\r\n", aswRet);
                    }
                }
            }
            else
            {
                aswRet = RES_OK;
            }
            break;
        }
        default:
            NEU_LOG_ERROR("Unknow funcID:%d\r\n", nstQueueData.funcID)
            break;
    }

    return RES_OK;
}


Type_sWord nswCall_audio_RtpConfSet(VOID)
{
    Type_sWord aswRet = RES_FAIL;

#ifdef SOC_PLATFORM_QUECTEL
    if((!nbTuenrRtpConfSetState) && nblCall_audio_QMSReadyCheck())
#elif defined SOC_PLATFORM_LINKSCI
    if(!nbTuenrRtpConfSetState)
#endif
    {		
        nbTuenrRtpConfSetState = TRUE;
        //Set RTP config
        neu_rtp_config_t ntRtpCfg = {0};
        Type_uByte FromnubAddr[20] = {0};
        Type_uByte nubAddr[20] = {0};

        strcpy(FromnubAddr, NEU_RTP_IHU_FROM_ADDR);
        strcpy(nubAddr, NEU_RTP_IHU_ADDR);

        ntRtpCfg.auwSrcIp    = inet_addr(FromnubAddr);
        ntRtpCfg.ahSrcPort   = NEU_RTP_TBOX_PORT;
        ntRtpCfg.auwDstIp    = inet_addr(nubAddr);
        ntRtpCfg.ahDstPort   = NEU_RTP_IHU_PORT;
        ntRtpCfg.astChanType = NEU_RTP_AUDIO_CHAN_VOICE;
#ifdef SOC_PLATFORM_QUECTEL
        ntRtpCfg.pltype      = NEU_RTP_PAYLOAD_TYPE_G711A;
#elif  defined SOC_PLATFORM_LINKSCI
	    ntRtpCfg.pltype      = NEU_RTP_PAYLOAD_TYPE_PCM;
#endif
        ntRtpCfg.sample_rate = 8000;
        ntRtpCfg.chanel_num  = 1;
        ntRtpCfg.psize       = 1024;
        ntRtpCfg.socket_ttl  = 20;
        ntRtpCfg.ssrc        = 0xFFFF0000;
	strcpy(ntRtpCfg.eth_name,"eth0.4");

        aswRet = wswAudioRtpConfig(&ntRtpCfg);
        if(RES_OK != aswRet)
        {
            NEU_LOG_ERROR("Set RTP config failed[%d]!\r\n", aswRet);
            return aswRet;
        }
    }

    return aswRet;
}


Type_sWord aaaaaaaaaaaaaaaaanswCall_audio_RtpConfSet(VOID)
{
    Type_sWord aswRet = RES_FAIL;

#ifdef SOC_PLATFORM_QUECTEL
    if((!nbTuenrRtpConfSetState) && nblCall_audio_QMSReadyCheck())
#elif defined SOC_PLATFORM_LINKSCI
    if(!nbTuenrRtpConfSetState)
#endif
    {		
        nbTuenrRtpConfSetState = TRUE;
        //Set RTP config
        neu_rtp_config_t ntRtpCfg = {0};
        Type_uByte FromnubAddr[20] = {0};
        Type_uByte nubAddr[20] = {0};

        strcpy(FromnubAddr, NEU_RTP_IHU_FROM_ADDR);
        strcpy(nubAddr, NEU_RTP_IHU_ADDR);

        ntRtpCfg.auwSrcIp    = inet_addr(FromnubAddr);
        ntRtpCfg.ahSrcPort   = NEU_RTP_TBOX_PORT;
        ntRtpCfg.auwDstIp    = inet_addr(nubAddr);
        ntRtpCfg.ahDstPort   = NEU_RTP_IHU_PORT;
        ntRtpCfg.astChanType = NEU_RTP_AUDIO_CHAN_VOICE;
#ifdef SOC_PLATFORM_QUECTEL
        ntRtpCfg.pltype      = NEU_RTP_PAYLOAD_TYPE_G711A;
#elif  defined SOC_PLATFORM_LINKSCI
	    ntRtpCfg.pltype      = NEU_RTP_PAYLOAD_TYPE_PCM;
#endif
        ntRtpCfg.sample_rate = 8000;
        ntRtpCfg.chanel_num  = 1;
        ntRtpCfg.psize       = 1024;
        ntRtpCfg.socket_ttl  = 20;
        ntRtpCfg.ssrc        = 0xFFFF0000;
	strcpy(ntRtpCfg.eth_name,"eth0.4");

        aswRet = wswAudioRtpConfig(&ntRtpCfg);
        if(RES_OK != aswRet)
        {
            NEU_LOG_ERROR("Set RTP config failed[%d]!\r\n", aswRet);
            return aswRet;
        }
    }

    return aswRet;
}