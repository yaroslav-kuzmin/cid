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
#include <stdlib.h>
#include <stdint.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include "total.h"
#include "job.h"
#include "video.h"
#include "control.h"

/*****************************************************************************/

/*****************************************************************************/
/*  конфиурирование системы                                                  */
/*****************************************************************************/
int save_config(void)
{
#if 0
	in version 2.40
	GtkWidget * md_err;
	GError * err = NULL;
	int rc;

	rc = g_key_file_save_to_file(ini_file,STR_KEY_FILE_NAME,&err);
	if(rc != TRUE){
		md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,"Несмог сохранить файл конфигурации : %s",err->message);
		gtk_dialog_run(GTK_DIALOG(md_err));
		gtk_widget_destroy(md_err);
		g_error_free(err);
		return FAILURE;
	}
/*TODO сделать запись в файл*/
GError * err = NULL;
gsize size;
char * buffer =g_key_file_to_data (ini_file,&size,&err);/*записывает Ini файл в буффер*/

#endif
	return SUCCESS;
}

int init_config(void)
{
	int rc;
	GtkWidget * md_err;
	GError * err = NULL;

	ini_file = g_key_file_new();
	rc = g_key_file_load_from_file(ini_file,STR_KEY_FILE_NAME,G_KEY_FILE_NONE,&err);
	if(rc == FALSE){
		md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK
		                              ,"Нет файла конфигурации %s \n %s",STR_KEY_FILE_NAME,err->message);
		gtk_dialog_run(GTK_DIALOG(md_err));
		gtk_widget_destroy (md_err);
		g_critical("%s : %s",STR_KEY_FILE_NAME,err->message);
		exit(0);
	}
	return SUCCESS;
}

int deinit_config(void)
{
	g_key_file_free(ini_file);
	ini_file = NULL;
	return SUCCESS;
}

/*****************************************************************************/
/*     система логирования                                                   */
/*****************************************************************************/
static GIOChannel * logging_channel = NULL;
static GString * logging = NULL;
static GTimeVal current_time;
static char STR_CURRENT_TIME[] = "\n[ %02d.%02d.%04d %02d:%02d:%02d ] ";
static char STR_ERROR[]    = " ОШИБКА СИСТЕМЫ ";
static char STR_CRITICAL[] = " ОШИБКА ";
static char STR_WARNING[]  = " ПРЕДУПРЕЖДЕНИЕ ";
static char STR_MESSAGE[]  = " ";
static char STR_INFO[]     = "    ";
static char STR_DEBUG[]    = "debug : ";

static void save_logging(const gchar *log_domain,GLogLevelFlags log_level,
             const gchar *message,gpointer user_data)
{
	GDateTime * p_dt;
	GIOStatus rc;
	gsize bw;
	GError * err;
	char * str_level;

	if(logging_channel != NULL){
		if(log_level != G_LOG_LEVEL_DEBUG){
			if( log_level == G_LOG_LEVEL_MESSAGE){
				str_level = STR_MESSAGE;
				goto set_str_level;
			}
			if(log_level == G_LOG_LEVEL_INFO){
				str_level = STR_INFO;
				goto set_str_level;
			}
			if(log_level == G_LOG_LEVEL_WARNING){
				str_level = STR_WARNING;
				goto set_str_level;
			}
			if(log_level == G_LOG_LEVEL_CRITICAL){
				str_level = STR_CRITICAL;
				goto set_str_level;
			}
			if(log_level == G_LOG_LEVEL_ERROR){
				str_level = STR_ERROR;
				goto set_str_level;
			}
set_str_level:
 			g_get_current_time (&current_time);
			p_dt = g_date_time_new_from_timeval_local(&current_time);
			g_string_printf(logging,STR_CURRENT_TIME
 		         ,g_date_time_get_day_of_month(p_dt)
		         ,g_date_time_get_month(p_dt)
		         ,g_date_time_get_year(p_dt)
		         ,g_date_time_get_hour(p_dt)
		         ,g_date_time_get_minute(p_dt)
		         ,g_date_time_get_second(p_dt));
			g_string_append(logging,str_level);
			g_string_append(logging,message);
			g_date_time_unref(p_dt);
		}
		else{
			g_string_printf(logging,"\n%s%s",STR_DEBUG,message);
		}

		err = NULL;
		rc = g_io_channel_write_chars(logging_channel,logging->str,-1,&bw,&err);
		if(rc != G_IO_STATUS_NORMAL){
			GtkWidget * md_err;
			md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE
			                               ,"Ошибка ведения лога : %s\nЛог закрыт",err->message);
			gtk_dialog_run(GTK_DIALOG(md_err));
			gtk_widget_destroy(md_err);
			g_error_free(err);
			g_io_channel_shutdown(logging_channel,TRUE,NULL);
			g_io_channel_unref(logging_channel);
			logging_channel = NULL;
		}
		if(log_level == G_LOG_LEVEL_ERROR){
			g_io_channel_shutdown(logging_channel,TRUE,NULL);
		}
	}
}

