#include "stdafx.h"
#include "QueryApi.h"

#include "../../include/QueueEnum.h"
#include "../../include/QueueHeader.h"

#include "../../include/ApiHeader.h"
#include "../../include/ApiStruct.h"

#include "../../include/toolkit.h"

#include "../../common/Queue/MsgQueue.h"

// �����õ����뽻��ͬһ������ת��
#include "../LTS_Trade_v2/TypeConvert.h"

#include <cstring>
#include <assert.h>

void* __stdcall Query_Q(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	// ���ڲ����ã����ü���Ƿ�Ϊ��
	CQueryApi* pApi = (CQueryApi*)pApi2;
	pApi->QueryInThread(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
	return nullptr;
}

void CQueryApi::QueryInThread(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	int iRet = 0;
	switch (type)
	{
	case E_Init:
		iRet = _Init();
		break;
	case E_ReqUserLoginField:
		iRet = _ReqUserLogin(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case QueryType::QueryType_ReqQryTradingAccount:
		iRet = _ReqQryTradingAccount(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case QueryType::QueryType_ReqQryInvestorPosition:
		iRet = _ReqQryInvestorPosition(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case QueryType::QueryType_ReqQryInstrument:
		iRet = _ReqQryInstrument(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case QueryType::QueryType_ReqQryInvestor:
		iRet = _ReqQryInvestor(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case QueryType::QueryType_ReqQryOrder:
		iRet = _ReqQryOrder(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case QueryType::QueryType_ReqQryTrade:
		iRet = _ReqQryTrade(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	case E_AuthRandCodeField:
		iRet = _ReqFetchAuthRandCode(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		break;
	default:
		break;
	}

	if (0 == iRet)
	{
		//���سɹ�����ӵ��ѷ��ͳ�
		m_nSleep = 1;
	}
	else
	{
		m_msgQueue_Query->Input_Copy(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		//ʧ�ܣ���4���ݽ�����ʱ����������1s
		m_nSleep *= 4;
		m_nSleep %= 1023;
	}
	this_thread::sleep_for(chrono::milliseconds(m_nSleep));
}

void CQueryApi::Register(void* pCallback, void* pClass)
{
	m_pClass = pClass;
	if (m_msgQueue == nullptr)
		return;

	m_msgQueue_Query->Register((void*)Query_Q, this);
	m_msgQueue->Register(pCallback, this);
	if (pCallback)
	{
		m_msgQueue_Query->StartThread();
		m_msgQueue->StartThread();
	}
	else
	{
		m_msgQueue_Query->StopThread();
		m_msgQueue->StopThread();
	}
}

CQueryApi::CQueryApi(void)
{
	m_pApi = nullptr;
	m_lRequestID = 0;
	m_nSleep = 1;

	// �Լ�ά��������Ϣ����
	m_msgQueue = new CMsgQueue();
	m_msgQueue_Query = new CMsgQueue();

	m_msgQueue_Query->Register((void*)Query_Q, this);
	m_msgQueue_Query->StartThread();
}


CQueryApi::~CQueryApi(void)
{
	Disconnect();
}

bool CQueryApi::IsErrorRspInfo(const char* szSource, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bRet)
	{
		ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));

		pField->RawErrorID = pRspInfo->ErrorID;
		strcpy(pField->Text, pRspInfo->ErrorMsg);
		strcpy(pField->Source, szSource);

		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnRtnError, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
	}
	return bRet;
}

bool CQueryApi::IsErrorRspInfo(CSecurityFtdcRspInfoField *pRspInfo)
{
	bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));

	return bRet;
}

void CQueryApi::Connect(const string& szPath,
	ServerInfoField* pServerInfo,
	UserInfoField* pUserInfo)
{
	m_szPath = szPath;
	memcpy(&m_ServerInfo, pServerInfo, sizeof(ServerInfoField));
	memcpy(&m_UserInfo, pUserInfo, sizeof(UserInfoField));

	m_msgQueue_Query->Input_NoCopy(RequestType::E_Init, m_msgQueue_Query, this, 0, 0,
		nullptr, 0, nullptr, 0, nullptr, 0);
}

int CQueryApi::_Init()
{
	char *pszPath = new char[m_szPath.length() + 1024];
	srand((unsigned int)time(nullptr));
	sprintf(pszPath, "%s/%s/%s/Qr/%d/", m_szPath.c_str(), m_ServerInfo.BrokerID, m_UserInfo.UserID, rand());
	makedirs(pszPath);

	m_pApi = CSecurityFtdcQueryApi::CreateFtdcQueryApi(pszPath);
	delete[] pszPath;

	m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Initialized, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	if (m_pApi)
	{
		m_pApi->RegisterSpi(this);

		//��ӵ�ַ
		size_t len = strlen(m_ServerInfo.Address) + 1;
		char* buf = new char[len];
		strncpy(buf, m_ServerInfo.Address, len);

		char* token = strtok(buf, _QUANTBOX_SEPS_);
		while (token)
		{
			if (strlen(token)>0)
			{
				m_pApi->RegisterFront(token);
			}
			token = strtok(nullptr, _QUANTBOX_SEPS_);
		}
		delete[] buf;

		//������ţ������Ǵ�ͷ��ʼ�����ˣ���������Ҫ�ر���һ��
		//if (m_ServerInfo.PublicTopicResumeType < ResumeType::ResumeType_Undefined)
		//{
		//	m_pApi->SubscribePublicTopic((THOST_TE_RESUME_TYPE)m_ServerInfo.PublicTopicResumeType);
		//}
		//else
		//{
		//	m_pApi->SubscribePublicTopic(THOST_TERT_QUICK);
		//}

		//if (m_ServerInfo.PrivateTopicResumeType < ResumeType::ResumeType_Undefined)
		//{
		//	m_pApi->SubscribePrivateTopic((THOST_TE_RESUME_TYPE)m_ServerInfo.PrivateTopicResumeType);
		//}
		//else
		//{
		//	m_pApi->SubscribePrivateTopic(THOST_TERT_QUICK);
		//}

		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Connecting, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		//��ʼ������
		m_pApi->Init();
	}

	return 0;
}

void CQueryApi::Disconnect()
{
	if (m_msgQueue_Query)
	{
		m_msgQueue_Query->StopThread();
		m_msgQueue_Query->Register(nullptr, nullptr);
		m_msgQueue_Query->Clear();
		delete m_msgQueue_Query;
		m_msgQueue_Query = nullptr;
	}

	if (m_pApi)
	{
		m_pApi->RegisterSpi(nullptr);
		m_pApi->Release();
		m_pApi = nullptr;

		// ȫ����ֻ�����һ��
		m_msgQueue->Clear();
		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Disconnected, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		// ��������
		m_msgQueue->Process();
	}

	if (m_msgQueue)
	{
		m_msgQueue->StopThread();
		m_msgQueue->Register(nullptr, nullptr);
		m_msgQueue->Clear();
		delete m_msgQueue;
		m_msgQueue = nullptr;
	}

	m_lRequestID = 0;//�����߳��Ѿ�ֹͣ��û�б�Ҫ��ԭ�Ӳ�����
	
	Clear();
}

void CQueryApi::Clear()
{
	for (unordered_map<string, OrderField*>::iterator it = m_id_platform_order.begin(); it != m_id_platform_order.end(); ++it)
		delete it->second;
	m_id_platform_order.clear();

	for (unordered_map<string, CSecurityFtdcOrderField*>::iterator it = m_id_api_order.begin(); it != m_id_api_order.end(); ++it)
		delete it->second;
	m_id_api_order.clear();

	//for (unordered_map<string, QuoteField*>::iterator it = m_id_platform_quote.begin(); it != m_id_platform_quote.end(); ++it)
	//	delete it->second;
	//m_id_platform_quote.clear();

	//for (unordered_map<string, CThostFtdcQuoteField*>::iterator it = m_id_api_quote.begin(); it != m_id_api_quote.end(); ++it)
	//	delete it->second;
	//m_id_api_quote.clear();

	for (unordered_map<string, PositionField*>::iterator it = m_id_platform_position.begin(); it != m_id_platform_position.end(); ++it)
		delete it->second;
	m_id_platform_position.clear();
}

void CQueryApi::OnFrontConnected()
{
	m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Connected, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	//ReqUserLogin();
	ReqFetchAuthRandCode();
}

void CQueryApi::OnFrontDisconnected(int nReason)
{
	RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

	//����ʧ�ܷ��ص���Ϣ��ƴ�Ӷ��ɣ���Ҫ��Ϊ��ͳһ���
	pField->RawErrorID = nReason;
	GetOnFrontDisconnectedMsg(nReason, pField->Text);

	m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
}

void CQueryApi::ReqFetchAuthRandCode()
{
	CSecurityFtdcAuthRandCodeField* pBody = (CSecurityFtdcAuthRandCodeField*)m_msgQueue_Query->new_block(sizeof(CSecurityFtdcAuthRandCodeField));

	strncpy(pBody->RandCode, "", sizeof(TSecurityFtdcAuthCodeType));

	m_msgQueue_Query->Input_NoCopy(RequestType::E_AuthRandCodeField, m_msgQueue_Query, this, 0, 0,
		pBody, sizeof(CSecurityFtdcAuthRandCodeField), nullptr, 0, nullptr, 0);
}

int CQueryApi::_ReqFetchAuthRandCode(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Authorizing, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	return m_pApi->ReqFetchAuthRandCode((CSecurityFtdcAuthRandCodeField*)ptr1, ++m_lRequestID);
}

void CQueryApi::OnRspFetchAuthRandCode(CSecurityFtdcAuthRandCodeField *pAuthRandCode, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

	if (!IsErrorRspInfo(pRspInfo)
		&& pAuthRandCode)
	{
		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Authorized, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);

		ReqUserLogin(pAuthRandCode->RandCode);
	}
	else
	{
		pField->RawErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(Char256Type));

		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}
}

void CQueryApi::ReqUserLogin(TSecurityFtdcAuthCodeType	RandCode)
{
	CSecurityFtdcReqUserLoginField* pBody = (CSecurityFtdcReqUserLoginField*)m_msgQueue_Query->new_block(sizeof(CSecurityFtdcReqUserLoginField));

	strncpy(pBody->BrokerID, m_ServerInfo.BrokerID, sizeof(TSecurityFtdcBrokerIDType));
	strncpy(pBody->UserID, m_UserInfo.UserID, sizeof(TSecurityFtdcInvestorIDType));
	strncpy(pBody->Password, m_UserInfo.Password, sizeof(TSecurityFtdcPasswordType));
	strncpy(pBody->UserProductInfo, m_ServerInfo.UserProductInfo, sizeof(TSecurityFtdcProductInfoType));
	strncpy(pBody->AuthCode, m_ServerInfo.AuthCode, sizeof(TSecurityFtdcAuthCodeType));
	strncpy(pBody->RandCode, RandCode, sizeof(TSecurityFtdcAuthCodeType));

	m_msgQueue_Query->Input_NoCopy(RequestType::E_ReqUserLoginField, m_msgQueue_Query, this, 0, 0,
		pBody, sizeof(CSecurityFtdcReqUserLoginField), nullptr, 0, nullptr, 0);
}

int CQueryApi::_ReqUserLogin(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Logining, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	return m_pApi->ReqUserLogin((CSecurityFtdcReqUserLoginField*)ptr1, ++m_lRequestID);
}

void CQueryApi::OnRspUserLogin(CSecurityFtdcRspUserLoginField *pRspUserLogin, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

	if (!IsErrorRspInfo(pRspInfo)
		&&pRspUserLogin)
	{
		pField->TradingDay = GetDate(pRspUserLogin->TradingDay);
		pField->LoginTime = GetTime(pRspUserLogin->LoginTime);

		sprintf(pField->SessionID, "%d:%d", pRspUserLogin->FrontID, pRspUserLogin->SessionID);

		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Logined, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);

		// ���µ�¼��Ϣ�����ܻ��õ�
		memcpy(&m_RspUserLogin,pRspUserLogin,sizeof(CSecurityFtdcRspUserLoginField));
		m_nMaxOrderRef = atol(pRspUserLogin->MaxOrderRef);
		// �Լ�����ʱID��1��ʼ�����ܴ�0��ʼ
		m_nMaxOrderRef = m_nMaxOrderRef>1 ? m_nMaxOrderRef : 1;
		//ReqSettlementInfoConfirm();
		ReqQueryField body = { 0 };
		ReqQuery(QueryType::QueryType_ReqQryInvestor, &body);

		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Done, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		if (m_ServerInfo.PrivateTopicResumeType > ResumeType::ResumeType_Restart
			&& (m_ServerInfo.PrivateTopicResumeType<ResumeType::ResumeType_Undefined))
		{
			ReqQueryField body = { 0 };
			ReqQuery(QueryType::QueryType_ReqQryOrder, &body);
			//ReqQryTrade();
			//ReqQryQuote();
		}
	}
	else
	{
		pField->RawErrorID = pRspInfo->ErrorID;
		strncpy(pField->Text, pRspInfo->ErrorMsg, sizeof(Char256Type));

		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
	}
}

void CQueryApi::ReqQuery(QueryType type, ReqQueryField* pQuery)
{
	m_msgQueue_Query->Input_Copy(type, m_msgQueue_Query, this, 0, 0,
		pQuery, sizeof(ReqQueryField), nullptr, 0, nullptr, 0);
}

int CQueryApi::_ReqQryTradingAccount(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	CSecurityFtdcQryTradingAccountField body = { 0 };

	strcpy(body.BrokerID, m_RspUserLogin.BrokerID);
	strcpy(body.InvestorID, m_RspUserLogin.UserID);

	return m_pApi->ReqQryTradingAccount(&body, ++m_lRequestID);
}

void CQueryApi::OnRspQryTradingAccount(CSecurityFtdcTradingAccountField *pTradingAccount, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo("OnRspQryTradingAccount", pRspInfo, nRequestID, bIsLast))
	{
		if (pTradingAccount)
		{
			AccountField* pField = (AccountField*)m_msgQueue->new_block(sizeof(AccountField));

			strcpy(pField->AccountID, pTradingAccount->AccountID);
			pField->PreBalance = pTradingAccount->PreBalance;
			pField->CurrMargin = pTradingAccount->CurrMargin;
			//pField->CloseProfit = pTradingAccount->CloseProfit;
			//pField->PositionProfit = pTradingAccount->PositionProfit;
			pField->Balance = pTradingAccount->Balance;
			pField->Available = pTradingAccount->Available;
			pField->Deposit = pTradingAccount->Deposit;
			pField->Withdraw = pTradingAccount->Withdraw;
			pField->FrozenTransferFee = pTradingAccount->FrozenTransferFee;
			pField->FrozenStampTax = pTradingAccount->FrozenStampTax;
			pField->FrozenCommission = pTradingAccount->FrozenCommission;
			pField->FrozenCash = pTradingAccount->FrozenCash;
			pField->TransferFee = pTradingAccount->TransferFee;
			pField->StampTax = pTradingAccount->StampTax;
			pField->Commission = pTradingAccount->Commission;
			pField->CashIn = pTradingAccount->CashIn;

			m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnRspQryTradingAccount, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(AccountField), nullptr, 0, nullptr, 0);
		}
		else
		{
			m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnRspQryTradingAccount, m_msgQueue, m_pClass, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}
}

int CQueryApi::_ReqQryInvestorPosition(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	ReqQueryField* pQuery = (ReqQueryField*)ptr1;

	CSecurityFtdcQryInvestorPositionField body = { 0 };

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID, sizeof(TSecurityFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID, sizeof(TSecurityFtdcInvestorIDType));
	strncpy(body.InstrumentID, pQuery->InstrumentID, sizeof(TSecurityFtdcInstrumentIDType));

	return m_pApi->ReqQryInvestorPosition(&body, ++m_lRequestID);
}

// ��ծ��ع����ֲ���Net���ҹ��ൽ��Long�У������ֱ�Ӵӳɽ��з����������ֹ鵽��Net��
// ���п�ƽ��Ͷ�����Ͷ��ǿ�
void CQueryApi::OnRspQryInvestorPosition(CSecurityFtdcInvestorPositionField *pInvestorPosition, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo("OnRspQryInvestorPosition", pRspInfo, nRequestID, bIsLast))
	{
		if (pInvestorPosition)
		{
			PositionIDType positionId = { 0 };
			sprintf(positionId, "%s:%s:%d:%d",
				pInvestorPosition->InstrumentID, pInvestorPosition->ExchangeID,
				TSecurityFtdcPosiDirectionType_2_PositionSide(pInvestorPosition->PosiDirection), TSecurityFtdcHedgeFlagType_2_HedgeFlagType(pInvestorPosition->HedgeFlag));

			PositionField* pField = nullptr;
			unordered_map<string, PositionField*>::iterator it = m_id_platform_position.find(positionId);
			if (it == m_id_platform_position.end())
			{
				pField = (PositionField*)m_msgQueue->new_block(sizeof(PositionField));

				sprintf(pField->Symbol, "%s.%s", pInvestorPosition->InstrumentID, pInvestorPosition->ExchangeID);
				strcpy(pField->InstrumentID, pInvestorPosition->InstrumentID);
				strcpy(pField->ExchangeID, pInvestorPosition->ExchangeID);
				strcpy(pField->AccountID, pInvestorPosition->AccountID);
				pField->Side = TSecurityFtdcPosiDirectionType_2_PositionSide(pInvestorPosition->PosiDirection);
				pField->HedgeFlag = TSecurityFtdcHedgeFlagType_2_HedgeFlagType(pInvestorPosition->HedgeFlag);

				m_id_platform_position.insert(pair<string, PositionField*>(positionId, pField));
			}
			else
			{
				pField = it->second;
			}

			pField->Position = pInvestorPosition->Position;
			pField->TodayPosition = pInvestorPosition->TodayPosition;
			pField->HistoryPosition = pInvestorPosition->YdPosition;

			// �������ռ�ȫ���ٱ���֪ͨһ��
			if (bIsLast)
			{
				int cnt = 0;
				int count = m_id_platform_position.size();
				for (unordered_map<string, PositionField*>::iterator iter = m_id_platform_position.begin(); iter != m_id_platform_position.end(); iter++)
				{
					++cnt;
					m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRspQryInvestorPosition, m_msgQueue, m_pClass, cnt == count, 0, iter->second, sizeof(PositionField), nullptr, 0, nullptr, 0);
				}
			}
		}
	}
}

