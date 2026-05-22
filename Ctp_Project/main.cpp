#include <iostream>
#include <stdio.h>
#include <string>
#include "RealMdSpi.h"
#include "RealTradeSpi.h"

using namespace std;

// SimNow
TThostFtdcBrokerIDType gBrokerID = "9999";
TThostFtdcInvestorIDType gInvesterID = "263248";
TThostFtdcPasswordType gInvesterPassword = "ZHANGzhi426!";
TThostFtdcAppIDType	AppID = "simnow_client_test";
TThostFtdcAuthCodeType	AuthCode = "0000000000000000";

// 行情参数
CThostFtdcMdApi* g_pMdUserApi = nullptr;
char gMdFrontAddr[] = "tcp://182.254.243.31:30011";
char* g_pInstrumentID[] = { "CF703" };
int instrumentNum = 1;

// 交易参数
CThostFtdcTraderApi* g_pTradeUserApi = nullptr;
char gTradeFrontAddr[] = "tcp://182.254.243.31:30001";
TThostFtdcInstrumentIDType g_pTradeInstrumentID = "CF703";
TThostFtdcPriceType gLimitPrice = 16160;

int main()
{
	g_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi();
	CThostFtdcMdSpi* pMdUserSpi = new RealMdSpi;
	g_pMdUserApi->RegisterSpi(pMdUserSpi);
	g_pMdUserApi->RegisterFront(gMdFrontAddr);
	g_pMdUserApi->Init();

	char* tdFlowPath = ".//TradingData/";                                    // 存放交易接口在本地生成的流文件的文件路径（.con）
	g_pTradeUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(tdFlowPath);  // 创建交易实例
	RealTradeSpi* pTradeSpi = new RealTradeSpi;
	g_pTradeUserApi->RegisterSpi(pTradeSpi);
	g_pTradeUserApi->RegisterFront(gTradeFrontAddr);
	g_pTradeUserApi->SubscribePublicTopic(THOST_TERT_RESTART);
	g_pTradeUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);
	g_pTradeUserApi->Init();

	g_pMdUserApi->Join();
	delete pMdUserSpi;
	g_pMdUserApi->Release();

	g_pTradeUserApi->Join();
	delete pTradeSpi;
	g_pTradeUserApi->Release();

	return 0;
}