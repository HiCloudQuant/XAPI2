function OnTdConnectionStatus(varargin)

% varargin��ÿ��cell����˼��ο�doc�е�COM Event Handlers
event_struct = varargin{1,end-1};
sender = event_struct.sender;
status = event_struct.status_String;

disp(status);

if strcmp(status,'Done')
	
end

end