int CQueryApi::_ReqQryInstrument(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	ReqQueryField* pQuery = (ReqQueryField*)ptr1;

	CSecurityFtdcQryInstrumentField body = { 0 };
	strncpy(body.InstrumentID, pQuery->InstrumentID, sizeof(TSecurityFtdcInstrumentIDType));
	strncpy(body.ExchangeID, pQuery->ExchangeID, sizeof(TSecurityFtdcExchangeIDType));

	return m_pApi->ReqQryInstrument(&body, ++m_lRequestID);
}

void CQueryApi::OnRspQryInstrument(CSecurityFtdcInstrumentField *pInstrument, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo("OnRspQryInstrument", pRspInfo, nRequestID, bIsLast))
	{
		if (pInstrument)
		{
			InstrumentField* pField = (InstrumentField*)m_msgQueue->new_block(sizeof(InstrumentField));

			strncpy(pField->InstrumentID, pInstrument->InstrumentID, sizeof(InstrumentIDType));
			strncpy(pField->ExchangeID, pInstrument->ExchangeID, sizeof(ExchangeIDType));

			sprintf(pField->Symbol,"%s.%s",pInstrument->InstrumentID,pInstrument->ExchangeID);
			sprintf(pField->InstrumentName, "%s(%s)", pInstrument->ExchangeInstID, pInstrument->InstrumentName);
			strncpy(pField->ProductID, pInstrument->ProductID, sizeof(InstrumentIDType));

			//strncpy(pField->InstrumentName, pInstrument->InstrumentName, sizeof(InstrumentNameType));
			pField->Type = CSecurityFtdcInstrumentField_2_InstrumentType(pInstrument);
			pField->VolumeMultiple = pInstrument->VolumeMultiple;
			pField->PriceTick = CSecurityFtdcInstrumentField_2_PriceTick(pInstrument);
			pField->ExpireDate = GetDate(pInstrument->ExpireDate);
			pField->OptionsType = CSecurityFtdcInstrumentField_2_PutCall(pInstrument);
			pField->StrikePrice = pInstrument->ExecPrice;


			// ��Ȩ�ı����
			if (strlen(pInstrument->InstrumentID) == 8)
			{
				strncpy(pField->UnderlyingInstrID, pInstrument->ExchangeInstID, 6);
				sprintf(&pField->UnderlyingInstrID[6], ".%s", pInstrument->ExchangeID);
			}


			m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnRspQryInstrument, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(InstrumentField), nullptr, 0, nullptr, 0);
		}
		else
		{
			m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnRspQryInstrument, m_msgQueue, m_pClass, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}
}

