%% ��ѯί��
function tbl = ReqQryOrder(td)

SetWait(0);

% ��ѯ
global order_table;
order_table = [];
td.NewQuery();
td.SetQuery('Int32ID',-1);
td.ReqQuery('ReqQryOrder');

% ��Ҫѭ���ȴ�����
Wait();

% ����
tbl = order_table;

end