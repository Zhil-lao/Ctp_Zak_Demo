#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
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
TThostFtdcPriceType gLimitPrice = 16000;

// ---- Bench 测试全局变量 ---- //
#define MAX_BENCH 10000
char g_benRefs[MAX_BENCH][13];
volatile int g_benRefCnt = 0;
volatile char g_benDone[MAX_BENCH] = {0};
volatile int g_benOk = 0;
volatile int g_benSent = 0;

extern TThostFtdcOrderRefType order_ref;  // RealTradeSpi.cpp 中定义

void bench_thread_func()
{
	auto start = std::chrono::steady_clock::now();
	int nextRef = atoi(order_ref) + 1;

	std::cout << "\n[Bench] 开始发单，起始 OrderRef=" << nextRef << std::endl;

	while (true) {
		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= 1)
			break;
		if (g_benRefCnt >= MAX_BENCH) break;

		int idx = g_benRefCnt;
		sprintf(g_benRefs[idx], "%d", nextRef++);
		sprintf(order_ref, "%s", g_benRefs[idx]);
		g_benRefCnt = idx + 1;

		CThostFtdcInputOrderField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, gBrokerID);
		strcpy(req.InvestorID, gInvesterID);
		strcpy(req.InstrumentID, g_pTradeInstrumentID);
		strcpy(req.OrderRef, order_ref);
		req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		req.Direction = THOST_FTDC_D_Buy;
		req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		req.IsSwapOrder = 0;
		req.LimitPrice = gLimitPrice;
		req.VolumeTotalOriginal = 1;
		req.TimeCondition = THOST_FTDC_TC_IOC;
		req.VolumeCondition = THOST_FTDC_VC_AV;
		req.MinVolume = 1;
		req.ContingentCondition = THOST_FTDC_CC_Immediately;
		req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		req.IsAutoSuspend = 0;
		req.UserForceClose = 0;

		static int requestID = 0;
		g_pTradeUserApi->ReqOrderInsert(&req, ++requestID);
		g_benSent++;
	}

	std::cout << "[Bench] 报单结束，等待回报..." << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << "\n========== Bench Result ==========" << std::endl;
	std::cout << "报单总数:     " << g_benSent << std::endl;
	std::cout << "正确应答数:   " << g_benOk << std::endl;
	std::cout << "==================================\n" << std::endl;
}

int main()
{
	//g_pMdUserApi = CThostFtdcMdApi::CreateFtdcMdApi();
	//CThostFtdcMdSpi* pMdUserSpi = new RealMdSpi;
	//g_pMdUserApi->RegisterSpi(pMdUserSpi);
	//g_pMdUserApi->RegisterFront(gMdFrontAddr);
	//g_pMdUserApi->Init();

	char* tdFlowPath = ".//TradingData/";                                    // 存放交易接口在本地生成的流文件的文件路径（.con）
	g_pTradeUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(tdFlowPath);  // 创建交易实例
	RealTradeSpi* pTradeSpi = new RealTradeSpi;
	g_pTradeUserApi->RegisterSpi(pTradeSpi);
	g_pTradeUserApi->RegisterFront(gTradeFrontAddr);
	g_pTradeUserApi->SubscribePublicTopic(THOST_TERT_QUICK);
	g_pTradeUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);
	g_pTradeUserApi->Init();

	// 等待登录完成后启动 bench 线程
	while (!pTradeSpi->loginFlag) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	std::thread benchThread(bench_thread_func);
	benchThread.detach();

	//g_pMdUserApi->Join();
	//delete pMdUserSpi;
	//g_pMdUserApi->Release();

	g_pTradeUserApi->Join();
	delete pTradeSpi;
	g_pTradeUserApi->Release();

	return 0;
}