void CQueryApi::OnRspError(CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	IsErrorRspInfo("OnRspError", pRspInfo, nRequestID, bIsLast);
}

int CQueryApi::_ReqQryOrder(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	CSecurityFtdcQryOrderField body = { 0 };

	strcpy(body.BrokerID, m_RspUserLogin.BrokerID);
	strcpy(body.InvestorID, m_RspUserLogin.UserID);

	return m_pApi->ReqQryOrder(&body, ++m_lRequestID);
}

void CQueryApi::OnOrder(CSecurityFtdcOrderField *pOrder, int nRequestID, bool bIsLast)
{
	if (nullptr == pOrder)
		return;

	OrderIDType orderId = { 0 };
	sprintf(orderId, "%d:%d:%s", pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef);
	OrderIDType orderSydId = { 0 };

	{
		// ����ԭʼ������Ϣ�����ڳ���

		unordered_map<string, CSecurityFtdcOrderField*>::iterator it = m_id_api_order.find(orderId);
		if (it == m_id_api_order.end())
		{
			// �Ҳ����˶�������ʾ���µ�
			CSecurityFtdcOrderField* pField = new CSecurityFtdcOrderField();
			memcpy(pField, pOrder, sizeof(CSecurityFtdcOrderField));
			m_id_api_order.insert(pair<string, CSecurityFtdcOrderField*>(orderId, pField));
		}
		else
		{
			// �ҵ��˶���
			// ��Ҫ�ٸ��Ʊ������һ�ε�״̬������ֻҪ��һ�ε����ڳ������ɣ����£��������ñȽ�
			CSecurityFtdcOrderField* pField = it->second;
			memcpy(pField, pOrder, sizeof(CSecurityFtdcOrderField));
		}

		// ����SysID���ڶ���ɽ��ر��붩��
		sprintf(orderSydId, "%s:%s", pOrder->ExchangeID, pOrder->OrderSysID);
		m_sysId_orderId.insert(pair<string, string>(orderSydId, orderId));
	}

	{
		// ��API�Ķ���ת�����Լ��Ľṹ��
		if (nRequestID == 0)
		{
			OrderField* pField = nullptr;
			unordered_map<string, OrderField*>::iterator it = m_id_platform_order.find(orderId);
			if (it == m_id_platform_order.end())
			{
				// ����ʱ������Ϣ��û�У������Ҳ�����Ӧ�ĵ��ӣ���Ҫ����Order�Ļָ�
				pField = (OrderField*)m_msgQueue->new_block(sizeof(OrderField));

				// ����ʱ������Ϣ��û�У������Ҳ�����Ӧ�ĵ��ӣ���Ҫ����Order�Ļָ�
				CSecurityFtdcOrderField_2_OrderField_0(orderId, pOrder, pField);


				// ��ӵ�map�У������������ߵĶ�ȡ������ʧ��ʱ����֪ͨ��
				m_id_platform_order.insert(pair<string, OrderField*>(orderId, pField));
			}
			else
			{
				pField = it->second;
				strcpy(pField->ID, orderId);
				strcpy(pField->LocalID, pField->ID);
				pField->LeavesQty = pOrder->VolumeTotal;
				pField->Price = atof(pOrder->LimitPrice);
				pField->Status = CSecurityFtdcOrderField_2_OrderStatus(pOrder);
				pField->ExecType = CSecurityFtdcOrderField_2_ExecType(pOrder);
				strcpy(pField->OrderID, pOrder->OrderSysID);
				strncpy(pField->Text, pOrder->StatusMsg, sizeof(Char256Type));
			}

			m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRtnOrder, m_msgQueue, m_pClass, 0, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);
		}
		else
		{
			OrderField* pField = nullptr;
			pField = (OrderField*)m_msgQueue->new_block(sizeof(OrderField));

			// ����ʱ������Ϣ��û�У������Ҳ�����Ӧ�ĵ��ӣ���Ҫ����Order�Ļָ�
			CSecurityFtdcOrderField_2_OrderField_0(orderId, pOrder, pField);

			// ��ӵ�map�У������������ߵĶ�ȡ������ʧ��ʱ����֪ͨ��
			//m_id_platform_order.insert(pair<string, OrderField*>(orderId, pField));

			m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRspQryOrder, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(OrderField), nullptr, 0, nullptr, 0);

			m_msgQueue->delete_block(pField);
		}
		
	}
}

