#include <iostream>
#include <iomanip>
#include <windows.h>
#include <time.h>
#include <thread>
#include <chrono>
#include <sstream>
#include "RealTradeSpi.h"
#include "ThostFtdcUserApiDataType.h"

// ---- 全局参数声明 ---- //
extern TThostFtdcBrokerIDType gBrokerID;                      // 模拟经纪商代码
extern TThostFtdcInvestorIDType gInvesterID;                  // 投资者账户名
extern TThostFtdcPasswordType gInvesterPassword;              // 投资者密码
extern CThostFtdcTraderApi* g_pTradeUserApi;                  // 交易指针
extern char gTradeFrontAddr[];                                // 模拟交易前置地址
extern TThostFtdcInstrumentIDType g_pTradeInstrumentID;       // 所交易的合约代码

extern TThostFtdcPriceType gLimitPrice;                       // 交易价格

extern TThostFtdcAppIDType	AppID;							  // AppID
extern TThostFtdcAuthCodeType AuthCode;						  // 授权码

// 会话参数
TThostFtdcFrontIDType	trade_front_id;		//前置编号
TThostFtdcSessionIDType	session_id;		    //会话编号
TThostFtdcOrderRefType	order_ref;			//报单引用

// ---- Bench: OrderRef 匹配（定义在 main.cpp）---- //
#define MAX_BENCH 10000
extern char g_benRefs[MAX_BENCH][13];
extern volatile int g_benRefCnt;
extern volatile char g_benDone[MAX_BENCH];
extern volatile int g_benOk;

// 方案 A: 处理 std::string 或可隐式转换为 std::string 的类型
void printField(const std::string& label, const std::string& value) {
	std::cout << "  " << std::left << std::setw(15) << label << " : " << value << std::endl;
}

// 方案 B: 专门处理 C 风格字符串 (const char*)
// 这能覆盖 typedef char xxx[9] 退化后的情况，也能覆盖 const char*
void printField(const std::string& label, const char* value) {
	// 安全处理：防止 nullptr，并自动截断到第一个 \0
	std::string safeValue = (value != nullptr) ? std::string(value) : std::string("(null)");
	std::cout << "  " << std::left << std::setw(15) << label << " : " << safeValue << std::endl;
}

// 方案 C: 处理单字符 char (针对 typedef char xxx)
// 如果传入的是单个 char，会自动匹配这个重载（如果上面两个不匹配）
// 但为了保险，也可以显式提供一个 char 版本，或者依赖隐式转换
void printField(const std::string& label, char value) {
	std::cout << "  " << std::left << std::setw(15) << label << " : " << value << std::endl;
}

void RealTradeSpi::OnFrontConnected()
{
	std::cout << "====网络连接成功===" << std::endl;

	// 登录前客户端认证
	reqAuthenticate();
}

void RealTradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		CThostFtdcReqUserLoginField loginReq;
		memset(&loginReq, 0, sizeof(loginReq));
		strcpy(loginReq.BrokerID, gBrokerID);
		strcpy(loginReq.UserID, gInvesterID);
		strcpy(loginReq.Password, gInvesterPassword);
		static int requestID = 0; // 请求编号
		int rt = g_pTradeUserApi->ReqUserLogin(&loginReq, requestID);
		if (!rt)
			std::cout << ">>>>>>交易--发送登录请求成功" << std::endl;
		else
			std::cerr << "--->>>交易--发送登录请求失败" << std::endl;
	}
}

void RealTradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo)) {
		loginFlag = true;
		std::cout << "\n[SUCCESS] 交易账户登录成功" << std::endl;
		std::cout << "--------------------------------" << std::endl;
		printField("交易日", pRspUserLogin->TradingDay);
		printField("登录时间", pRspUserLogin->LoginTime);
		printField("经纪商", pRspUserLogin->BrokerID);
		printField("账户名", pRspUserLogin->UserID);
		std::cout << "--------------------------------\n" << std::endl;

		// 保存会话参数
		trade_front_id = pRspUserLogin->FrontID;		 // 前置编号
		session_id = pRspUserLogin->SessionID;			 // 会话编号
		//int nOrderRef = atoi(pRspUserLogin->MaxOrderRef) + 1;
		//sprintf(order_ref, "%d", nOrderRef);
		strcpy(order_ref, pRspUserLogin->MaxOrderRef);
		// 投资者结算结果确认，这里针对的是昨日的结算结果，其实只需要调一次
		reqSettlementInfoConfirm();
	}
}

void RealTradeSpi::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	isErrorRspInfo(pRspInfo);
}

void RealTradeSpi::OnFrontDisconnected(int nReason)
{
	std::cerr << "=====网络连接断开=====" << std::endl;
	std::cerr << "错误码： " << nReason << std::endl;
}

void RealTradeSpi::OnHeartBeatWarning(int nTimeLapse)
{
	std::cerr << "=====网络心跳超时=====" << std::endl;
	std::cerr << "距上次连接时间： " << nTimeLapse << std::endl;
}

void RealTradeSpi::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		loginFlag = false; // 登出就不能再交易了 
		std::cout << "=====账户登出成功=====" << std::endl;
		std::cout << "经纪商： " << pUserLogout->BrokerID << std::endl;
		std::cout << "帐户名： " << pUserLogout->UserID << std::endl;
	}
}

void RealTradeSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "\n[SUCCESS] 投资者结算结果确认成功" << std::endl;
		std::cout << "--------------------------------" << std::endl;
		printField("确认日期", pSettlementInfoConfirm->ConfirmDate);
		printField("确认时间", pSettlementInfoConfirm->ConfirmTime);
		std::cout << "--------------------------------\n" << std::endl;
		// 查询投资者结算单
		reqQrySettlementInfo();
	}
}

void RealTradeSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{	
	if (!isErrorRspInfo(pRspInfo))
	{
		if (pSettlementInfo == nullptr) {
			std::cout << "\n[SUCCESS] 投资者结算结果查询成功" << std::endl;
			std::cout << "--------------------------------" << std::endl;
			printField("交易日", pSettlementInfo->TradingDay);
			printField("结算编号", pSettlementInfo->SettlementID);
			printField("消息正文", pSettlementInfo->Content);
			std::cout << "--------------------------------\n" << std::endl;
		}
		// simnow的结算单查询接口有点问题，返回的pSettlementInfo是nullptr，所以直接进入下一步查询合约
		// 请求查询合约
		CThostFtdcQryInstrumentField instrumentReq;
		memset(&instrumentReq, 0, sizeof(instrumentReq));
		strcpy(instrumentReq.InstrumentID, g_pTradeInstrumentID);
		static int requestID = 0; // 请求编号
		int rt = g_pTradeUserApi->ReqQryInstrument(&instrumentReq, requestID);
		if (!rt)
			std::cout << ">>>>>>发送合约查询请求成功" << std::endl;
		else
			std::cerr << "--->>>发送合约查询请求失败" << std::endl;
	}
};

void RealTradeSpi::OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID,bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "\n[SUCCESS] 查询合约结果成功" << std::endl;
		std::cout << "--------------------------------" << std::endl;
		printField("交易所代码", pInstrument->ExchangeID);
		printField("合约代码", pInstrument->InstrumentID);
		printField("合约在交易所的代码", pInstrument->ExchangeInstID);
		printField("执行价", pInstrument->StrikePrice);
		printField("到期日", pInstrument->EndDelivDate);
		printField("当前交易状态", pInstrument->IsTrading);
		std::cout << "--------------------------------\n" << std::endl;
		// 请求查询投资者资金账户
		reqQueryTradingAccount();
	}
}

void RealTradeSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====查询投资者资金账户成功=====" << std::endl;
		std::cout << "投资者账号： " << pTradingAccount->AccountID << std::endl;
		std::cout << "可用资金： " << pTradingAccount->Available << std::endl;
		std::cout << "可取资金： " << pTradingAccount->WithdrawQuota << std::endl;
		std::cout << "当前保证金: " << pTradingAccount->CurrMargin << std::endl;
		std::cout << "平仓盈亏： " << pTradingAccount->CloseProfit << std::endl;

		// 请求查询投资者持仓
		reqQueryInvestorPosition();
	}
}

void RealTradeSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID,bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====查询投资者持仓成功=====" << std::endl;
		if (pInvestorPosition)
		{
			std::cout << "合约代码： " << pInvestorPosition->InstrumentID << std::endl;
			std::cout << "开仓价格： " << pInvestorPosition->OpenAmount << std::endl;
			std::cout << "开仓量： " << pInvestorPosition->OpenVolume << std::endl;
			std::cout << "开仓方向： " << pInvestorPosition->PosiDirection << std::endl;
			std::cout << "占用保证金：" << pInvestorPosition->UseMargin << std::endl;
		}
		else
			std::cout << "----->该合约未持仓" << std::endl;

		//std::cout << "=====开始进入策略交易=====" << std::endl;
		//// 需要注意bIsLast的值，CTP的查询接口是分批返回的，只有当bIsLast为true时才表示查询结束，此时才可以进行下一步的交易操作，否则导致重复下单
		//if (loginFlag && bIsLast)
		//	// FAK order: limit price + IOC + AV (fill-or-kill, avoid accumulating positions)
		//	int rt = sendBenchOrder(g_pTradeInstrumentID, gLimitPrice, 1,
		//		THOST_FTDC_D_Buy,
		//		THOST_FTDC_OPT_LimitPrice,
		//		THOST_FTDC_TC_IOC,
		//		THOST_FTDC_VC_AV);
	}
}

void RealTradeSpi::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====报单录入成功=====" << std::endl;
		std::cout << "合约代码： " << pInputOrder->InstrumentID << std::endl;
		std::cout << "价格： " << pInputOrder->LimitPrice << std::endl;
		std::cout << "数量： " << pInputOrder->VolumeTotalOriginal << std::endl;
		std::cout << "开仓方向： " << pInputOrder->Direction << std::endl;
	}
}

void RealTradeSpi::OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::cout << "=====报单操作成功=====" << std::endl;
		std::cout << "合约代码： " << pInputOrderAction->InstrumentID << std::endl;
		std::cout << "操作标志： " << pInputOrderAction->ActionFlag;
	}
}

void RealTradeSpi::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
	// ---- Bench: 匹配 OrderRef，只统计首次非 Unknown 状态 ---- //
	for (int i = 0; i < g_benRefCnt; i++) {
		if ((pOrder->FrontID == trade_front_id) &&
			(pOrder->SessionID == session_id) &&
			(strcmp(pOrder->OrderRef, g_benRefs[i]) == 0)) {
			if (!g_benDone[i] && pOrder->OrderStatus != THOST_FTDC_OST_Unknown) {
				g_benDone[i] = 1;
				g_benOk++;
			}
			return;
		}
	}

	// char str[10];
	// sprintf(str, "%d", pOrder->OrderSubmitStatus);
	// int orderState = atoi(str) - 48;	//报单状态0=已经提交，3=已经接受

	// std::cout << "=====收到报单应答=====" << std::endl;

	// if (isMyOrder(pOrder))
	// {
	// 	if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded) {
	// 		std::cout << "--->>> 成交！" << std::endl;
	// 	}
	// 	if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing) {
	// 		std::cout << "--->>> 部分成交！" << std::endl;
	// 		reqOrderAction(pOrder); // 这里可以撤单
	// 		reqUserLogout(); // 登出测试
	// 	}
	// 	if (isTradingOrder(pOrder))
	// 	{
	// 		std::cout << "--->>> 等待成交中！" << std::endl;
	// 	}
	// 	else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
	// 		std::cout << pOrder->StatusMsg << std::endl;
	// }
}

