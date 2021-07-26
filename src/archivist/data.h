/**************************************************************************
* Copyright 2015 John Denholm                                             *
*                                                                         *
* Licensed under the Apache License, Version 2.0 (the "License");         *
* you may not use this file except in compliance with the License.        *
* You may obtain a copy of the License at                                 *
*                                                                         *
*     http://www.apache.org/licenses/LICENSE-2.0                          *
*                                                                         *
* Unless required by applicable law or agreed to in writing, software     *
* distributed under the License is distributed on an "AS IS" BASIS,       *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
* See the License for the specific language governing permissions and     *
* limitations under the License.                                          *
*                                                                         *
*                                                                         *
* data.h - incoming data handling function prototypes                     *
*                                                                         *
* Updates:                                                                *
**************************************************************************/

#ifndef ARCHIVIST_DATA_H
#define ARCHIVIST_DATA_H

#define LINE_SEPARATOR			'\n'


line_fn data_handle_line;
line_fn data_handle_record;

buf_fn data_parse_line;
buf_fn data_parse_bin;


#endif