void CQueryApi::OnRspQryOrder(CSecurityFtdcOrderField *pOrder, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo("OnRspQryOrder", pRspInfo, nRequestID, bIsLast))
	{
		OnOrder(pOrder, nRequestID, bIsLast);
	}
}

//void CQueryApi::ReqQryTrade()
//{
//	CSecurityFtdcQryTradeField* pBody = (CSecurityFtdcQryTradeField*)m_msgQueue_Query->new_block(sizeof(CSecurityFtdcQryTradeField));
//
//	strcpy(pBody->BrokerID, m_RspUserLogin.BrokerID);
//	strcpy(pBody->InvestorID, m_RspUserLogin.UserID);
//
//	m_msgQueue_Query->Input_NoCopy(RequestType::E_QryTradeField, m_msgQueue_Query, this, 0, 0,
//		pBody, sizeof(CSecurityFtdcQryTradeField), nullptr, 0, nullptr, 0);
//}

int CQueryApi::_ReqQryTrade(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	CSecurityFtdcQryTradeField body = { 0 };

	strcpy(body.BrokerID, m_RspUserLogin.BrokerID);
	strcpy(body.InvestorID, m_RspUserLogin.UserID);

	return m_pApi->ReqQryTrade(&body, ++m_lRequestID);
}

void CQueryApi::OnTrade(CSecurityFtdcTradeField *pTrade, int nRequestID, bool bIsLast)
{
	if (nullptr == pTrade)
		return;

	TradeField* pField = (TradeField*)m_msgQueue->new_block(sizeof(TradeField));
	strcpy(pField->InstrumentID, pTrade->InstrumentID);
	strcpy(pField->ExchangeID, pTrade->ExchangeID);
	pField->Side = TSecurityFtdcDirectionType_2_OrderSide(pTrade->Direction);
	pField->Qty = pTrade->Volume;
	pField->Price = atof(pTrade->Price);
	pField->OpenClose = TSecurityFtdcOffsetFlagType_2_OpenCloseType(pTrade->OffsetFlag);
	pField->HedgeFlag = TSecurityFtdcHedgeFlagType_2_HedgeFlagType(pTrade->HedgeFlag);
	pField->Commission = 0;//TODO�������Ժ�Ҫ�������
	pField->Time = GetTime(pTrade->TradeTime);
	strcpy(pField->TradeID, pTrade->TradeID);

	if (nRequestID == 0)
	{
		OrderIDType orderSysId = { 0 };
		sprintf(orderSysId, "%s:%s", pTrade->ExchangeID, pTrade->OrderSysID);
		unordered_map<string, string>::iterator it = m_sysId_orderId.find(orderSysId);
		if (it == m_sysId_orderId.end())
		{
			// �˳ɽ��Ҳ�����Ӧ�ı���
			//assert(false);
		}
		else
		{
			// �ҵ���Ӧ�ı���
			strcpy(pField->ID, it->second.c_str());

			m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRtnTrade, m_msgQueue, m_pClass, 0, 0, pField, sizeof(TradeField), nullptr, 0, nullptr, 0);

			unordered_map<string, OrderField*>::iterator it2 = m_id_platform_order.find(it->second);
			if (it2 == m_id_platform_order.end())
			{
				// �˳ɽ��Ҳ�����Ӧ�ı���
				//assert(false);
			}
			else
			{
				// ���¶�����״̬
				// �Ƿ�Ҫ֪ͨ�ӿ�
			}

			OnTrade(pField);
		}
	}
	else
	{
		m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRspQryTrade, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(TradeField), nullptr, 0, nullptr, 0);
	}
	
	// �����ڴ�
	m_msgQueue->delete_block(pField);
}