void RealTradeSpi::OnRtnTrade(CThostFtdcTradeField* pTrade)
{
	std::cout << "=====报单成功成交=====" << std::endl;
	std::cout << "成交时间： " << pTrade->TradeTime << std::endl;
	std::cout << "合约代码： " << pTrade->InstrumentID << std::endl;
	std::cout << "成交价格： " << pTrade->Price << std::endl;
	std::cout << "成交量： " << pTrade->Volume << std::endl;
	std::cout << "开平仓方向： " << pTrade->Direction << std::endl;
}

bool RealTradeSpi::isErrorRspInfo(CThostFtdcRspInfoField* pRspInfo)
{
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (bResult)
		std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
	return bResult;
}

// 客户端认证请求
void RealTradeSpi::reqAuthenticate()
{
	CThostFtdcReqAuthenticateField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, gBrokerID);
	strcpy(req.AppID, AppID);
	strcpy(req.AuthCode, AuthCode);
	strcpy(req.UserID, gInvesterID);
	static int requestID = 0;
	int iResult = g_pTradeUserApi->ReqAuthenticate(&req, requestID);
	std::cerr << "---> 发送客户端认证:" << ((iResult == 0) ? "成功" : "失败") << std::endl;
}

// 登出
void RealTradeSpi::reqUserLogout()
{
	CThostFtdcUserLogoutField logoutReq;
	memset(&logoutReq, 0, sizeof(logoutReq));
	strcpy(logoutReq.BrokerID, gBrokerID);
	strcpy(logoutReq.UserID, gInvesterID);
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqUserLogout(&logoutReq, requestID);
	if (!rt)
		std::cout << ">>>>>>交易--发送登出请求成功" << std::endl;
	else
		std::cerr << "--->>>交易--发送登出请求失败" << std::endl;
}

// 投资者结算结果确认
void RealTradeSpi::reqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField settlementConfirmReq;
	memset(&settlementConfirmReq, 0, sizeof(settlementConfirmReq));
	strcpy(settlementConfirmReq.BrokerID, gBrokerID);
	strcpy(settlementConfirmReq.InvestorID, gInvesterID);
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqSettlementInfoConfirm(&settlementConfirmReq, requestID);
	if (!rt)
	{
		std::cout << ">>>>>>发送投资者结算结果确认请求成功" << std::endl;
	}
	else
		std::cerr << "--->>>发送投资者结算结果确认请求失败" << std::endl;
}

