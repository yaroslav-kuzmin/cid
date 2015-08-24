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

/*TODO перенести в файл базы данных*/
char TEMP_JOB_0[] = "Изделие 000";
#define TEMP_PRESSURE           2
#define TEMP_HOUR               0
#define TEMP_MINUTE             10
#define TEMP_SECOND             22
#define TEMP_UPRISE_ANGEL       35
#define TEMP_LOWERING_ANGLE     5

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
	GError * err;
/*	char * str_level; TODO сделать обработку уровней/
	int exit = NOT_OK;
	switch(log_level){
		case G_LOG_LEVEL_ERROR:{
		case G_LOG_LEVEL_CRITICAL:
			exit = OK;
			break;
		}
	}
*/
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
		err = NULL;
		rc = g_io_channel_write_chars(log_file
				,STR_CURRENT_TIME,LEN_STR_CURRENT_TIME
 				,&bw,&err);
		if(rc != G_IO_STATUS_NORMAL){
 		 	/*TODO*/;
			g_error_free(err);
			err = NULL;
		}
		rc = g_io_channel_write_chars(log_file
		,message,-1,&bw,&err);
		if(rc != G_IO_STATUS_NORMAL){
 		 	/*TODO*/;
			g_error_free(err);
		}
	}
}

int init_log(void)
{
	GError * err = NULL;
	log_file = g_io_channel_new_file (STR_LOG_FILE,"a",&err);
	if(log_file == NULL){
		g_message("%s",err->message);
		exit(FAILURE);
	}
	g_log_set_default_handler(save_file,NULL);
	return SUCCESS;
}
/*****************************************************************************/
int init_config(void)
{
	int rc;
	ini_file = g_key_file_new();
	GError * err = NULL;
	rc = g_key_file_load_from_file(ini_file,STR_KEY_FILE_NAME,G_KEY_FILE_NONE,&err);
	if(rc == FALSE){
		g_message("%s",err->message);
		g_error_free(err);
		g_io_channel_shutdown(log_file,TRUE,NULL);
		exit(0);
	}
	return SUCCESS;
}
/*****************************************************************************/
void load_job(GtkButton * b,gpointer d)
{
	g_message("Загрузить работу");
}
void create_job(GtkButton * b,gpointer d)
{
	g_message("Cоздать работу");
}

char STR_LOAD_JOB[] = "загрузить работу";
char STR_CREATE_JOB[] = "создать работу";

GtkWidget * job_panel = NULL;

#define JOB_PANEL_SPACING      1

int create_job_panel(void)
{
	GtkWidget * b_load_job;
	GtkWidget * b_create_job;

	job_panel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,JOB_PANEL_SPACING);

	b_load_job = gtk_button_new_with_label(STR_LOAD_JOB);
	g_signal_connect(b_load_job,"clicked",G_CALLBACK(load_job),NULL);

	b_create_job = gtk_button_new_with_label(STR_CREATE_JOB);
	g_signal_connect(b_create_job,"clicked",G_CALLBACK(create_job),NULL);

	gtk_box_pack_start(GTK_BOX(job_panel),b_load_job,TRUE,TRUE,JOB_PANEL_SPACING);
	gtk_box_pack_start(GTK_BOX(job_panel),b_create_job,TRUE,TRUE,JOB_PANEL_SPACING);

	gtk_widget_show(b_load_job);
	gtk_widget_show(b_create_job);
	gtk_widget_show(job_panel);
	return SUCCESS;
}
/*****************************************************************************/

char STR_INFO_PANEL[] = "ИНФОРМАЦИЯ";

GtkWidget * info_panel = NULL;
GString * s_name_job_info_panel = NULL;
GtkWidget * w_name_job_info_panel = NULL;

#define INFO_SPACING            3

