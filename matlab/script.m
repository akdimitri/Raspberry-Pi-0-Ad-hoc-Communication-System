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
fprintf("READING FROM FILE...\n");
lines = cell(0,1);  %initialize array to hold strings

% ENTER PATH OF LOG FILE HERE
path = '../results/test_final_8462/log.txt';
fileID = fopen(path,'r');
fprintf("FROM PATH %s \n", path);

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
        message = regexp( lines(i), '\d{4}_\d{4}_\d{10}_CPU temperature is : \d{2}.\d{6}', 'match');
        if ~(isempty(message))
            count = count + 1;
            fprintf("%s\n",message); 
            MESSAGE_GENERATOR_MESSAGE(count) = message;
        end
    end
end
MESSAGES_GENERATED = count;

fprintf("\nMESSAGES GENERATED %d\n\n", MESSAGES_GENERATED);

clearvars count index message i path; 

%% SERVER
fprintf("-----------------\n");
SERVER_GRANTED_COMMUNICATION = 0;
SERVER_MESSAGE_RECEIVED = 0;
SERVER_NEW_MESSAGE_RECEIVED = 0;
SERVER_MESSAGE_MET = 0;
SERVER_TIMEOUT = 0;
SERVER_ERROR = 0;
SERVER_CLOSED_SUCCESSFULLY = 0;
SERVER_ARRIVED_DESTINATION = 0;


temp0 = 0;

for i = 1:length(lines)
    index = regexp(lines(i),'^SERVER:*');
    if ~(isempty(index))
        %fprintf("%s \n", lines(i));
        granted = regexp(lines(i), 'GRANTED COMMUNICATION', 'match');
        if ~(isempty(granted))
             %fprintf("%s \n", lines(i));
             SERVER_GRANTED_COMMUNICATION = SERVER_GRANTED_COMMUNICATION + 1;
        end
        
        recv = regexp(lines(i), 'SERVER: MESSAGE RECEIVED', 'match');
        if ~(isempty(recv))
            %fprintf("%s \n", lines(i));
            SERVER_MESSAGE_RECEIVED = SERVER_MESSAGE_RECEIVED + 1;
        end
        
        new = regexp(lines(i), 'SERVER: NEW MESSAGE RECEIVED', 'match');
        if ~(isempty(new))
            %fprintf("%s \n", lines(i));
            SERVER_NEW_MESSAGE_RECEIVED = SERVER_NEW_MESSAGE_RECEIVED + 1;
        end
        
        met = regexp(lines(i), 'MESSAGE HAS ALREADY BEEN MET', 'match');
        if ~(isempty(met))
            %fprintf("%s \n", lines(i));
            SERVER_MESSAGE_MET = SERVER_MESSAGE_MET + 1;
        end
        
        dest = regexp(lines(i), 'MESSAGE ARRIVED AT ITS DESTINATION', 'match');
        if ~(isempty(dest))
            %fprintf("%s \n", lines(i));
            SERVER_ARRIVED_DESTINATION = SERVER_ARRIVED_DESTINATION + 1;
        end
        
        summary = regexp(lines(i), 'SERVER: MESSAGES RECEIVED SUCCESSFULLY FROM *', 'match');
        if ~(isempty(summary))
            temp0 = temp0 + 1;
            saved = regexp( lines(i), ': (\d+)', 'tokens');            
            summary_received(temp0) = str2num(saved{1}{1});
            summary_saved(temp0) = str2num(saved{2}{1});            
            %fprintf("%s \n", lines(i));            
        end
        timeout = regexp(lines(i), 'SERVER: TIMEOUT RECEIVED', 'match');
        if ~(isempty(timeout))
            fprintf("%s \n", lines(i));  
            SERVER_TIMEOUT = SERVER_TIMEOUT + 1;
        end
        
        error = regexp(lines(i), 'ERROR', 'match');
        if ~(isempty(error))
            %fprintf("ERROR line %d : %s \n", i, lines(i)); 
            SERVER_ERROR = SERVER_ERROR + 1;
        end
        
        suc = regexp(lines(i), 'CLOSED SUCCESSFULLY', 'match');
        if ~(isempty(suc))
            %disp(lines(i));
            SERVER_CLOSED_SUCCESSFULLY = SERVER_CLOSED_SUCCESSFULLY + 1;
        end
        

    
    end
