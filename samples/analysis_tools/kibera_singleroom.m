figure('units','normalized','outerposition',[0 0 1 1])
s1=subplot(311)
yyaxis right
plot(absolute_time,PM25);
datetick('x','HHPM')
ylabel('PM 2.5 (ug/m^3)');
yyaxis left
plot(absolute_time,Max_temp);
ylabel('Temperature( degree celsius)');
title('Estimated stove temperature and measured PM 2.5 levels against period of observation')
xlabel('Period of observation')
legend('Estimated stove temperature', 'Measured PM 2.5')
s2=subplot(312)
%Replace single zero value between two valid values with previous value to
%workround hardware bug
binary_pattern_index=strfind(FCBDB62F1727_count_binary', [1 0 1]);
FCBDB62F1727_count_binary(binary_pattern_index+1)=FCBDB62F1727_count_binary(binary_pattern_index);

binary_pattern_index=strfind(FCBDB62F1727_count_binary', [1 0 0 1]);
FCBDB62F1727_count_binary(binary_pattern_index+1)=FCBDB62F1727_count_binary(binary_pattern_index);
FCBDB62F1727_count_binary(binary_pattern_index+2)=FCBDB62F1727_count_binary(binary_pattern_index);

binary_pattern_index=strfind(F2416138F240_count_binary', [1 0 1]);
F2416138F240_count_binary(binary_pattern_index+1)=F2416138F240_count_binary(binary_pattern_index);

binary_pattern_index=strfind(F2416138F240_count_binary', [1 0 0 1]);
F2416138F240_count_binary(binary_pattern_index+1)=F2416138F240_count_binary(binary_pattern_index);
F2416138F240_count_binary(binary_pattern_index+2)=F2416138F240_count_binary(binary_pattern_index);

F2416138F240_mean_dBm_logical=F2416138F240_mean_dBm < 0;
binary_pattern_index=strfind(F2416138F240_mean_dBm_logical', [1 0 1]);
F2416138F240_mean_dBm(binary_pattern_index+1)=F2416138F240_mean_dBm(binary_pattern_index);

binary_pattern_index=strfind(F2416138F240_mean_dBm_logical', [1 0 0 1]);
F2416138F240_mean_dBm(binary_pattern_index+1)=F2416138F240_mean_dBm(binary_pattern_index);
F2416138F240_mean_dBm(binary_pattern_index+2)=F2416138F240_mean_dBm(binary_pattern_index);
F2416138F240_mean_dBm(F2416138F240_mean_dBm == 0) = NaN;

FCBDB62F1727_mean_dBm_logical=FCBDB62F1727_mean_dBm < 0;
binary_pattern_index=strfind(FCBDB62F1727_mean_dBm_logical', [1 0 1]);
FCBDB62F1727_mean_dBm(binary_pattern_index+1)=FCBDB62F1727_mean_dBm(binary_pattern_index);

binary_pattern_index=strfind(FCBDB62F1727_mean_dBm_logical', [1 0 0 1]);
FCBDB62F1727_mean_dBm(binary_pattern_index+1)=FCBDB62F1727_mean_dBm(binary_pattern_index);
FCBDB62F1727_mean_dBm(binary_pattern_index+2)=FCBDB62F1727_mean_dBm(binary_pattern_index);
FCBDB62F1727_mean_dBm(FCBDB62F1727_mean_dBm == 0) = NaN;

y1=area(absolute_time, (FCBDB62F1727_count_binary*2)-((FCBDB62F1727_mean_dBm-nanmean(FCBDB62F1727_mean_dBm))/((max(FCBDB62F1727_mean_dBm)-min(FCBDB62F1727_mean_dBm))/(2/2))))
hold on;
y2=area(absolute_time, (F2416138F240_count_binary)-((F2416138F240_mean_dBm-nanmean(F2416138F240_mean_dBm))/((max(F2416138F240_mean_dBm)-min(F2416138F240_mean_dBm))/(1/2))))
datetick('x','HHPM')
xlabel('Period of observation')
ylabel('Occupancy');
ylabel('Temperature( degree celsius)');
ylabel('Occupancy');
xlabel('Period of observation')
legend('Primary participant occupancy(Female)','Secondary participant occupancy(Male)')
title('Participant presence and average movement against period of observation')
ylabel('Occupancy(present or absent)');
s3=subplot(313);
c=categorical({'WHO 24 hr mean','primary participant(Female and 8 month child)','Secondary participant(Male)'})
bar(c, [25, mean(FCBDB62F1727_count_binary.*PM25) mean(F2416138F240_count_binary.*PM25)])
ylabel('Mean PM 2.5 (ug/m^3)');
title("Participant mean PM 2.5 exposure compared to WHO's 24 hour mean guideline")
print('Kibera', '-dpng', '-r800'); %<-Save as PNG with 300 DPI