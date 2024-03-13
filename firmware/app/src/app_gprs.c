/**
 * @file app_gprs.c
 * @author rohirto
 * @brief GPRS related functions
 * @version 0.1
 * @date 2024-03-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "app.h"
#include "app_gprs.h"
#include "app_gps.h"

bool networkFlag = false;
//BSNL Context
Network_PDP_Context_t bsnl_context = {
                .apn        ="bsnlnet",  //airtelgprs.com  -> For Airtel    //bsnlnet  -> For BSNL
                .userName   = ""    ,
                .userPasswd = ""
            };
//Airtel Context
Network_PDP_Context_t airtel_context = {
                .apn        ="airtelgprs.com",  //airtelgprs.com  -> For Airtel    //bsnlnet  -> For BSNL
                .userName   = ""    ,
                .userPasswd = ""
            };

int Http_Post(const char* domain, int port,const char* path,uint8_t* body, uint16_t bodyLen, char* retBuffer, int bufferLen)
{
    uint8_t ip[16];
    bool flag = false;
    uint16_t recvLen = 0;

    //connect server
    memset(ip,0,sizeof(ip));
    if(DNS_GetHostByName2(domain,ip) != 0)
    {
         #if USE_TRACE
        Trace(2,"get ip error");
        #endif
        return -1;
    }
    // Trace(2,"get ip success:%s -> %s",domain,ip);
    char* servInetAddr = ip;
    char* temp = OS_Malloc(2048);
    if(!temp)
    {
         #if USE_TRACE
        Trace(2,"malloc fail");
        #endif
        return -1;
    }
    snprintf(temp,2048,"POST %s HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: Keep-Alive\r\nHost: %s\r\nContent-Length: %d\r\n\r\n",
                            path,domain,bodyLen);
    char* pData = temp;
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd < 0){
         #if USE_TRACE
        Trace(2,"socket fail");
        #endif
        OS_Free(temp);
        return -1;
    }
    // Trace(2,"fd:%d",fd);

    struct sockaddr_in sockaddr;
    memset(&sockaddr,0,sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    inet_pton(AF_INET,servInetAddr,&sockaddr.sin_addr);

    int ret = connect(fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr_in));
    if(ret < 0){
         #if USE_TRACE
        Trace(2,"socket connect fail");
        #endif
        OS_Free(temp);
        close(fd);
        return -1;
    }
    // Trace(2,"socket connect success");
     #if USE_TRACE
    Trace(2,"send request:%s",pData);
    #endif
    ret = send(fd, pData, strlen(pData), 0);
    if(ret < 0){
         #if USE_TRACE
        Trace(2,"socket send fail");
        #endif
        OS_Free(temp);
        close(fd);
        return -1;
    }
    ret = send(fd, body, bodyLen, 0);
    if(ret < 0){
         #if USE_TRACE
        Trace(2,"socket send fail");
        #endif
        OS_Free(temp);
        close(fd);
        return -1;
    }
    // Trace(2,"socket send success");

    struct fd_set fds;
    struct timeval timeout={12,0};
    FD_ZERO(&fds);
    FD_SET(fd,&fds);
    while(!flag)
    {
        ret = select(fd+1,&fds,NULL,NULL,&timeout);
        // Trace(2,"select return:%d",ret);
        switch(ret)
        {
            case -1:
                 #if USE_TRACE
                Trace(2,"select error");
                #endif
                flag = true;
                break;
            case 0:
                 #if USE_TRACE
                Trace(2,"select timeout");
                #endif
                flag = true;
                break;
            default:
                if(FD_ISSET(fd,&fds))
                {
                    memset(retBuffer,0,bufferLen);
                    ret = recv(fd,retBuffer,bufferLen,0);
                    recvLen += ret;
                    if(ret < 0)
                    {
                         #if USE_TRACE
                        Trace(2,"recv error");
                        #endif
                        flag = true;
                        break;
                    }
                    else if(ret == 0)
                    {
                         #if USE_TRACE
                        Trace(2,"ret == 0");
                        #endif
                        break;
                    }
                    else if(ret < 1352)
                    {
                        GPS_DEBUG_I("recv len:%d,data:%s",recvLen,retBuffer);
                        close(fd);
                        OS_Free(temp);
                        return recvLen;
                    }
                }
                break;
        }
    }
    close(fd);
    OS_Free(temp);
    return -1;
}

#if USE_SMS
void SMSInit()
{
    if(!SMS_SetFormat(SMS_FORMAT_TEXT,SIM0))
    {
        #if USE_TRACE
        Trace(1,"sms set format error");
        #endif
        return;
    }
    SMS_Parameter_t smsParam = {
        .fo = 17 ,
        .vp = 167,
        .pid= 0  ,
        .dcs= 8  ,//0:English 7bit, 4:English 8 bit, 8:Unicode 2 Bytes
    };
    if(!SMS_SetParameter(&smsParam,SIM0))
    {
        #if USE_TRACE
        Trace(1,"sms set parameter error");
        #endif
        return;
    }
    if(!SMS_SetNewMessageStorage(SMS_STORAGE_SIM_CARD))
    {
        #if USE_TRACE
        Trace(1,"sms set message storage fail");
        #endif
        return;
    }
}
#endif

