%% ��ѯ�ֲ�
function tbl = ReqQryInvestorPosition(td)

SetWait(0);

% ��ѯ
global position_table;
position_table = [];
td.NewQuery();
td.SetQuery('Int32ID',-1);
td.ReqQuery('ReqQryInvestorPosition');

% ��Ҫѭ���ȴ�����
Wait();

% ����
tbl = position_table;

end