int create_info_panel(void)
{
	GtkWidget * vbox;
	PangoContext * pc_label;
	PangoFontDescription * pfd_label;
	GdkRGBA color_label;

	/*info_panel = gtk_frame_new(STR_INFO_PANEL);*/
	info_panel = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(info_panel),INFO_SPACING);
	/*gtk_frame_set_label_align (GTK_FRAME(info_panel),0.5,0);*/

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,INFO_SPACING);

	s_name_job_info_panel= g_string_new(NULL);
	g_string_append(s_name_job_info_panel,TEMP_JOB_0);
	w_name_job_info_panel = gtk_label_new(s_name_job_info_panel->str);
	pc_label = gtk_widget_get_pango_context(w_name_job_info_panel);
	pfd_label = pango_context_get_font_description(pc_label);
	pango_font_description_set_size(pfd_label,30000);
	gtk_widget_override_font(w_name_job_info_panel,pfd_label);
	color_label.red = 0;
	color_label.green = 0;
	color_label.blue = 1;
	color_label.alpha = 1;
	gtk_widget_override_color(w_name_job_info_panel,GTK_STATE_FLAG_NORMAL,&color_label);

	gtk_container_add(GTK_CONTAINER(info_panel),vbox);
	gtk_box_pack_start(GTK_BOX(vbox),w_name_job_info_panel,TRUE,TRUE,INFO_SPACING);

	gtk_widget_show(w_name_job_info_panel);
	gtk_widget_show(vbox);
	gtk_widget_show(info_panel);

	return SUCCESS;
}
/*****************************************************************************/
GtkWidget * video_stream = NULL;

#define DEFAULT_VIDEO_WIDTH      720
#define DEFAULT_VIDEO_HEIGHT     576

int create_video_stream(void)
{
	GError * err = NULL;
	GdkPixbuf * image;

	video_stream = gtk_image_new();
	gtk_widget_set_size_request(video_stream,DEFAULT_VIDEO_WIDTH,DEFAULT_VIDEO_HEIGHT);

	image = gdk_pixbuf_new_from_file(STR_NAME_DEFAULT_VIDEO,&err);
	if(err != NULL){
		g_message("%s",err->message);
		g_error_free(err);
	}
	else{
		gtk_image_set_from_pixbuf(GTK_IMAGE(video_stream),image);
	}

	gtk_widget_show(video_stream);

	return SUCCESS;
}
/*****************************************************************************/

void auto_mode(GtkButton * b,gpointer d)
{
	g_message("Автоматическое управление");
}

void manual_mode(GtkButton * b,gpointer d)
{
	g_message("Ручное управление");
}

void auto_start_mode(GtkButton * b,gpointer d)
{
	g_message("Запуск автоматической работы");
}

void auto_stop_mode(GtkButton * b,gpointer d)
{
	g_message("Останов автоматической работы");
}

void auto_pause_mode(GtkButton * b,gpointer d)
{
	g_message("Пауза автоматической работы");
}

char STR_BUTTON_AUTO[] = "Автоматическое управление";
char STR_BUTTON_AUTO_START[] = "СТАРТ";
char STR_BUTTON_AUTO_STOP[]  = "СТОП";
char STR_BUTTON_AUTO_PAUSE[] = "ПАУЗА";
char STR_BUTTON_MANUAL[] = "Ручное управление";

#define CONTROL_BUTTON_SPACING        1