int flush_logging(gpointer ud)
{
	GIOStatus rc;
	GError * err;
	if(logging_channel == NULL){
		return FALSE;
	}
	err = NULL;
	rc = g_io_channel_flush(logging_channel,&err);
	if(rc != G_IO_STATUS_NORMAL){
		GtkWidget * md_err;
		md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE
			                               ,"Ошибка ведения лога : %s\nЛог закрыт",err->message);
		gtk_dialog_run(GTK_DIALOG(md_err));
		gtk_widget_destroy(md_err);
		g_error_free(err);
		g_io_channel_shutdown(logging_channel,TRUE,NULL);
		g_io_channel_unref(logging_channel);
		logging_channel = NULL;
	}
	return TRUE;
}

int time_flush_logging = 60 * MILLISECOND;

int init_logging(void)
{
	GtkWidget * md_err;
	GError * err = NULL;
	const char * name_logging = STR_LOGGING;
	/*TODO проверка на размер лога и архивирование*/
	/*TODO брать имя лога из ini файла */
	logging_channel = g_io_channel_new_file(name_logging,"a",&err);
	if(logging_channel == NULL){
		md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE
		                              ,"Несмог создать систему логирования %s \n %s",STR_LOGGING,err->message);
		gtk_dialog_run(GTK_DIALOG(md_err));
		gtk_widget_destroy (md_err);
		g_critical(" %s : %s",STR_LOGGING,err->message);
		exit(FAILURE);
	}
	g_timeout_add(time_flush_logging,flush_logging,NULL);

	g_log_set_default_handler(save_logging,NULL);
	logging = g_string_new(" ");

	return SUCCESS;
}

int deinit_logging(void)
{
	g_io_channel_shutdown(logging_channel,TRUE,NULL);
	logging_channel = NULL;
	return SUCCESS;
}
/*****************************************************************************/

int layout_widget(GtkWidget * w,GtkAlign ha,GtkAlign va,gboolean he,gboolean ve)
{
	gtk_widget_set_halign(w,ha);
	gtk_widget_set_valign(w,va);
	gtk_widget_set_hexpand(w,he);
	gtk_widget_set_vexpand(w,ve);
	return SUCCESS;
}

/*****************************************************************************/
GtkWidget * win_main = NULL;
GtkAccelGroup * accgro_main = NULL;

void unrealaze_window_main(GtkWidget * w,gpointer ud)
{
}

void destroy_window_main(GtkWidget * w,gpointer ud)
{
	deinit_control_device();
	deinit_video_stream();
	deinit_db();
	g_message("Останов системы\n");
	deinit_config();
	deinit_logging();
	gtk_main_quit();
}

#define MAIN_SPACING       3

int create_window_main(void)
{
	GtkWidget * vbox = NULL;
	GtkWidget * wtemp;
/*
	GError * err = NULL;
	GdkPixbuf * icon = NULL;
	icon = gdk_pixbuf_new_from_file(STR_NAME_ICON,&err);
	if(err != NULL){
		g_critical("%s",err->message);
		g_error_free(err);
	}
	else{
		gtk_window_set_default_icon(icon);
	}
*/
	accgro_main = gtk_accel_group_new();

	win_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(win_main),MAIN_SPACING);
	gtk_window_set_title(GTK_WINDOW(win_main),STR_NAME_PROGRAMM);
	gtk_window_set_resizable(GTK_WINDOW(win_main),FALSE);
	gtk_window_set_position (GTK_WINDOW(win_main),GTK_WIN_POS_CENTER);
	g_signal_connect(win_main,"destroy",G_CALLBACK(destroy_window_main), NULL);
	g_signal_connect(win_main,"unrealize",G_CALLBACK(unrealaze_window_main),NULL);
	gtk_window_add_accel_group(GTK_WINDOW(win_main),accgro_main);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,MAIN_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),MAIN_SPACING);

	wtemp = create_menu_main();
	gtk_box_pack_start(GTK_BOX(vbox),wtemp,FALSE,TRUE,MAIN_SPACING);
	wtemp = create_status_device();
	gtk_box_pack_start(GTK_BOX(vbox),wtemp,TRUE,TRUE,MAIN_SPACING);
	wtemp = create_video_stream();
	gtk_box_pack_start(GTK_BOX(vbox),wtemp,TRUE,TRUE,MAIN_SPACING);
	wtemp = create_control_panel();
	gtk_box_pack_start(GTK_BOX(vbox),wtemp,FALSE,TRUE,MAIN_SPACING);

	gtk_container_add(GTK_CONTAINER(win_main),vbox);

	gtk_widget_show(vbox);
	gtk_widget_show(win_main);

	return SUCCESS;
}
/*****************************************************************************/
int main(int argc,char * argv[])
{
	gtk_init(&argc,&argv);

	temp_string = g_string_new(NULL);

	init_config();
	init_logging();
	g_message("Запуск системы : %s",STR_NAME_PROGRAMM);
	read_config_device();
	init_db();

	create_window_main();

	gtk_main();

	return SUCCESS;
}
/*****************************************************************************/
