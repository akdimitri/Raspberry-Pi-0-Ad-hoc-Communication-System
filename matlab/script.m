%% CLEAN UP
clc
clear
%% REDIRECT TO FILE
% TO REDIRECT OUTPUT TO FILE EXECUTE THE FOLLOWING FROM COMMAND LINE
% Turn on recording of the command window output
% diary('outfile.txt');
% 
% script;
%
% Turn recording back off
% diary off;

%% READ FROM FILE
fprintf("READING FROM FILE...");
lines = cell(0,1);  %initialize array to hold strings

% ENTER PATH OF LOG FILE HERE
path = '../results/8462_1h/log.txt';
fileID = fopen(path,'r');
fprintf("FROM PATH %s ", path);

line = fgetl(fileID);
while ischar(line)
    lines{end+1,1} = line;
    line = fgetl(fileID);
end

fclose(fileID);
clearvars fileID line;
lines = string(lines);
fprintf("DONE\n");

%% MESSAGE GENERATOR
fprintf("-----------------\n");
count = 0;

for i = 1:length(lines)
    index = regexp(lines(i),'MESSAGE GENERATOR:*');
    if ~(isempty(index))
        count = count + 1;
        %display(lines(i));
        time = regexp( lines(i), '\d{10}', 'match');
        MESSAGE_GENERATOR_TIMESTAMP(count) = double(time);
    end
end
MESSAGES_GENERATED = count;

fprintf("MESSAGES GENERATED %d\n", MESSAGES_GENERATED);

clearvars count index time i; 

%% SERVER
fprintf("-----------------\n");
SERVER_GRANTED_COMMUNICATION = 0;
SERVER_NEW_MESSAGE_RECEIVED = 0;
SERVER_MESSAGE_MET = 0;
SERVER_TIMEOUT = 0;
SERVER_ERROR = 0;
SERVER_CLOSED_SUCCESSFULLY = 0;
SERVER_ARRIVED_DESTINATION = 0;

for i = 1:length(lines)
    index = regexp(lines(i),'^SERVER:*');
    if ~(isempty(index))
        granted = regexp(lines(i), 'GRANTED COMMUNICATION', 'match');
        if ~(isempty(granted))
             %disp(lines(i));
             SERVER_GRANTED_COMMUNICATION = SERVER_GRANTED_COMMUNICATION + 1;
        end
        new = regexp(lines(i), 'NEW MESSAGE RECEIVED', 'match');
        if ~(isempty(new))
            %disp(lines(i));
            SERVER_NEW_MESSAGE_RECEIVED = SERVER_NEW_MESSAGE_RECEIVED + 1;
        end
        
        met = regexp(lines(i), 'MESSAGE HAS ALREADY BEEN MET', 'match');
        if ~(isempty(met))
            %disp(lines(i));
            SERVER_MESSAGE_MET = SERVER_MESSAGE_MET + 1;
        end
        
        timeout = regexp(lines(i), 'TIMEOUT', 'match');
        if ~(isempty(timeout))
            %disp(lines(i));
            SERVER_TIMEOUT = SERVER_TIMEOUT + 1;
        end
        
        error = regexp(lines(i), 'ERROR', 'match');
        if ~(isempty(error))
            %disp(lines(i));
            SERVER_ERROR = SERVER_ERROR + 1;
        end
        
        suc = regexp(lines(i), 'CLOSED SUCCESSFULLY', 'match');
        if ~(isempty(suc))
            %disp(lines(i));
            SERVER_CLOSED_SUCCESSFULLY = SERVER_CLOSED_SUCCESSFULLY + 1;
        end
        
        dest = regexp(lines(i), 'MESSAGE ARRIVED AT ITS DESTINATION', 'match');
        if ~(isempty(dest))
            disp(lines(i));
            SERVER_ARRIVED_DESTINATION = SERVER_ARRIVED_DESTINATION + 1;
        end
    
    end
end

clearvars index granted i ans new met timeout suc dest error;
fprintf("A FOREIGN DEVICE GRANTED COMMUNICATION %d TIMES\n", SERVER_GRANTED_COMMUNICATION);
fprintf("A NEW MESSAGE ARRIVED %d TIMES\n", SERVER_NEW_MESSAGE_RECEIVED);
fprintf("A MESSAGE HAD ALREADY BEEN MET %d TIMES\n", SERVER_MESSAGE_MET);
fprintf("SERVER RECEIVED A TIMEOUT %d TIMES\n", SERVER_TIMEOUT);
fprintf("A SERVER ERROR OCCURED %d TIMES\n", SERVER_ERROR);
fprintf("SERVER COMMUNICATION SOCKET CLOSED SUCCESSFULLY %d TIMES\n", SERVER_CLOSED_SUCCESSFULLY);
fprintf("SERVER RECOGNISED A MESSAGE RECEIVED AS FINAL DESTINATION %d\n", SERVER_ARRIVED_DESTINATION);
%% CLIENT
fprintf("-----------------\n");
CLIENT_SENT_SUCCESS = 0;
CLIENT_TIMEOUT = 0;
CLIENT_ERROR = 0;
CLIENT_SEND_MESSAGE_ERROR = 0;
CLIENT_ALL_SENT = 0;
CLIENT_SUMMARY = zeros(0,2);