GtkWidget * create_control_button(void)
{
	GtkWidget * cgrid;

	GtkWidget * hboxb;
	GtkWidget * b_auto;
	GtkWidget * b_manual;

	GtkWidget * b_auto_start;
	GtkWidget * b_auto_stop;
	GtkWidget * b_auto_pause;

	cgrid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(cgrid),CONTROL_BUTTON_SPACING);

	hboxb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,CONTROL_BUTTON_SPACING);
	gtk_box_set_homogeneous (GTK_BOX(hboxb),TRUE);

	b_auto = gtk_radio_button_new_with_label(NULL,STR_BUTTON_AUTO);
	g_signal_connect(b_auto,"clicked",G_CALLBACK(auto_mode),NULL);
	b_manual = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(b_auto)
	                                                      ,STR_BUTTON_MANUAL);
	g_signal_connect(b_manual,"clicked",G_CALLBACK(manual_mode),NULL);
	/*gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b_manual),TRUE);*/

	b_auto_start = gtk_button_new_with_label(STR_BUTTON_AUTO_START);
	g_signal_connect(b_auto_start,"clicked",G_CALLBACK(auto_start_mode),NULL);

	b_auto_stop = gtk_button_new_with_label(STR_BUTTON_AUTO_STOP);
	g_signal_connect(b_auto_stop,"clicked",G_CALLBACK(auto_stop_mode),NULL);

	b_auto_pause = gtk_button_new_with_label(STR_BUTTON_AUTO_PAUSE);
	g_signal_connect(b_auto_pause,"clicked",G_CALLBACK(auto_pause_mode),NULL);


	gtk_grid_attach(GTK_GRID(cgrid),hboxb,0,0,3,1);
	gtk_box_pack_start(GTK_BOX(hboxb),b_auto,TRUE,TRUE,CONTROL_BUTTON_SPACING);
	gtk_box_pack_start(GTK_BOX(hboxb),b_manual,TRUE,TRUE,CONTROL_BUTTON_SPACING);
	gtk_grid_attach(GTK_GRID(cgrid),b_auto_start,0,1,1,1);
	gtk_grid_attach(GTK_GRID(cgrid),b_auto_stop,0,2,1,1);
	gtk_grid_attach(GTK_GRID(cgrid),b_auto_pause,0,3,1,1);

	gtk_widget_show(b_auto_pause);
	gtk_widget_show(b_auto_stop);
	gtk_widget_show(b_auto_start);
	gtk_widget_show(b_manual);
	gtk_widget_show(b_auto);
	gtk_widget_show(hboxb);
	gtk_widget_show(cgrid);

	return cgrid;
}

/**************************************/

char STR_SET_VALUE[] =     " Установлено ";
char STR_CURRENT_VALUE[] = " Текущие     ";
char STR_PRESSURE[] = "Рабочие давление, атм";
char STR_TIME_JOB[] = "Время работы";
char STR_UPRISE_ANGEL[] =   "Угол подъема, градусов";
char STR_LOWERING_ANGEL[] = "Угол опускания, градусов";

GtkWidget * w_pressure_set = NULL;
GtkWidget * w_pressure_current = NULL;
GtkWidget * w_time_set = NULL;
GtkWidget * w_time_current = NULL;
GtkWidget * w_uprise_set = NULL;
GtkWidget * w_uprise_current = NULL;
GtkWidget * w_lowering_set = NULL;
GtkWidget * w_lowering_current = NULL;
GString * s_pressure_set = NULL;
GString * s_pressure_current = NULL;
GString * s_time_set = NULL;
GString * s_time_current = NULL;
GString * s_uprise_set = NULL;
GString * s_uprise_current = NULL;
GString * s_lowering_set = NULL;
GString * s_lowering_current = NULL;

#define SET_PANEL_SPACING           1

