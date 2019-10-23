figure('units','normalized','outerposition',[0 0 1 1])
s1=subplot(311)
yyaxis right
plot(absolute_time,filtered_PM25);
datetick('x','HHPM')
ylabel('PM 2.5 (ug/m^3)');
yyaxis left
plot(absolute_time,filtered_Max_temp);
ylabel('Temperature( degree celsius)');
title('Estimated stove temperature and measured PM 2.5 levels against period of observation')
xlabel('Period of observation')
legend('Estimated stove temperature', 'Measured PM 2.5')
s2=subplot(312)
% %Replace single zero value between two valid values with previous value to
% %workround hardware bug
% binary_pattern_index=strfind(FCBDB62F1727_count_binary', [1 0 1]);
% FCBDB62F1727_count_binary(binary_pattern_index+1)=FCBDB62F1727_count_binary(binary_pattern_index);
% 
% binary_pattern_index=strfind(FCBDB62F1727_count_binary', [1 0 0 1]);
% FCBDB62F1727_count_binary(binary_pattern_index+1)=FCBDB62F1727_count_binary(binary_pattern_index);
% FCBDB62F1727_count_binary(binary_pattern_index+2)=FCBDB62F1727_count_binary(binary_pattern_index);
% 
% binary_pattern_index=strfind(F2416138F240_count_binary', [1 0 1]);
% F2416138F240_count_binary(binary_pattern_index+1)=F2416138F240_count_binary(binary_pattern_index);
% 
% binary_pattern_index=strfind(F2416138F240_count_binary', [1 0 0 1]);
% F2416138F240_count_binary(binary_pattern_index+1)=F2416138F240_count_binary(binary_pattern_index);
% F2416138F240_count_binary(binary_pattern_index+2)=F2416138F240_count_binary(binary_pattern_index);
% 
% F2416138F240_mean_dBm_logical=F2416138F240_mean_dBm < 0;
% binary_pattern_index=strfind(F2416138F240_mean_dBm_logical', [1 0 1]);
% F2416138F240_mean_dBm(binary_pattern_index+1)=F2416138F240_mean_dBm(binary_pattern_index);
% 
% binary_pattern_index=strfind(F2416138F240_mean_dBm_logical', [1 0 0 1]);
% F2416138F240_mean_dBm(binary_pattern_index+1)=F2416138F240_mean_dBm(binary_pattern_index);
% F2416138F240_mean_dBm(binary_pattern_index+2)=F2416138F240_mean_dBm(binary_pattern_index);
% F2416138F240_mean_dBm(F2416138F240_mean_dBm == 0) = NaN;
% 
% FCBDB62F1727_mean_dBm_logical=FCBDB62F1727_mean_dBm < 0;
% binary_pattern_index=strfind(FCBDB62F1727_mean_dBm_logical', [1 0 1]);
% FCBDB62F1727_mean_dBm(binary_pattern_index+1)=FCBDB62F1727_mean_dBm(binary_pattern_index);
% 
% binary_pattern_index=strfind(FCBDB62F1727_mean_dBm_logical', [1 0 0 1]);
% FCBDB62F1727_mean_dBm(binary_pattern_index+1)=FCBDB62F1727_mean_dBm(binary_pattern_index);
% FCBDB62F1727_mean_dBm(binary_pattern_index+2)=FCBDB62F1727_mean_dBm(binary_pattern_index);
% FCBDB62F1727_mean_dBm(FCBDB62F1727_mean_dBm == 0) = NaN;
% 
y2=area(absolute_time, (Njeri_presence_binary*2))
hold on;
y1=area(absolute_time, (Mama_presence_binary))
datetick('x','HHPM')
xlabel('Period of observation')
ylabel('Occupancy');
ylabel('Temperature( degree celsius)');
ylabel('Occupancy');
xlabel('Period of observation')
legend('Secondary participant occupancy(Female-26 years old)','Primary participant occupancy(Female-above 40 years old)')
title('Participant presence against period of observation')
ylabel('Occupancy(present or absent)');
s3=subplot(313);
c=categorical({'primary participant(Female-above 40 years old)','WHO 24 hr mean','Secondary participant(Female-26 years old)'})
bar(c, [mean(Mama_presence_binary.*filtered_PM25), 25, mean(Njeri_presence_binary.*filtered_PM25)]);
ylabel('Mean PM 2.5 (ug/m^3)');
title("Participant mean PM 2.5 exposure compared to WHO's 24 hour mean guideline")
print('gikambura', '-dpng', '-r800'); %<-Save as PNG with 800 DPI