// 投资者结算结果查询
void RealTradeSpi::reqQrySettlementInfo()
{
	CThostFtdcQrySettlementInfoField settlementReq;
	memset(&settlementReq, 0, sizeof(settlementReq));
	strcpy(settlementReq.BrokerID, gBrokerID);
	strcpy(settlementReq.InvestorID, gInvesterID);
	strcpy(settlementReq.TradingDay, "20260522");
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqQrySettlementInfo(&settlementReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者结算结果查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者结算结果查询请求失败" << std::endl;
}

// 请求查询资金帐户
void RealTradeSpi::reqQueryTradingAccount()
{
	CThostFtdcQryTradingAccountField tradingAccountReq;
	memset(&tradingAccountReq, 0, sizeof(tradingAccountReq));
	strcpy(tradingAccountReq.BrokerID, gBrokerID);
	strcpy(tradingAccountReq.InvestorID, gInvesterID);
	static int requestID = 0; // 请求编号
	//std::this_thread::sleep_for(std::chrono::milliseconds(700)); // 有时候需要停顿一会才能查询成功
	int rt = g_pTradeUserApi->ReqQryTradingAccount(&tradingAccountReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者资金账户查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者资金账户查询请求失败" << std::endl;
}

void RealTradeSpi::reqQueryInvestorPosition()
{
	CThostFtdcQryInvestorPositionField postionReq;
	memset(&postionReq, 0, sizeof(postionReq));
	strcpy(postionReq.BrokerID, gBrokerID);
	strcpy(postionReq.InvestorID, gInvesterID);
	strcpy(postionReq.InstrumentID, g_pTradeInstrumentID);
	static int requestID = 0; // 请求编号
	//std::this_thread::sleep_for(std::chrono::milliseconds(700)); // 有时候需要停顿一会才能查询成功
	int rt = g_pTradeUserApi->ReqQryInvestorPosition(&postionReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者持仓查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者持仓查询请求失败" << std::endl;
}

static int sendBenchOrder(TThostFtdcInstrumentIDType instrumentID, TThostFtdcPriceType price, TThostFtdcVolumeType volume, char direction, char orderPriceType, char timeCondition, char volumeCondition)
{
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	strcpy(orderInsertReq.BrokerID, gBrokerID);
	strcpy(orderInsertReq.InvestorID, gInvesterID);
	strcpy(orderInsertReq.InstrumentID, instrumentID);
	strcpy(orderInsertReq.OrderRef, order_ref);
	orderInsertReq.OrderPriceType = orderPriceType;
	orderInsertReq.Direction = direction;
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	orderInsertReq.LimitPrice = price;
	orderInsertReq.IsSwapOrder = 0;
	orderInsertReq.VolumeTotalOriginal = volume;
	orderInsertReq.TimeCondition = timeCondition;
	orderInsertReq.VolumeCondition = volumeCondition;
	orderInsertReq.MinVolume = 1;
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	orderInsertReq.IsAutoSuspend = 0;
	orderInsertReq.UserForceClose = 0;
	static int requestID = 0;
	return g_pTradeUserApi->ReqOrderInsert(&orderInsertReq, ++requestID);
}

// 请求报单操作
void RealTradeSpi::reqOrderAction(CThostFtdcOrderField* pOrder)
{
	static bool orderActionSentFlag = false; // 是否发送了报单
	if (orderActionSentFlag)
		return;

	CThostFtdcInputOrderActionField orderActionReq;
	memset(&orderActionReq, 0, sizeof(orderActionReq));
	///经纪公司代码
	strcpy(orderActionReq.BrokerID, pOrder->BrokerID);
	///投资者代码
	strcpy(orderActionReq.InvestorID, pOrder->InvestorID);
	///报单操作引用
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///报单引用
	strcpy(orderActionReq.OrderRef, pOrder->OrderRef);
	///请求编号
	//	TThostFtdcRequestIDType	RequestID;
	///前置编号
	orderActionReq.FrontID = trade_front_id;
	///会话编号
	orderActionReq.SessionID = session_id;
	///交易所代码
	//	TThostFtdcExchangeIDType	ExchangeID;
	///报单编号
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///操作标志
	orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;
	///价格
	//	TThostFtdcPriceType	LimitPrice;
	///数量变化
	//	TThostFtdcVolumeType	VolumeChange;
	///用户代码
	//	TThostFtdcUserIDType	UserID;
	///合约代码
	strcpy(orderActionReq.InstrumentID, pOrder->InstrumentID);
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqOrderAction(&orderActionReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>发送报单操作请求成功" << std::endl;
	else
		std::cerr << "--->>>发送报单操作请求失败" << std::endl;
	orderActionSentFlag = true;
}

// 登陆之后，交易核心会返回对应此次连接的前置机编号 FrontID 和会话编号 SessionID，不变的
// OrderRef 需要注意由于是字符串数组，不能直接比较。
bool RealTradeSpi::isMyOrder(CThostFtdcOrderField* pOrder)
{
	return ((pOrder->FrontID == trade_front_id) &&
		(pOrder->SessionID == session_id) &&
		(strcmp(pOrder->OrderRef, order_ref) == 0));
}

bool RealTradeSpi::isTradingOrder(CThostFtdcOrderField* pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}