GtkWidget * create_set_panel(void)
{
	GtkWidget * igrid;
	GtkWidget * set_value;
	GtkWidget * current_value;
	GtkWidget * pressure;
	GtkWidget * time;
	GtkWidget * uprise;
	GtkWidget * lowering;

	igrid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(igrid),SET_PANEL_SPACING);

	set_value = gtk_label_new(STR_SET_VALUE);
	current_value = gtk_label_new(STR_CURRENT_VALUE);

	pressure = gtk_label_new(STR_PRESSURE);
	gtk_widget_set_halign(pressure,GTK_ALIGN_START);
	s_pressure_set = g_string_new(NULL);
	g_string_append_printf(s_pressure_set," %d ",TEMP_PRESSURE);
	w_pressure_set = gtk_label_new(s_pressure_set->str);
	s_pressure_current = g_string_new(NULL);
	g_string_append_printf(s_pressure_current," %d ",TEMP_PRESSURE);
	w_pressure_current = gtk_label_new(s_pressure_current->str);

	time = gtk_label_new(STR_TIME_JOB);
	gtk_widget_set_halign(time,GTK_ALIGN_START);
	s_time_set = g_string_new(NULL);
	g_string_append_printf(s_time_set," %02d:%02d:%02d",TEMP_HOUR,TEMP_MINUTE,TEMP_SECOND);
	w_time_set = gtk_label_new(s_time_set->str);
	s_time_current = g_string_new(NULL);
	g_string_append_printf(s_time_current," %02d:%02d:%02d",TEMP_HOUR,TEMP_MINUTE-2,TEMP_SECOND-2);
	w_time_current = gtk_label_new(s_time_current->str);

	uprise = gtk_label_new(STR_UPRISE_ANGEL);
	gtk_widget_set_halign(uprise,GTK_ALIGN_START);
	s_uprise_set = g_string_new(NULL);
	g_string_append_printf(s_uprise_set," %d ",TEMP_UPRISE_ANGEL);
	w_uprise_set = gtk_label_new(s_uprise_set->str);
	s_uprise_current = g_string_new(NULL);
	g_string_append_printf(s_uprise_current," %d ",TEMP_UPRISE_ANGEL - 10);
	w_uprise_current = gtk_label_new(s_uprise_current->str);

	lowering = gtk_label_new(STR_LOWERING_ANGEL);
	gtk_widget_set_halign(lowering,GTK_ALIGN_START);
	s_lowering_set = g_string_new(NULL);
	g_string_append_printf(s_lowering_set," %d ",TEMP_LOWERING_ANGLE);
	w_lowering_set = gtk_label_new(s_lowering_set->str);
	s_lowering_current = g_string_new(NULL);
	g_string_append_printf(s_lowering_current," %d ",TEMP_UPRISE_ANGEL - 10);
	w_lowering_current = gtk_label_new(s_lowering_current->str);

	gtk_grid_attach(GTK_GRID(igrid),set_value         ,2,0,1,1);
	gtk_grid_attach(GTK_GRID(igrid),current_value     ,3,0,1,1);

	gtk_grid_attach(GTK_GRID(igrid),pressure          ,0,1,2,1);
	gtk_grid_attach(GTK_GRID(igrid),w_pressure_set    ,2,1,1,1);
	gtk_grid_attach(GTK_GRID(igrid),w_pressure_current,3,1,1,1);

	gtk_grid_attach(GTK_GRID(igrid),time              ,0,2,2,1);
	gtk_grid_attach(GTK_GRID(igrid),w_time_set        ,2,2,1,1);
	gtk_grid_attach(GTK_GRID(igrid),w_time_current    ,3,2,1,1);

	gtk_grid_attach(GTK_GRID(igrid),uprise            ,0,3,2,1);
	gtk_grid_attach(GTK_GRID(igrid),w_uprise_set      ,2,3,1,1);
	gtk_grid_attach(GTK_GRID(igrid),w_uprise_current  ,3,3,1,1);

	gtk_grid_attach(GTK_GRID(igrid),lowering          ,0,4,2,1);
	gtk_grid_attach(GTK_GRID(igrid),w_lowering_set    ,2,4,1,1);
	gtk_grid_attach(GTK_GRID(igrid),w_lowering_current,3,4,1,1);

	gtk_widget_show(w_lowering_current);
	gtk_widget_show(w_lowering_set);
	gtk_widget_show(lowering);
	gtk_widget_show(w_uprise_current);
	gtk_widget_show(w_uprise_set);
	gtk_widget_show(uprise);
	gtk_widget_show(w_time_current);
	gtk_widget_show(w_time_set);
	gtk_widget_show(time);
	gtk_widget_show(w_pressure_current);
	gtk_widget_show(w_pressure_set);
	gtk_widget_show(pressure);
	gtk_widget_show(current_value);
	gtk_widget_show(set_value);
	gtk_widget_show(igrid);

