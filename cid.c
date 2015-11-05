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
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "version.h"
#include "total.h"
#include "job.h"
#include "video.h"
#include "control.h"

/*****************************************************************************/

/*****************************************************************************/
/*  конфиурирование системы                                                  */
/*****************************************************************************/
#if 0
static int save_config(void)
{
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
	err = NULL;
	g_key_file_to_data (ini_file,&size,&err);/*записывает Ini файл в буффер*/
	return SUCCESS;
}
#endif

static int init_config(void)
{
	int rc;
	GError * err = NULL;

	ini_file = g_key_file_new();
	rc = g_key_file_load_from_file(ini_file,STR_KEY_FILE_NAME,G_KEY_FILE_NONE,&err);
	if(rc == FALSE){
		GtkWidget * md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK
		                                           ,"Нет файла конфигурации %s \n %s",STR_KEY_FILE_NAME,err->message);
		gtk_dialog_run(GTK_DIALOG(md_err));
		gtk_widget_destroy (md_err);
		g_critical("%s : %s!",STR_KEY_FILE_NAME,err->message);
		exit(0);
	}
	return SUCCESS;
}

static int deinit_config(void)
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

static int flush_logging(gpointer ud)
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

#define MAX_LOGGING_FILE        0x800000

off_t max_logging_file = MAX_LOGGING_FILE;

static int check_size_log(const char * name)
{
	int rc;
	GStatBuf info;

	rc = g_file_test(name,G_FILE_TEST_IS_REGULAR);
	if(rc == FALSE){
		return SUCCESS;
	}
	else{
		rc = g_stat(name,&info);
		if(rc != 0){
			return FAILURE;
		}
		if(info.st_size > max_logging_file){
			g_remove(name);
		}
	}
	return SUCCESS;
}

#define DEFAULT_TIMEOUT_FLUSH_LOGGING    60

static int timeout_flush_logging = DEFAULT_TIMEOUT_FLUSH_LOGGING * MILLISECOND;

static int init_logging(void)
{
	GtkWidget * md_err;
	GError * err = NULL;
	const char * name_logging = STR_LOGGING;

	check_size_log(name_logging);

	logging_channel = g_io_channel_new_file(name_logging,"a",&err);
	if(logging_channel == NULL){
		md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE
		                               ,"Несмог создать систему логирования %s \n %s",STR_LOGGING,err->message);
		gtk_dialog_run(GTK_DIALOG(md_err));
		gtk_widget_destroy (md_err);
		g_critical(" %s : %s!",STR_LOGGING,err->message);
		exit(FAILURE);
	}
	g_timeout_add(timeout_flush_logging,flush_logging,NULL);

	g_log_set_default_handler(save_logging,NULL);
	logging = g_string_new(" ");

	return SUCCESS;
}

static int deinit_logging(void)
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

int set_size_font(GtkWidget * w,int size)
{
	PangoContext * pancon_info;
	PangoFontDescription * panfondes_info;

	pancon_info = gtk_widget_get_pango_context(w);
	panfondes_info = pango_context_get_font_description(pancon_info);
	pango_font_description_set_size(panfondes_info,size);
	gtk_widget_override_font(w,panfondes_info);

	return SUCCESS;
}

/*****************************************************************************/
GtkWidget * win_main = NULL;
GtkAccelGroup * accgro_main = NULL;

static void unrealaze_window_main(GtkWidget * w,gpointer ud)
{
}

static void destroy_window_main(GtkWidget * w,gpointer ud)
{
	deinit_control_device();
	deinit_video();
	deinit_db();
	g_message("Останов системы.\n");
	deinit_config();
	deinit_logging();
	gtk_main_quit();
}

static GdkPixbuf * default_icon = NULL;

static int about_programm(GdkPixbuf * icon)
{
	GtkWidget * dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog),STR_NAME_PROGRAMM);
	g_string_printf(temp_string,"%d.%03d - %x",VERSION_MAJOR,VERSION_MINOR,VERSION_GIT);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog),temp_string->str);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog),STR_COPYRIGHT);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),STR_COMMENT);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog),STR_LICENSE);
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog),GTK_LICENSE_CUSTOM);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),NULL);
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog),STR_EMAIL_LABEL);
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog),STR_AUTHORS);
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(dialog),STR_AUTHORS);
	gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(dialog),NULL);
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog),NULL);
	if(icon != NULL){
		gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog),icon);
	}
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	return SUCCESS;
}

static gboolean key_press_event_window_main(GtkWidget * w,GdkEvent  *event,gpointer ud)
{
	GdkEventType type = event->type;
	gint state;

	if(type == GDK_KEY_PRESS){
		GdkEventKey * event_key = (GdkEventKey*)event;
		state = event_key->state;
		if( (state & GDK_SHIFT_MASK) && (state & GDK_CONTROL_MASK)){
			if( event_key->keyval == GDK_KEY_A){
				about_programm((GdkPixbuf*)ud);
			}
		}
	}
	return FALSE;
}

#define MAIN_SPACING       3

static char RESOURCE_DEFAULT_ICON[] = "/cid/resource/cid.png";

static int create_window_main(void)
{
	GtkWidget * vbox = NULL;
	GtkWidget * wtemp;

	accgro_main = gtk_accel_group_new();
	default_icon = gdk_pixbuf_new_from_resource(RESOURCE_DEFAULT_ICON,NULL);
	if(default_icon != NULL){
		gtk_window_set_default_icon(default_icon);
	}

	win_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(win_main),MAIN_SPACING);
	gtk_window_set_title(GTK_WINDOW(win_main),STR_NAME_PROGRAMM);
	gtk_window_set_resizable(GTK_WINDOW(win_main),FALSE);
	gtk_window_set_position (GTK_WINDOW(win_main),GTK_WIN_POS_CENTER);
	g_signal_connect(win_main,"destroy",G_CALLBACK(destroy_window_main), NULL);
	g_signal_connect(win_main,"unrealize",G_CALLBACK(unrealaze_window_main),NULL);
	g_signal_connect(win_main,"key-press-event",G_CALLBACK(key_press_event_window_main),default_icon);
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
	g_message("Запуск системы : %s.",STR_NAME_PROGRAMM);
	init_db();

	create_window_main();

	gtk_main();

	return SUCCESS;
}
/*****************************************************************************/
