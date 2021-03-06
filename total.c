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

/*****************************************************************************/
#include <gtk/gtk.h>

/*****************************************************************************/
/*    Общие переменые                                                        */
/*****************************************************************************/

const char STR_NAME_PROGRAMM[] = "Управление Промышленным Оборудованием";
const char STR_COPYRIGHT[] = "(C) <2015> <Кузьмин Ярослав>";
const char STR_COMMENT[] = "Программа позволяет управлять установкой водяной автоматической";
const char STR_LICENSE[] =
"  Эта программа является свободным программным обеспечением:             \n"
"  вы можете распространять и/или изменять это в соответствии с           \n"
"  условиями в GNU General Public License, опубликованной                 \n"
"  Фондом свободного программного обеспечения, как версии 3 лицензии,     \n"
"  или (по вашему выбору) любой более поздней версии.                     \n"
"                                                                         \n"
"  Эта программа распространяется в надежде, что она будет полезной,      \n"
"  но БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ; даже без подразумеваемой гарантии          \n"
"  Или ПРИГОДНОСТИ ДЛЯ КОНКРЕТНЫХ ЦЕЛЕЙ. См GNU General Public License    \n"
"  для более подробной информации.                                        \n"
"                                                                         \n"
"  Вы должны были получить копию GNU General Public License               \n"
"  вместе с этой программой. Если нет, см <http://www.gnu.org/licenses/>  \n"
"                                                                           ";

const char STR_EMAIL[] = "kuzmin.yaroslav@gmail.com";
const char STR_EMAIL_LABEL[] = "kuzmin.yaroslav@gmail.com";
const char * STR_AUTHORS[] = {"Кузьмин Ярослав",NULL};

const char STR_KEY_FILE_NAME[] = "cid.ini";
GKeyFile * ini_file = NULL;
const char STR_GLOBAL_KEY[] = "global";
const char STR_MODBUS_KEY[] = "modbus";
const char STR_VIDEO_KEY[] = "video";

const char STR_LOGGING[] = "cid.log";
const char STR_NAME_DB[] = "cid.db";

GdkRGBA color_black =     {0  ,0  ,0  ,1};
GdkRGBA color_green =     {0  ,1  ,0  ,1};
GdkRGBA color_red =       {1  ,0  ,0  ,1};
GdkRGBA color_white =     {1  ,1  ,1  ,1};
GdkRGBA color_lite_blue = {0.2,0.1,1  ,1};
GdkRGBA color_lite_red  = {1  ,0.2,0.1,1};
GdkRGBA color_lite_green= {0.2,1  ,0.1,1};

GString * temp_string = NULL;

/*****************************************************************************/