/*
	gtk_widget_show(b_create_job);
	gtk_widget_show(b_load_job);
	gtk_widget_show(vboxb);
*/
	return igrid;
}
/**************************************/
char STR_CONTROL_PANEL[] = "УПРАВЛЕНИЕ";

GtkWidget * control_panel = NULL;

#define CONTROL_SPACING        1

int create_control_panel(void)
{
	GtkWidget * hboxm;
	GtkWidget * cbutton;
	GtkWidget * spanel;

	control_panel = gtk_frame_new(STR_CONTROL_PANEL);
	gtk_container_set_border_width(GTK_CONTAINER(control_panel),CONTROL_SPACING);
	gtk_frame_set_label_align (GTK_FRAME(control_panel),0.5,0);

	hboxm = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,CONTROL_SPACING);

	cbutton = create_control_button();
	spanel = create_set_panel();

	gtk_container_add(GTK_CONTAINER(control_panel),hboxm);
	gtk_box_pack_start(GTK_BOX(hboxm),cbutton,TRUE,TRUE,CONTROL_SPACING);
	gtk_box_pack_start(GTK_BOX(hboxm),spanel,TRUE,TRUE,CONTROL_SPACING);
	gtk_widget_show(hboxm);
	gtk_widget_show(control_panel);
	return SUCCESS;
}

/*****************************************************************************/
GtkWidget * main_window = NULL;

void main_destroy(GtkWidget * w,gpointer ud)
{
	g_key_file_free(ini_file);

	g_message("Останов системы\n");
	g_io_channel_shutdown(log_file,TRUE,NULL);
	gtk_main_quit();
}

#define MAIN_SPACING       3

int create_main_window(void)
{
	GError * err = NULL;
	GdkPixbuf * icon = NULL;
	GtkWidget * vbox = NULL;

	icon = gdk_pixbuf_new_from_file(STR_NAME_ICON,&err);
	if(err != NULL){
		g_message("%s",err->message);
		g_error_free(err);
	}
	else{
		gtk_window_set_default_icon(icon);
	}

	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(main_window),MAIN_SPACING);
	gtk_window_set_title(GTK_WINDOW(main_window),STR_NAME_PROGRAMM);
	/*gtk_window_set_default_size(GTK_WINDOW(main_window),850,-1);*/
	gtk_window_set_resizable(GTK_WINDOW(main_window),FALSE);
	gtk_window_set_position (GTK_WINDOW(main_window),GTK_WIN_POS_CENTER);
	g_signal_connect (main_window, "destroy",G_CALLBACK (main_destroy), NULL);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,MAIN_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),MAIN_SPACING);

	create_job_panel();
	create_info_panel();
	create_video_stream();
	create_control_panel();

	gtk_box_pack_start(GTK_BOX(vbox),job_panel,TRUE,TRUE,MAIN_SPACING);
	gtk_box_pack_start(GTK_BOX(vbox),info_panel,TRUE,TRUE,MAIN_SPACING);
	gtk_box_pack_start(GTK_BOX(vbox),video_stream,TRUE,TRUE,MAIN_SPACING);
	gtk_box_pack_start(GTK_BOX(vbox),control_panel,TRUE,TRUE,MAIN_SPACING);

	gtk_container_add(GTK_CONTAINER(main_window),vbox);

	gtk_widget_show(vbox);
	gtk_widget_show(main_window);

	return SUCCESS;
}
/*****************************************************************************/
int main(int argc,char * argv[])
{
	init_log();

	g_message("Запуск системы");
	g_message("%s",STR_NAME_PROGRAMM);

	init_config();

	gtk_init(&argc,&argv);

	create_main_window();

	gtk_main();

	return SUCCESS;
}
/*****************************************************************************/
