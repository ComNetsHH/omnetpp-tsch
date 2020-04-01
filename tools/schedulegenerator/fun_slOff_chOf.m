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

function slOff_chOff=fun_slOff_chOf(macSlotframeSize, macChannelSize)
% Create Matrix of unique pairs (slotOffset, channelOffset)
%
% dimension time (slotframe):
k=1:macSlotframeSize; 
slotOff=rem(k-1,macSlotframeSize);

l=1:macChannelSize; 
ChannOff=rem(l-1,macChannelSize);

% dimension frequency (channel):

slOff_chOff=[]; 
for k=1:macChannelSize
    slOff_chOff=[slOff_chOff; slotOff',ChannOff(k)*ones(size(slotOff'))];
end

return
