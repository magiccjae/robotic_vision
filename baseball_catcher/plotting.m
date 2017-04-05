clear; clc; close all;

[y, z] = textread('pts.txt', '%f %f', 18);
z2 = 0:0.1:500;

% p2 = [0.671037 -49.6086 1079.34]/100;
% least square estimation
p2 = polyfit(z,y,2);
y2 = polyval(p2,z2);

figure(2); hold on;
title('y vs z');
scatter(z,y);
plot(z2,y2);
legend('measured', 'estimated');
xlabel('z(inch)');
ylabel('y(inch)');
% xlim([0 500]);
% ylim([0 80]);