end
SERVER_summary_received = tabulate(summary_received);
SERVER_summary_saved = tabulate(summary_saved);
clearvars index granted i ans new met timeout suc dest error recv saved summary temp0 summary_received summary_saved;
fprintf("\nSERVER\n");
fprintf("%d TIMES A FOREIGN DEVICE GRANTED COMMUNICATION \n", SERVER_GRANTED_COMMUNICATION);
fprintf("%d MESSAGES ARRIVED AT SERVER\n", SERVER_MESSAGE_RECEIVED);
fprintf("%d TIMES A NEW MESSAGE ARRIVED \n", SERVER_NEW_MESSAGE_RECEIVED);
fprintf("%d TIMES A MESSAGE HAD ALREADY BEEN MET \n", SERVER_MESSAGE_MET);
fprintf("%d TIMES SERVER WAS THE FINAL DESTINATION\n", SERVER_ARRIVED_DESTINATION);
fprintf("%d TIMES A TIMEOUT RECEIVED\n", SERVER_TIMEOUT);
fprintf("%d TIMES A SERVER ERROR OCCURED \n", SERVER_ERROR);
fprintf("%d TIMES SERVER COMMUNICATION SOCKET CLOSED SUCCESSFULLY \n", SERVER_CLOSED_SUCCESSFULLY);

%% CLIENT
 fprintf("-----------------\n");
CLIENT_SENT_SUCCESS = 0;
CLIENT_TIMEOUT = 0;
CLIENT_ERROR = 0;
CLIENT_ALL_SENT = 0;


for i = 1:length(lines)
    index = regexp(lines(i),'^CLIENT:*'); 
    if ~(isempty(index))
        %fprintf("%s \n", lines(i));   
        sent = regexp(lines(i), 'SENT SUCCESSFULLY', 'match');
        if ~(isempty(sent))
            %fprintf("%s \n", lines(i)); 
            CLIENT_SENT_SUCCESS = CLIENT_SENT_SUCCESS+ 1;
            duration = regexp(lines(i), 'in (\d+) usecs', 'tokens');
            CLIENT_SEND_DURATION(CLIENT_SENT_SUCCESS) = str2num(duration{1}{1});
        end
        
        timeout = regexp(lines(i), 'MESSAGE SEND PROCESS TIMEOUT RECEIVED', 'match');
        if ~(isempty(timeout))
            %disp(lines(i));
            CLIENT_TIMEOUT = CLIENT_TIMEOUT + 1;
        end
        
        error = regexp(lines(i), 'ERROR', 'match');
        if ~(isempty(error))
            fprintf("CLIENT ERROR in line %d : %s \n",i, lines(i)); 
            CLIENT_ERROR = CLIENT_ERROR + 1;
        end        
        
        
        all_sent = regexp(lines(i), 'CLIENT: ALL ELEMENTS HAVE ALREADY BEEN SENT', 'match');
        if ~(isempty(all_sent))
            %disp(lines(i));
            CLIENT_ALL_SENT = CLIENT_ALL_SENT + 1;
        end
    
    end
end
 
CLIENT_MEAN_SEND_TIME = mean(CLIENT_SEND_DURATION);
fprintf('\nCLIENT\n');
fprintf("%d TIMES A MESSAGE WAS SENT SUCCESSFULLY \n", CLIENT_SENT_SUCCESS);
fprintf("%d TIMES CLIENT RECEIVED A TIMEOUT \n", CLIENT_TIMEOUT);
fprintf("%d TIMES CLIENT ENCOUNTERED AN ERROR \n", CLIENT_ERROR);
fprintf("%d TIMES CLIENT DID NOT SEND ANY MESSAGES, ALL ELEMENTS HAD ALREADY BEEN SENT \n", CLIENT_ALL_SENT);
fprintf("%f usecs IS THE MEAN DURATION OF SEND PROCCESS\n", CLIENT_MEAN_SEND_TIME);
% for i = 1:length(CLIENT_SEND_FREQ(:,1))
%     fprintf("%d MESSAGES SENT %d TIMES WITH MEAN TIME %.0f usecs\n", CLIENT_SEND_FREQ(i,1), CLIENT_SEND_FREQ(i,2), MEAN_SEND_TIME(i));
% end
% 
% temp = 0;
% count = 0;
% for i = 1:length(CLIENT_SEND_FREQ(:,1))
%     temp = 0;
%     count = 0;
%     for j = 1:length(CLIENT_SUMMARY(:,2))
%         if CLIENT_SEND_FREQ(i,1) == CLIENT_SUMMARY(j,1)
%             count = count + 1;
%             temp(count) = CLIENT_SUMMARY(j,2);
%         end
%     end
%     str = sprintf("SEND DURATION for %d MESSAGES PER COMMUNCATION", CLIENT_SEND_FREQ(i,1));
%     figure;
%     scatter(temp, 1:length(temp));
%     title(str);
%     xlabel('index') 
%     ylabel('time in usec');
% end

figure;
scatter(1:length(CLIENT_SEND_DURATION), CLIENT_SEND_DURATION);
title('SEND DURATION TIME');
xlabel('index') 
ylabel('time in usec');


clearvars i index sent timeout error error_send_message summary res temp j all_sent str count duration;