void CQueryApi::OnTrade(TradeField *pTrade)
{
	PositionIDType positionId = { 0 };
	sprintf(positionId, "%s:%s:%d:%d",
		pTrade->InstrumentID, pTrade->ExchangeID, TradeField_2_PositionSide(pTrade), pTrade->HedgeFlag);

	PositionField* pField = nullptr;
	unordered_map<string, PositionField*>::iterator it = m_id_platform_position.find(positionId);
	if (it == m_id_platform_position.end())
	{
		pField = (PositionField*)m_msgQueue->new_block(sizeof(PositionField));

		sprintf(pField->Symbol, "%s.%s", pTrade->InstrumentID, pTrade->ExchangeID);
		strcpy(pField->InstrumentID, pTrade->InstrumentID);
		strcpy(pField->ExchangeID, pTrade->ExchangeID);
		pField->Side = TradeField_2_PositionSide(pTrade);
		pField->HedgeFlag = TSecurityFtdcHedgeFlagType_2_HedgeFlagType(pTrade->HedgeFlag);

		m_id_platform_position.insert(pair<string, PositionField*>(positionId, pField));
	}
	else
	{
		pField = it->second;
	}

	if (pTrade->Side == OrderSide::OrderSide_Buy)
	{
		pField->Position += pTrade->Qty;
		pField->TodayPosition += pTrade->Qty;
	}
	else
	{
		pField->Position -= pTrade->Qty;
		if (pTrade->OpenClose == OpenCloseType::OpenCloseType_CloseToday)
		{
			pField->TodayPosition -= pTrade->Qty;
		}
		else
		{
			pField->HistoryPosition -= pTrade->Qty;
			// �������ı����ɸ������ӽ��쿪ʼ������
			if (pField->HistoryPosition<0)
			{
				pField->TodayPosition += pField->HistoryPosition;
				pField->HistoryPosition = 0;
			}
		}

		// �������ֱ�����²�ѯ
		if (pField->Position < 0 || pField->TodayPosition < 0 || pField->HistoryPosition < 0)
		{
			ReqQueryField body = { 0 };
			ReqQuery(QueryType::QueryType_ReqQryInvestorPosition, &body);
			return;
		}
	}

	m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRspQryInvestorPosition, m_msgQueue, m_pClass, false, 0, pField, sizeof(PositionField), nullptr, 0, nullptr, 0);
}

