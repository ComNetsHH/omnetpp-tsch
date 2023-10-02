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

function str_pre=str_preamble()

lf=setstr(10);
tab='        ';

str_pre=['<?xml version="1.0"?>', lf,...
'<TSCHSchedule xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"  xsi:schemaLocation="TSCH_SCHEDULE.xsd">', lf,...
tab,'<!-- Slotframe -->'];

