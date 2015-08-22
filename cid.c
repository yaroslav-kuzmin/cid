/*****************************************************************************/
/*                                                                           */
/* УПО - Управление Промышленым Оборудованием                                */
/*  Копирайт (C) <2015> <Кузьмин Ярослав>                                    */
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
/*  Copyright (C) <2013> <Kuzmin Yaroslav>                                   */
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
#include <stdlib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include "total.h"

/*****************************************************************************/
GtkWidget * main_window = NULL;

/*****************************************************************************/
static GIOChannel * log_file = NULL;
static GTimeVal current_time;
static char STR_CURRENT_TIME[] = "\n[ 00.00.0000 00:00:00 ] ";
#define LEN_STR_CURRENT_TIME    25

void save_file(const gchar *log_domain,GLogLevelFlags log_level,
             const gchar *message,gpointer user_data)
{
	GDateTime * p_dt;
	GIOStatus rc;
	gsize bw;

	if(log_file != NULL){
 		g_get_current_time (&current_time);
		p_dt = g_date_time_new_from_timeval_local(&current_time);
		g_sprintf(STR_CURRENT_TIME,"\n[ %02d.%02d.%04d %02d:%02d:%02d ] "
 		         ,g_date_time_get_day_of_month(p_dt)
		         ,g_date_time_get_month(p_dt)
		         ,g_date_time_get_year(p_dt)
		         ,g_date_time_get_hour(p_dt)
		         ,g_date_time_get_minute(p_dt)
		         ,g_date_time_get_second(p_dt));
		error_check = NULL;
		rc = g_io_channel_write_chars(log_file
				,STR_CURRENT_TIME,LEN_STR_CURRENT_TIME
 				,&bw,&error_check);
		rc = g_io_channel_write_chars(log_file
		,message,-1,&bw,&error_check);
		if(rc != G_IO_STATUS_NORMAL){
 		 	/*TODO*/;
			g_error_free(error_check);
		}
	}
}

int init_log(void)
{
	log_file = g_io_channel_new_file (STR_LOG_FILE,"a",&error_check);
	if(log_file == NULL){
		g_message("Not open log file : %s : %s",STR_LOG_FILE,error_check->message);
		exit(FAILURE);
	}
	g_log_set_default_handler(save_file,NULL);
	return SUCCESS;
}
/*****************************************************************************/

/*****************************************************************************/
int main(int argc,char * argv[])
{
	init_log();

	g_message("%s",STR_NAME_PROGRAMM);

	/*TODO перенести в функцию закрытия*/
	g_io_channel_shutdown(log_file,TRUE,&error_check);

	return SUCCESS;
}
/*****************************************************************************/