void CQueryApi::OnRspQryTrade(CSecurityFtdcTradeField *pTrade, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo("OnRspQryTrade", pRspInfo, nRequestID, bIsLast))
	{
		OnTrade(pTrade, nRequestID, bIsLast);
	}
}

//void CQueryApi::ReqQryInvestor()
//{
//	CSecurityFtdcQryInvestorField* pBody = (CSecurityFtdcQryInvestorField*)m_msgQueue_Query->new_block(sizeof(CSecurityFtdcQryInvestorField));
//
//	strncpy(pBody->BrokerID, m_RspUserLogin.BrokerID, sizeof(TSecurityFtdcBrokerIDType));
//	strncpy(pBody->InvestorID, m_RspUserLogin.UserID, sizeof(TSecurityFtdcInvestorIDType));
//
//	m_msgQueue_Query->Input_NoCopy(RequestType::E_QryInvestorField, m_msgQueue_Query, this, 0, 0,
//		pBody, sizeof(CSecurityFtdcQryInvestorField), nullptr, 0, nullptr, 0);
//}

int CQueryApi::_ReqQryInvestor(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	CSecurityFtdcQryInvestorField body = { 0 };

	strcpy(body.BrokerID, m_RspUserLogin.BrokerID);
	strcpy(body.InvestorID, m_RspUserLogin.UserID);

	return m_pApi->ReqQryInvestor(&body, ++m_lRequestID);
}

void CQueryApi::OnRspQryInvestor(CSecurityFtdcInvestorField *pInvestor, CSecurityFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo("OnRspQryInvestor", pRspInfo, nRequestID, bIsLast))
	{
		if (pInvestor)
		{
			memcpy(&m_Investor, pInvestor, sizeof(CSecurityFtdcInvestorField));

			InvestorField* pField = (InvestorField*)m_msgQueue->new_block(sizeof(InvestorField));
			
			strcpy(pField->BrokerID, pInvestor->BrokerID);
			strcpy(pField->InvestorID, pInvestor->InvestorID);
			strcpy(pField->InvestorName, pInvestor->InvestorName);
			strcpy(pField->IdentifiedCardNo, pInvestor->IdentifiedCardNo);
			pField->IdentifiedCardType = TSecurityFtdcIdCardTypeType_2_IdCardType(pInvestor->IdentifiedCardType);

			m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnRspQryInvestor, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(InvestorField), nullptr, 0, nullptr, 0);
		}
		else
		{
			m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnRspQryInvestor, m_msgQueue, m_pClass, bIsLast, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		}
	}
}
