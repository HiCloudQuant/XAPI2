%% ����
td = Tdx_Connect();

% ��Ҫѭ���ȴ�����
SetWait(0);
Wait();
return

%% һ��Ҫ�ȴ���¼�ɹ�����
positions = ReqQryInvestorPosition(td)
orders = ReqQryOrder(td)
return;

BuyLimit(td,'600000', 100, 16.4);

td.CancelOrder('20161024:A499180558:1166')
