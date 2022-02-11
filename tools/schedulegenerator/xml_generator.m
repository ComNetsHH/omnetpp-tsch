% TSCH schedule xml generator implementation according to spec. 

% Restriction:
% Present version only allows 16^2=256 different neighbor_addresses.
% If this is not enough, construction of the neighbor_address has to 
% be augmented.

% Copyright (C) 2020  Institute of Communication Networks (ComNets),
%                     Hamburg University of Technology (TUHH)
%           (C) 2020  Frank Laue, Daniel Plöger
%
% This program is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with this program.  If not, see <https://www.gnu.org/licenses/>.

function xml_generator(m, n, fillSchedule,macChannelSize,macSlotframeSize)
% General variable Parameters:
write2file=true;  % write files
Disp=false;        % Show xml strings on the screen

if nargin < 5
    % Tsch Parameter
    %
    % Variables:
    m=1;           % Number of Gateways
    n=2;           % Number of Sensor Nodes per Gateway
    fillSchedule=true;
    macChannelSize=2;
    macSlotframeSize=2;
end

% Fixed:
%lf="\n";
lf=setstr(10);
slotOffset=0; 
channelOffset=0;
tx=0;              % rx, tx are set as int -> will be converted to string 
rx=1;
shared='false';
macDelimiter = ":";
xmlDelimiter = "_";
%===============================================================================
% Generate unique combinations of (slotOffset, channelOffset)
slOff_chOff=fun_slOff_chOf(macSlotframeSize, macChannelSize);
% Generate Scenario-xml-schedule-files:
count=1;
count_max=macChannelSize*macSlotframeSize;
if fillSchedule
   N=count_max;
else
   N=n;
end

for nn=1:n
    eval(['str_node_',num2str(nn),'='''';']);
end


for mm=1:m  % Iteration over Gateways
    if Disp
       disp('=================================================================')
    end
    str_gw=[''];
    GW_add = dec2hex(mm+(mm-1)*n,2); 
    gateway_address=['0A:AA:00:00:00:',GW_add];
    Gateway_schedule_name=['schedule-',gateway_address,'.xml'];
    Gateway_schedule_name = replace(Gateway_schedule_name,macDelimiter,xmlDelimiter);
    for nn=1:N
        if fillSchedule
           nn=rem(nn-1,n)+1;
        end
        eval(['STR=str_node_',num2str(nn),';']);

        neighbor_address=['0A:AA:00:00:00:',dec2hex(mm+(mm-1)*n+nn,2)];

        % GW:        
        str=xml_Link(macSlotframeSize, slOff_chOff(count+mm-1,1),... 
                     slOff_chOff(count+mm-1,2), bool_num2str(tx),... 
                     bool_num2str(rx), shared, neighbor_address);
        str_gw=[str_gw,lf,str];
        % Node:
        str=xml_Link(macSlotframeSize, slOff_chOff(count+mm-1,1),... 
                     slOff_chOff(count+mm-1,2), bool_num2str(~tx),... 
                     bool_num2str(~rx), shared, gateway_address);
        str=[STR, str, lf];

        eval(['str_node_',num2str(nn),'=str;'])
        Node_schedule_name=['schedule-',neighbor_address,'.xml'];
        Node_schedule_name = replace(Node_schedule_name,macDelimiter,xmlDelimiter);

        if count <= n
           %disp(["=========>> count: ",num2str(count)])
           eval(['Node_schedule_name_node_',num2str(nn),'=Node_schedule_name;']);
        end

        count=count+1;
        if fillSchedule && (nn==n)
           tx=~tx; % Toggle rx, tx for each round of n
           rx=~rx;
        end
        if count > count_max
           if Disp
              disp([lf, '==========>>>>> Schedule matrix full ===========> stopped', lf])
           end
           break
        end
    end
    str_gw=[str_preamble(), str_gw, lf, str_epilogue()];
    if write2file
       fid = fopen (Gateway_schedule_name, 'w'); 
       fprintf(fid,str_gw);
       %fdisp(fid,str_gw); 
       fclose(fid);
    else
       if Disp
          disp('-------------------------------------------------------------')
          disp(['Gateway_schedule_name=',Gateway_schedule_name,lf])
          disp(str_gw)
       end
    end
    for nn=1:n
        eval(['str=str_node_',num2str(nn),';']);
        str=[str_preamble(), lf, str, lf, str_epilogue()];
        eval(['Node_schedule_name=Node_schedule_name_node_',num2str(nn),';']);
        if write2file
           fid = fopen (Node_schedule_name, 'w'); 
           fprintf(fid,str); 
           fclose(fid);
        else
           if Disp
              disp('----------------------------------------------------------')
              disp(['Node_schedule_name=',Node_schedule_name, lf])
              disp(str)
           end
        end
    end           

end