for i = 1:length(lines)
    index = regexp(lines(i),'^CLIENT:*'); 
    
    sent = regexp(lines(i), 'MESSAGE SENT SUCCESSFULLY', 'match');
    if ~(isempty(sent))
        %disp(lines(i));
        CLIENT_SENT_SUCCESS = CLIENT_SENT_SUCCESS+ 1;
    end
    
    timeout = regexp(lines(i), 'MESSAGE SEND PROCESS TIMEOUT RECEIVED', 'match');
    if ~(isempty(timeout))
        %disp(lines(i));
        CLIENT_TIMEOUT = CLIENT_TIMEOUT + 1;
    end
    
    error = regexp(lines(i), 'ERROR FAILed TO SEND MESSAGE', 'match');
    if ~(isempty(error))
        %disp(lines(i));
        CLIENT_ERROR = CLIENT_ERROR + 1;
    end
    
    error_send_message = regexp(lines(i), 'COMMUNICATION TERMINATED', 'match');
    if ~(isempty(error_send_message))
        %disp(lines(i));
        CLIENT_SEND_MESSAGE_ERROR = CLIENT_SEND_MESSAGE_ERROR + 1;
    end
    
    all_sent = regexp(lines(i), 'CLIENT: ALL ELEMENTS HAVE ALREADY BEEN SENT', 'match');
    if ~(isempty(all_sent))
        %disp(lines(i));
        CLIENT_ALL_SENT = CLIENT_ALL_SENT + 1;
    end
    
    summary = regexp(lines(i), 'COMMUNICATION DURATION', 'match');
    if ~(isempty(summary))
        %disp(lines(i)); 
        res = regexp(lines(i), ' \d+ ' , 'match');
        if ~(isempty(res))
            %disp(res);
            if( length(res) == 2)
                CLIENT_SUMMARY(end+1,1) = double(res(1));
                CLIENT_SUMMARY(end,2) = double(res(2));
            end
        end
    end
end

CLIENT_SEND_FREQ = tabulate(CLIENT_SUMMARY(:,1));
for i = 1:length(CLIENT_SEND_FREQ(:,1))
    MEAN_SEND_TIME(i) = 0;
    for j = 1:length(CLIENT_SUMMARY(:,2))
        if CLIENT_SEND_FREQ(i,1) == CLIENT_SUMMARY(j,1)
            MEAN_SEND_TIME(i) = MEAN_SEND_TIME(i) + CLIENT_SUMMARY(j,2);
        end
    end
end

MEAN_SEND_TIME = MEAN_SEND_TIME' ./ CLIENT_SEND_FREQ(:,2);

fprintf("A MESSAGE WAS SENT SUCCESSFULLY %d TIMES\n", CLIENT_SENT_SUCCESS);
fprintf("CLIENT RECEIVED A TIMEOUT %d TIMES\n", CLIENT_TIMEOUT);
fprintf("CLIENT ENCOUNTERED AN ERROR %d\n", CLIENT_ERROR);
fprintf("CLIENT ENCOUTERED AN ERROR IN SEND MESSAGE FUNCTION %d TIMES\n", CLIENT_SEND_MESSAGE_ERROR);
fprintf("CLIENT DID NOT SEND ANY MESSAGES, ALL ELEMENTS HAD ALREADY BEEN SENT %d TIMES\n", CLIENT_ALL_SENT);
for i = 1:length(CLIENT_SEND_FREQ(:,1))
    fprintf("%d MESSAGES SENT %d TIMES WITH MEAN TIME %.0f usecs\n", CLIENT_SEND_FREQ(i,1), CLIENT_SEND_FREQ(i,2), MEAN_SEND_TIME(i));
end

temp = 0;
count = 0;
for i = 1:length(CLIENT_SEND_FREQ(:,1))
    temp = 0;
    count = 0;
    for j = 1:length(CLIENT_SUMMARY(:,2))
        if CLIENT_SEND_FREQ(i,1) == CLIENT_SUMMARY(j,1)
            count = count + 1;
            temp(count) = CLIENT_SUMMARY(j,2);
        end
    end
    str = sprintf("SEND DURATION for %d MESSAGES PER COMMUNCATION", CLIENT_SEND_FREQ(i,1));
    figure;
    scatter(temp, 1:length(temp));
    title(str);
    xlabel('index') 
    ylabel('time in usec');
end




clearvars i index sent timeout error error_send_message summary res temp j all_sent str count;