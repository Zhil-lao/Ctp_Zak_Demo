#pragma once

#include "ThostFtdcTraderApi.h"
#include <chrono>

class RealTradeSpi : public CThostFtdcTraderSpi
{
public:
	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	void OnFrontConnected();

	// 客户端认证响应
	void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///登录请求响应
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///错误应答
	void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	void OnHeartBeatWarning(int nTimeLapse);

	///登出请求响应
	void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///投资者结算结果确认响应
	void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约响应
	void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询资金账户响应
	void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓响应
	void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单录入请求响应
	void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单操作请求响应
	void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单通知
	void OnRtnOrder(CThostFtdcOrderField* pOrder);

	///成交通知
	void OnRtnTrade(CThostFtdcTradeField* pTrade);


// ---- 自定义函数, 写成void，不需要返回值，也不需要传参，可直接调用该方法 ---- //
public:
	/// 登陆成功的标识
	bool loginFlag;

	// ---- Benchmark ---- //
	void runBenchmark(int durationSeconds, TThostFtdcInstrumentIDType instrumentID, TThostFtdcPriceType price, TThostFtdcVolumeType volume);
	void printBenchmarkResult();
	void sendNextBenchOrder(TThostFtdcInstrumentIDType instrumentID, TThostFtdcPriceType price, TThostFtdcVolumeType volume);

	/// 买开仓
	void buy_open(TThostFtdcInstrumentIDType instrumentID, TThostFtdcPriceType NewPrice, TThostFtdcVolumeType volume);

	/// 买平仓(默认平仓）
	void buy_close(TThostFtdcInstrumentIDType instrumentID, TThostFtdcPriceType NewPrice, TThostFtdcVolumeType volume, TThostFtdcTimeConditionType CombOffsetFlag=THOST_FTDC_OF_Close);

	/// 卖开仓
	void sell_open(TThostFtdcInstrumentIDType instrumentID, TThostFtdcPriceType NewPrice, TThostFtdcVolumeType volume);

	/// 卖平仓(默认平仓）
	void sell_close(TThostFtdcInstrumentIDType instrumentID, TThostFtdcPriceType NewPrice, TThostFtdcVolumeType volume, TThostFtdcTimeConditionType CombOffsetFlag=THOST_FTDC_OF_Close);

private:
	///客户端认证请求
	void reqAuthenticate();

	/// 登出请求
	void reqUserLogout(); 

	/// 投资者结果确认
	void reqSettlementInfoConfirm();

	void reqQrySettlementInfo();

	/// 请求查询资金帐户
	void reqQueryTradingAccount(); 

	/// 请求查询投资者持仓
	void reqQueryInvestorPosition();

	/// 请求报单操作
	void reqOrderAction(CThostFtdcOrderField* pOrder);

	/// 是否收到错误信息
	bool isErrorRspInfo(CThostFtdcRspInfoField* pRspInfo); 

	/// 是否我的报单回报
	bool isMyOrder(CThostFtdcOrderField* pOrder); 

	/// 是否正在交易的报单
	bool isTradingOrder(CThostFtdcOrderField* pOrder);

	// ---- Benchmark state ---- //
	int  benchNextOrderRef;
	int  benchSent;
	int  benchRspOk;
	int  benchRspErr;
	int  benchFlowCtrl;
	int  benchInFlight;
	int  benchMaxInFlight;
	int  benchRtnOrderCount;
	int  benchRtnTradeCount;
	int  benchDurationSeconds;
	bool benchRunning;
	std::chrono::steady_clock::time_point benchStartTime;
};
