% Copyright (C) 2020  Institute of Communication Networks (ComNets),
%                     Hamburg University of Technology (TUHH)
%           (C) 2020  Frank Laue
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

function str=xml_Link(macSlotframeSize, slotOffset, channelOffset, tx, rx, shared, neighbor_adress)

[numAddr,addLen]=size(neighbor_adress);

str=[''];
tab='        ';
lf=setstr(10);

for k=1:numAddr,
    STR=[
    [tab, tab, '<Link slotOffset="',num2str(slotOffset),'" channelOffset="',num2str(channelOffset),'">',lf],...
    [tab, tab, tab, '<Option tx="',tx, '" rx="',rx,'" shared="',shared,'"/>',lf],...
    [tab, tab, tab, '<Type normal="true" advertising="false" advertisingOnly="false" />',lf],...
    [tab, tab, tab,'<Neighbor address="',neighbor_adress(k,:),'"/>',lf],...
    [tab, tab, '</Link>']
    ];
    str=[str;STR];
end

return
