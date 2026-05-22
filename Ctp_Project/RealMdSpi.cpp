#include <iostream>
#include <fstream>
#include "RealMdSpi.h"

extern CThostFtdcMdApi* g_pMdUserApi;
extern TThostFtdcBrokerIDType gBrokerID;
extern TThostFtdcInvestorIDType gInvesterID;
extern TThostFtdcPasswordType gInvesterPassword;
extern char* g_pInstrumentID[];
extern int instrumentNum;

// 回调函数
// 连接成功应答
void RealMdSpi::OnFrontConnected()
{
	// 开始登录
	CThostFtdcReqUserLoginField loginReq;
	memset(&loginReq, 0, sizeof(loginReq));
	strcpy(loginReq.BrokerID, gBrokerID);
	strcpy(loginReq.UserID, gInvesterID);
	strcpy(loginReq.Password, gInvesterPassword);
	static int requestID = 0; // 请求编号
	int rt = g_pMdUserApi->ReqUserLogin(&loginReq, ++requestID);
	if (!rt)
		std::cout << "发送登录请求成功" << std::endl;
	else
		std::cerr << "发送登录请求失败，错误码："<< rt << std::endl;
}

// 断开连接通知
void RealMdSpi::OnFrontDisconnected(int nReason)
{
	std::cerr << "连接断开" << std::endl;
	std::cerr << "错误码： " << nReason << std::endl;
}

// 心跳超时警告
void RealMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
	std::cerr << "心跳超时" << std::endl;
	std::cerr << "距上次连接时间： " << nTimeLapse << std::endl;
}

// 登录应答
void RealMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!pRspInfo || (pRspInfo->ErrorID == 0))
	{
		std::cout << "账户登录成功" << std::endl;
		std::cout << "交易日： " << pRspUserLogin->TradingDay << std::endl;
		std::cout << "登录时间： " << pRspUserLogin->LoginTime << std::endl;
		std::cout << "经纪商： " << pRspUserLogin->BrokerID << std::endl;
		std::cout << "帐户名： " << pRspUserLogin->UserID << std::endl;
		std::cout << "会话编号：  " << pRspUserLogin->SessionID << std::endl;
		std::cout << "前置编号：  " << pRspUserLogin->FrontID << std::endl;
		std::cout << "最大报单引用： " << pRspUserLogin->MaxOrderRef << std::endl;
		std::cout << "上期所时间： " << pRspUserLogin->SHFETime << std::endl;

		// 开始订阅行情
		int rt = g_pMdUserApi->SubscribeMarketData(g_pInstrumentID, instrumentNum);
		if (!rt)
			std::cout << "发送订阅行情请求成功" << std::endl;
		else
			std::cerr << "发送订阅行情请求失败" << std::endl;
	}
	else
		std::cerr << "错误 ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
}

// 登出应答
void RealMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!pRspInfo || (pRspInfo->ErrorID == 0))
	{
		std::cout << "账户登出成功" << std::endl;
		std::cout << "经纪商： " << pUserLogout->BrokerID << std::endl;
		std::cout << "帐户名： " << pUserLogout->UserID << std::endl;
	}
	else
		std::cerr << "错误 ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
}

// 错误通知
void RealMdSpi::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo && (pRspInfo->ErrorID != 0))
		std::cerr << "错误 ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
}

// 订阅行情应答
void RealMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!pRspInfo || (pRspInfo->ErrorID == 0))
	{
		std::cout << "订阅行情成功" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->InstrumentID << std::endl;
	}
	else
		std::cerr << "错误 ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
}

// 取消订阅行情应答
void RealMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!pRspInfo || (pRspInfo->ErrorID == 0))
	{
		std::cout << "取消订阅行情成功" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->InstrumentID << std::endl;
	}
	else
		std::cerr << "错误 ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
}

// 订阅询价应答
void RealMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!pRspInfo || (pRspInfo->ErrorID == 0))
	{
		std::cout << "订阅询价成功" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->InstrumentID << std::endl;
	}
	else
		std::cerr << "错误 ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
}

// 取消订阅询价应答
void RealMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!pRspInfo || (pRspInfo->ErrorID == 0))
	{
		std::cout << "取消订阅询价成功" << std::endl;
		std::cout << "合约代码： " << pSpecificInstrument->InstrumentID << std::endl;
	}
	else
		std::cerr << "错误 ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
}

// 行情详情通知，频率每秒两次
void RealMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
	std::cout << "获得深度行情" << std::endl;
	std::cout << "交易日： " << pDepthMarketData->TradingDay << std::endl;
	std::cout << "交易所代码： " << pDepthMarketData->ExchangeID << std::endl;
	std::cout << "合约代码： " << pDepthMarketData->InstrumentID << std::endl;
	std::cout << "合约在交易所的代码： " << pDepthMarketData->ExchangeInstID << std::endl;
	std::cout << "最新价： " << pDepthMarketData->LastPrice << std::endl;
	std::cout << "数量： " << pDepthMarketData->Volume << std::endl;

	// 取消订阅行情
	//int rt = g_pMdUserApi->UnSubscribeMarketData(g_pInstrumentID, instrumentNum);
	//if (!rt)
	//	std::cout << "发送取消订阅行情请求成功" << std::endl;
	//else
	//	std::cerr << "发送取消订阅行情请求失败" << std::endl;
}

// 询价详情通知
void RealMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp)
{
	std::cout << "获得询价结果" << std::endl;
	std::cout << "交易日： " << pForQuoteRsp->TradingDay << std::endl;
	std::cout << "交易所代码： " << pForQuoteRsp->ExchangeID << std::endl;
	std::cout << "合约代码： " << pForQuoteRsp->InstrumentID << std::endl;
	std::cout << "询价编号： " << pForQuoteRsp->ForQuoteSysID << std::endl;
}