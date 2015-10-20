/*****************************************************************************/
/*                                                                           */
/* УПО - Управление Промышленым Оборудованием                                */
/*  Авторское Право (C) <2015> <Кузьмин Ярослав>                             */
/*                                                                           */
/*  Эта программа является свободным программным обеспечением:               */
/*  вы можете распространять и/или изменять это в соответствии с             */
/*  условиями в GNU General Public License, опубликованной                   */
/*  Фондом свободного программного обеспечения, как версии 3 лицензии,       */
/*  или (по вашему выбору) любой более поздней версии.                       */
/*                                                                           */
/*  Эта программа распространяется в надежде, что она будет полезной,        */
/*  но БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ; даже без подразумеваемой гарантии            */
/*  Или ПРИГОДНОСТИ ДЛЯ КОНКРЕТНЫХ ЦЕЛЕЙ. См GNU General Public License      */
/*  для более подробной информации.                                          */
/*                                                                           */
/*  Вы должны были получить копию GNU General Public License                 */
/*  вместе с этой программой. Если нет, см <http://www.gnu.org/licenses/>    */
/*                                                                           */
/*  Адрес для контактов: kuzmin.yaroslav@gmail.com                           */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/* cid - control industrial device                                           */
/*  Copyright (C) <2015> <Kuzmin Yaroslav>                                   */
/*                                                                           */
/*  This program is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*                                                                           */
/*  Email contact: kuzmin.yaroslav@gmail.com                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef TOTAL_H
#define TOTAL_H

#define SUCCESS          0
#define FAILURE          1

#define OK               0
#define NOT_OK           1

#define DISCONNECT       FAILURE
#define CONNECT          SUCCESS

extern const char STR_NAME_PROGRAMM[];
extern const char STR_COPYRIGHT[];
extern const char STR_COMMENT[];
extern const char STR_LICENSE[];
extern const char STR_EMAIL[];
extern const char STR_EMAIL_LABEL[];
extern const char * STR_AUTHORS[];

extern const char STR_LOGGING[];

extern const char STR_KEY_FILE_NAME[];
extern GKeyFile * ini_file;
extern const char STR_GLOBAL_KEY[];
extern const char STR_MODBUS_KEY[];
extern const char STR_VIDEO_KEY[];

extern const char STR_NAME_DB[];

extern GdkRGBA color_black;
extern GdkRGBA color_green;
extern GdkRGBA color_red;
extern GdkRGBA color_white;
extern GdkRGBA color_lite_blue;
extern GdkRGBA color_lite_red;
extern GdkRGBA color_lite_green;

#define SIZE_FONT_EXTRA_LARGE      40000
#define SIZE_FONT_LARGE            30000
#define SIZE_FONT_EXTRA_MEDIUM     20000
#define SIZE_FONT_MEDIUM           15000
#define SIZE_FONT_SMALL            10000
#define SIZE_FONT_MINI             9000

extern GString * temp_string;

#define MILLISECOND      1000

#endif

