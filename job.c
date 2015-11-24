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
/*#include <glib/gprintf.h> TODO test only*/
#include <gtk/gtk.h>
#include <sqlite3.h>

#include <modbus.h>
#include "total.h"
#include "cid.h"
#include "video.h"
#include "control.h"


/*****************************************************************************/
/* Общие переменые                                                           */
/*****************************************************************************/
#define MIN_RATE_PRESSURE              0.0059
#define MAX_RATE_PRESSURE              0.0069
#define DEFAULT_RATE_PRESSURE          0.0062
double rate_pressure = DEFAULT_RATE_PRESSURE;
#define PRINTF_PRESSURE(p)             ((double)(p)*rate_pressure)
#define FORMAT_PRESSURE                "%.1f"

#define MAX_TIME_SECOND                359999
/*максимально часов  99*/
/*максимально минут  59*/
/*максимально секунд 59*/

#define RATE_ANGLE                     0.126
#define PRINTF_ANGLE(a)                ((double)(a)*RATE_ANGLE)
#define FORMAT_ANGLE                   "%.1f"
#define MAX_LEN_STR_ANGLE              5

#define DEFAULT_SPEED_VERTICAL_MANUAL_MODE         MIN_SPEED_VERTICAL_IN_TIC
#define DEFAULT_SPEED_VERTICAL_AUTO_MODE           MAX_SPEED_VERTICAL_IN_TIC
#define DEFAULT_SPEED_VERTICAL_SAVE_JOB            (MAX_SPEED_VERTICAL_IN_TIC/2)

#define MIN_SPEED_VERTICAL                  0
#define MAX_SPEED_VERTICAL                  6
#define STEP_SPEED_VERTICAL                 0.75
#define RATE_SPEED_VERTICAL                 666.66667

#define DEFAULT_HORIZONTAL_OFFSET           MIN_HORIZONTAL_OFFSET_IN_TIC

#define ANGLE_NULL                          -1
#define MAX_ANGLE                           88.0
#define MIN_ANGLE                           0.0

/*****************************************************************************/
/*****************************************************************************/
/*  работа с базой данных                                                    */
/*****************************************************************************/
/*****************************************************************************/

static GHashTable * list_job = NULL;
static sqlite3 * database = NULL;

static const char QUERY_JOB_TABLE[] = "SELECT * FROM sqlite_master WHERE type = 'table'";
static const char QUERY_CREATE_JOB_TABLE[] =
"CREATE TABLE job(name PRIMARY KEY,pressure INTEGER,time INTEGER,uprise INTEGER,lowering INTEGER)";
static const char QUERY_ALL_JOB[] = "SELECT * FROM job";
static char QUIER_INSERT_ROW_JOB[] = "INSERT INTO job VALUES (";
#define DEFAULT_AMOUNT_COLUMN        5

#define NUMBER_NAME_COLUMN           0
#define NUMBER_PRESSURE_COLUMN       1
#define NUMBER_TIME_COLUMN           2
#define NUMBER_UPRISE_COLUMN         3
#define NUMBER_LOWERING_COLUMN       4

static const char STR_NAME_COLUMN_0[] = "name";
#define SIZE_STR_NAME_COLUMN_0           4
static const char STR_NAME_COLUMN_1[] = "pressure";
#define SIZE_STR_NAME_COLUMN_1           8
static const char STR_NAME_COLUMN_2[] = "time";
#define SIZE_STR_NAME_COLUMN_2           4
static const char STR_NAME_COLUMN_3[] = "uprise";
#define SIZE_STR_NAME_COLUMN_3           6
static const char STR_NAME_COLUMN_4[] = "lowering";
#define SIZE_STR_NAME_COLUMN_4           8

typedef struct _job_s job_s;
struct _job_s
{
	GString * name;
	uint16_t pressure;
	unsigned int time;
	int uprise;
	int lowering;
};

static guint hash_job(gconstpointer job)
{
	job_s * j = (job_s*)job;
	return g_string_hash(j->name);
}

static gboolean equal_job(gconstpointer a,gconstpointer b)
{
	job_s * p_a = (job_s*)a;
	job_s * p_b = (job_s*)b;
	GString * s_a = p_a->name;
	GString * s_b = p_b->name;
	return g_string_equal(s_a,s_b);
}

static void destroy_job(gpointer job)
{
	job_s *j = (job_s*)job;
	GString *s = j->name;
	g_string_free(s,TRUE);
	g_slice_free1(sizeof(job_s),j);
}

static int fill_list_job_db(void)
{
	int rc;
	sqlite3_stmt * query;
	int check_column = NOT_OK;
	job_s * job;

	list_job = g_hash_table_new_full(hash_job,equal_job,destroy_job,NULL);

	rc = sqlite3_prepare_v2(database,QUERY_ALL_JOB,-1,&query,NULL);
	if(rc != SQLITE_OK){
		g_critical("SQL error QUERY_ALL_JOB : %s!",sqlite3_errmsg(database));
		return FAILURE;
	}
	for(;;){
		rc = sqlite3_step(query);
		if(rc == SQLITE_DONE){
			break;	/* данных в запросе нет*/
		}
		if(rc == SQLITE_ERROR ){
			g_critical("SQL error QUERY_ALL_JOB step : %s!",sqlite3_errmsg(database));
			break;
		}
		if(rc == SQLITE_ROW){
			const char * str;

			if(check_column != OK){
				int amount_column;
				amount_column = sqlite3_data_count(query);
				if(amount_column != DEFAULT_AMOUNT_COLUMN){
					g_critical("SQL error not correct table job : %d!",amount_column);
					break;
				}
				str = sqlite3_column_name(query,0);
				str = g_strstr_len(STR_NAME_COLUMN_0,SIZE_STR_NAME_COLUMN_0,str);
				if(str == NULL){
					g_critical("SQL error not correct column 0!");
					break;
				}
				str = sqlite3_column_name(query,1);
				str = g_strstr_len(STR_NAME_COLUMN_1,SIZE_STR_NAME_COLUMN_1,str);
				if(str == NULL){
					g_critical("SQL error not correct column 1!");
					break;
				}
				str = sqlite3_column_name(query,2);
				str = g_strstr_len(STR_NAME_COLUMN_2,SIZE_STR_NAME_COLUMN_2,str);
				if(str == NULL){
					g_critical("SQL error not correct column 2!");
					break;
				}
				str = sqlite3_column_name(query,3);
				str = g_strstr_len(STR_NAME_COLUMN_3,SIZE_STR_NAME_COLUMN_3,str);
				if(str == NULL){
					g_critical("SQL error not correct column 3!");
					break;
				}
				str = sqlite3_column_name(query,4);
				str = g_strstr_len(STR_NAME_COLUMN_4,SIZE_STR_NAME_COLUMN_4,str);
				if(str == NULL){
					g_critical("SQL error not correct column 4!");
					break;
				}
				check_column = OK;
			}
			job = g_slice_alloc0(sizeof(job_s));
			str = (char *)sqlite3_column_text(query,NUMBER_NAME_COLUMN);
			job->name = g_string_new(str);
			job->pressure = sqlite3_column_int64(query,NUMBER_PRESSURE_COLUMN);
			if( (job->pressure < MIN_PRESSURE_IN_TIC) || (job->pressure > MAX_PRESSURE_IN_TIC)){
				job->pressure = DEFAULT_PRESSURE_IN_TIC;
			}
			job->time = sqlite3_column_int64(query,NUMBER_TIME_COLUMN);
			if(job->time > MAX_TIME_SECOND){
				job->time = MAX_TIME_SECOND;
			}
			job->uprise = sqlite3_column_int64(query,NUMBER_UPRISE_COLUMN);
			if((job->uprise < MIN_ANGLE_IN_TIC) || (job->uprise > MAX_ANGLE_IN_TIC )){
				job->uprise = MAX_ANGLE_IN_TIC;
			}
			job->lowering = sqlite3_column_int64(query,NUMBER_LOWERING_COLUMN);
			if((job->lowering < MIN_ANGLE_IN_TIC) || (job->lowering > MAX_ANGLE_IN_TIC )){
				job->uprise = MIN_ANGLE_IN_TIC;
			}
			g_hash_table_add(list_job,job);
			continue;
		}
		g_critical("SQL error QUERY_ALL_JOB end!");
		break;
	}
	sqlite3_finalize(query);
	return SUCCESS;
}

static int str_to_time_second(const char * str)
{
	gint hour = 0;
	gint minut = 0;
	gint second = 0;
	gint amount = 0;

	/*str 00:00:00*/
	if((str[2] != ':') || (str[5] != ':')){
		return amount;
	}

	hour = str[0] - '0';
	hour *= 10;
	hour += (str[1] - '0');
	if( (hour < 0) || (hour > 99) ){
		return amount;
	}

	minut = str[3] - '0';
	minut *= 10;
	minut += (str[4] - '0');
	if((minut < 0) || (minut > 59)){
		return amount;
	}
	second = str[6] - '0';
	second *= 10;
	second += (str[7] - '0');
	if((second < 0) || (second > 59)){
		return amount;
	}
	amount += ((hour*(60*60)) + (minut*60) + second);
	if(amount == 0){
		return amount;
	}
	return amount;
}

#define MATCH_NAME      -1
#define INVALID_NAME    -2

static job_s temp_job = {0};
static GString * query_row_job = NULL;

/*проверка введеных значений лежит на вышестояшей функции*/
static int insert_job_db(const char * name,const char *pressure,const char * time
              ,long int uprise,long int lowering )
{
	job_s * pj;
	char * sql_error;
	int rc;
	double d;

#if 0
	char * str = NULL;
	/*TODO проверка на некоректные имена*/
	str = g_strstr_len(name,-1," , ");
	if(str != NULL){
		return INVALID_NAME;
	}
#endif

	g_string_truncate(temp_job.name,0);
	g_string_append(temp_job.name,name);

	rc = g_hash_table_contains(list_job,&temp_job);
	if( rc == TRUE){
		return MATCH_NAME;
	}
	pj = g_slice_alloc0(sizeof(job_s));
	pj->name = g_string_new(name);

	d = g_strtod(pressure,NULL);
	d /= rate_pressure;
	pj->pressure = (uint16_t)d;

	pj->time = str_to_time_second(time);
	pj->uprise = uprise;
	pj->lowering = lowering;

	g_hash_table_add(list_job,pj);

	if(query_row_job == NULL){
		query_row_job = g_string_new(QUIER_INSERT_ROW_JOB);
	}
	g_string_printf(query_row_job,"%s",QUIER_INSERT_ROW_JOB);
	g_string_append_printf(query_row_job,"\'%s\',",pj->name->str);
	g_string_append_printf(query_row_job,"%d,",pj->pressure);
	g_string_append_printf(query_row_job,"%d,",pj->time);
	g_string_append_printf(query_row_job,"%d,%d)",pj->uprise,pj->lowering);

	rc = sqlite3_exec(database,query_row_job->str,NULL,NULL,&sql_error);
	if(rc != SQLITE_OK){
		/*TODO окно пользователю что несмог записать*/
		g_critical("SQL error QUERY_INSERT_JOB : %s!",sql_error);
		sqlite3_free(sql_error);
		return FAILURE;
	}

	return SUCCESS;
}
static const char QUERY_DELETE_ROW_JOB[] = "DELETE FROM job WHERE name=";

static int delete_job_db(const char * name)
{
	int rc;
	char * sql_error;

	g_string_truncate(temp_job.name,0);
	g_string_append(temp_job.name,name);

	g_hash_table_remove(list_job,&temp_job);

	if(query_row_job == NULL){
		query_row_job = g_string_new(QUERY_DELETE_ROW_JOB);
	}
	g_string_printf(query_row_job,"%s",QUERY_DELETE_ROW_JOB);
	g_string_append_printf(query_row_job,"\'%s\'",name);

	rc = sqlite3_exec(database,query_row_job->str,NULL,NULL,&sql_error);
	if(rc != SQLITE_OK){
		/*TODO окно пользователю что несмог удалить*/
		g_critical("SQL error QUERY_DELETE_ROW_JOB : %s!",sql_error);
		sqlite3_free(sql_error);
		return FAILURE;
	}
	return SUCCESS;
}

static GHashTableIter iter_job;

static int job_iter_init(void)
{
	g_hash_table_iter_init (&iter_job,list_job);
	return SUCCESS;
}

static job_s * job_iter_next(void)
{
	gpointer key;
	int rc = g_hash_table_iter_next(&iter_job,&key,NULL);
	if(rc != TRUE){
		key = NULL;
	}
	return (job_s *)key;
}

static int table_job = NOT_OK;
static char STR_JOB_TABLE_NAME[] =     "job";
#define SIZE_STR_JOB_TABLE_NAME          3
static char STR_COL_NAME_JOB_TABLE[] = "name";
#define SIZE_STR_COL_NAME_JOB_TABLE      4
static int check_table_job_db(void * ud, int argc, char **argv, char ** col_name)
{
	int i;
	char * name;
	char * value;
	char * check;
	for(i = 0;i < argc;i++){
		name = col_name[i];
		value = argv[i];
		check = NULL;
		check = g_strstr_len(name,SIZE_STR_COL_NAME_JOB_TABLE,STR_COL_NAME_JOB_TABLE);
		if(check != NULL){
			check = g_strstr_len(value,SIZE_STR_JOB_TABLE_NAME,STR_JOB_TABLE_NAME);
			if( check != NULL){
				table_job = OK;
			}
		}
	}
	return 0;
}

int init_db(void)
{
	int rc;

	char * error_message = NULL;
	const char * name_database = STR_NAME_DB;

	temp_job.name = g_string_new(NULL);

	rc = sqlite3_open(name_database,&database); // откравает базу данных
	if(rc != SQLITE_OK){
		char STR_ERROR[] = "Несмог открыть базу данных %s : %s!";
		GtkWidget * md_err = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE
		                                           ,STR_ERROR,name_database,sqlite3_errmsg(database));
		g_critical(STR_ERROR,STR_NAME_DB,sqlite3_errmsg(database));
		gtk_dialog_run(GTK_DIALOG(md_err));
		gtk_widget_destroy(md_err);
		sqlite3_close(database);
		return FAILURE;
	}

	rc = sqlite3_exec(database,QUERY_JOB_TABLE,check_table_job_db,NULL,&error_message);
	if( rc != SQLITE_OK ){
		g_critical("SQL error QUERY_JOB_TABLE : %s!",error_message);
		sqlite3_free(error_message);
		return FAILURE;
	}

	if(table_job == NOT_OK){
		rc = sqlite3_exec(database,QUERY_CREATE_JOB_TABLE,NULL,NULL, &error_message);
		if( rc != SQLITE_OK){
			g_critical("SQL error QUERY_CREATE_JOB_TABLE : %s!",error_message);
			sqlite3_free(error_message);
			return FAILURE;
		}
	}
	g_message("Открыл базу данных.");

	rc = fill_list_job_db();
	if(rc != SUCCESS){
		;/*TODO можно перезаписать базу данных создать диалог */
	}
	return SUCCESS;
}

int deinit_db(void)
{
	g_hash_table_destroy(list_job);
	list_job = NULL;
	if(database != NULL){
		sqlite3_close(database);
		g_message("Закрыл базу данных.");
		database = NULL;
	}

	return SUCCESS;
}

/*****************************************************************************/
/*****************************************************************************/
/*  общие элементы управления                                                */
/*****************************************************************************/
/*****************************************************************************/

static job_s * current_job = NULL;
static GtkWidget * fra_info = NULL;
static GtkWidget * fra_mode_auto = NULL;
static GtkWidget * fra_mode_manual = NULL;
static GtkWidget * fra_job_load = NULL;
static GtkWidget * fra_job_save = NULL;

static char STR_MODE_AUTO[] =   "Автоматическое управление";
static char STR_MODE_MANUAL[] = "Ручное Управление";
static char STR_JOB_LOAD[] =    "Загрузить работу";
static char STR_JOB_SAVE[] =    "Новая работа";

/*****************************************************************************/
/* Общие функции                                                             */
/*****************************************************************************/

#define INFO_FRAME         0
#define LOAD_JOB_FRAME     1
#define SAVE_JOB_FRAME     2
#define AUTO_MODE_FRAME    3
#define MANUAL_MODE_FRAME  4

static int current_frame = INFO_FRAME;
static int first_auto_mode = NOT_OK;
static int auto_mode_start = NOT_OK;
static int manual_mode_start = NOT_OK;

int check_auto_mode(void)
{
	return auto_mode_start;
}

static int not_connect_device(void)
{
	GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
	                                          ,GTK_BUTTONS_CLOSE,"Нет подключения к устройству!");
	gtk_dialog_run(GTK_DIALOG(error));
	gtk_widget_destroy(error);
	return SUCCESS;
}

static int select_frame(int frame)
{
	int rc;
	if(current_frame == AUTO_MODE_FRAME){
		if(auto_mode_start == OK){
			GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
			                                          ,GTK_BUTTONS_CLOSE,"Остановите Установку!");
			gtk_dialog_run(GTK_DIALOG(error));
			gtk_widget_destroy(error);
			return FAILURE;
		}
	}

	switch(frame){
		case INFO_FRAME:{
			gtk_widget_hide(fra_job_load);
			gtk_widget_hide(fra_job_save);
			gtk_widget_hide(fra_mode_auto);
			gtk_widget_hide(fra_mode_manual);
			gtk_widget_show(fra_info);
			current_frame = INFO_FRAME;
			break;
		}
		case LOAD_JOB_FRAME:{
			gtk_widget_hide(fra_info);
			gtk_widget_hide(fra_mode_auto);
			gtk_widget_hide(fra_mode_manual);
			gtk_widget_hide(fra_job_save);
			gtk_widget_show(fra_job_load);
			current_frame = LOAD_JOB_FRAME;
			break;
		}
		case SAVE_JOB_FRAME:{
			rc = check_connect_device(NULL);
			if(rc != SUCCESS){
				not_connect_device();
				return FAILURE;
			}
			gtk_widget_hide(fra_info);
			gtk_widget_hide(fra_mode_auto);
			gtk_widget_hide(fra_mode_manual);
			gtk_widget_hide(fra_job_load);
			gtk_widget_show(fra_job_save);
			current_frame = SAVE_JOB_FRAME;
			break;
		}
		case AUTO_MODE_FRAME:{
			rc = check_connect_device(NULL);
			if(rc != SUCCESS){
				not_connect_device();
				return FAILURE;
			}
			gtk_widget_hide(fra_info);
			gtk_widget_hide(fra_mode_manual);
			gtk_widget_hide(fra_job_save);
			if(current_job == NULL){
				gtk_widget_hide(fra_mode_auto);
				gtk_widget_show(fra_job_load);
				first_auto_mode = OK;
			 	current_frame = LOAD_JOB_FRAME;
			}
			else{
				gtk_widget_hide(fra_job_load);
				gtk_widget_show(fra_mode_auto);
				current_frame = AUTO_MODE_FRAME;
			}
			break;
		}
		case  MANUAL_MODE_FRAME:{
			rc = check_connect_device(NULL);
			if(rc != SUCCESS){
				not_connect_device();
				return FAILURE;
			}
			gtk_widget_hide(fra_info);
			gtk_widget_hide(fra_mode_auto);
			gtk_widget_hide(fra_job_load);
			gtk_widget_hide(fra_job_save);
			gtk_widget_show(fra_mode_manual);
			current_frame = MANUAL_MODE_FRAME;
			break;
		}
	}
	return SUCCESS;
}

static int set_active_color(GtkButton * w)
{
	GtkWidget * c = gtk_bin_get_child(GTK_BIN(w));
	gtk_widget_override_background_color(c,GTK_STATE_FLAG_NORMAL,&color_lite_red);
	gtk_widget_override_color(c,GTK_STATE_FLAG_NORMAL,&color_white);
	gtk_widget_override_background_color(GTK_WIDGET(w),GTK_STATE_FLAG_NORMAL,&color_green);
	return SUCCESS;
}

static int set_notactive_color(GtkButton * w)
{
	GtkWidget * c = gtk_bin_get_child (GTK_BIN(w));
	gtk_widget_override_background_color(c,GTK_STATE_FLAG_NORMAL,&color_lite_green);
	gtk_widget_override_color(c,GTK_STATE_FLAG_NORMAL,&color_black);
	gtk_widget_override_background_color(GTK_WIDGET(w),GTK_STATE_FLAG_NORMAL,&color_red);
	return SUCCESS;
}

static char STR_SPEED_VERTICAL[] = "Скорость\nвертикальная,\nград/сек";

static gboolean change_value_scale_speed(GtkRange * r,GtkScrollType s,gdouble v,gpointer ud)
{
	double speed_d;
	uint16_t speed_ui;

	speed_d = v / STEP_SPEED_VERTICAL;
	speed_d = (int)speed_d;
	speed_d *= STEP_SPEED_VERTICAL;

	gtk_range_set_value(r,speed_d);

	speed_ui = (uint16_t)(speed_d * RATE_SPEED_VERTICAL);
	command_speed_vertical(speed_ui);
	return TRUE;
}

static GtkWidget * create_scale_vertical_speed(uint16_t speed)
{
	GtkWidget * fra_speed;
	GtkWidget * box_speed;
	GtkWidget * lab_speed;
	GtkWidget * sca_speed;
	gdouble speed_d = (gdouble)speed / RATE_SPEED_VERTICAL;

	fra_speed = gtk_frame_new(NULL);
	layout_widget(fra_speed,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);

	box_speed = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	layout_widget(box_speed,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(box_speed),0);

	lab_speed = gtk_label_new(STR_SPEED_VERTICAL);
	layout_widget(lab_speed,GTK_ALIGN_START,GTK_ALIGN_START,FALSE,FALSE);
	set_size_font(lab_speed,SIZE_FONT_MINI);

	sca_speed = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL
	                                    ,MIN_SPEED_VERTICAL,MAX_SPEED_VERTICAL,STEP_SPEED_VERTICAL);
	gtk_scale_set_digits(GTK_SCALE(sca_speed),2); /*колличество знаков после запятой*/
	gtk_scale_set_value_pos(GTK_SCALE(sca_speed),GTK_POS_RIGHT);
	layout_widget(sca_speed,GTK_ALIGN_FILL,GTK_ALIGN_FILL,FALSE,TRUE);
	gtk_range_set_inverted(GTK_RANGE(sca_speed),TRUE);
	gtk_range_set_value(GTK_RANGE(sca_speed),speed_d);
	g_signal_connect(sca_speed,"change-value",G_CALLBACK(change_value_scale_speed),NULL);

	gtk_container_add(GTK_CONTAINER(fra_speed),box_speed);
	gtk_box_pack_start(GTK_BOX(box_speed),lab_speed,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(box_speed),sca_speed,TRUE,TRUE,0);

	gtk_widget_show(sca_speed);
	gtk_widget_show(lab_speed);
	gtk_widget_show(box_speed);
	gtk_widget_show(fra_speed);

	return fra_speed;
}

static int get_hour_in_second(unsigned int second)
{
	int hour;

	if(second > MAX_TIME_SECOND){
		second = MAX_TIME_SECOND;
	}
	hour = second / (60*60);

	return hour;
}

static int get_minute_in_second(unsigned int second)
{
	int hour;
	int minute;

	if(second > MAX_TIME_SECOND){
		second = MAX_TIME_SECOND;
	}
	hour = get_hour_in_second(second);
	minute = second - (hour * (60*60));
	minute /= 60;
	return minute;
}

static int get_second_in_second(unsigned int second_all)
{
	int hour;
	int minute;
	int second;

	if(second_all > MAX_TIME_SECOND){
		second_all = MAX_TIME_SECOND;
	}
	hour = get_hour_in_second(second_all);
	minute = get_minute_in_second(second_all);
	second = second_all - (hour*(60*60)) - (minute * 60);
	return second;
}

/*****************************************************************************/
/* Основное меню программы                                                   */
/*****************************************************************************/

/****************************************************************************/
/*  подменю управление                                                      */
/****************************************************************************/

static void activate_menu_auto_mode(GtkMenuItem * im,gpointer d)
{
	select_frame(AUTO_MODE_FRAME);
	g_message("%s.",STR_MODE_AUTO);
}

static void activate_menu_manual_mode(GtkMenuItem * im,gpointer d)
{
	select_frame(MANUAL_MODE_FRAME);
	g_message("%s.",STR_MODE_MANUAL);
}

static char STR_MODE[] =        "Режим";
static char STR_AUTO_MODE[] =   "автоматический";
static char STR_MANUAL_MODE[] = "ручной";

static GtkWidget * create_menu_mode(void)
{
	GtkWidget * menite_mode;
	GtkWidget * men_mode;
	GtkWidget * menite_temp;

	menite_mode = gtk_menu_item_new_with_label(STR_MODE);

	men_mode = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menite_mode),men_mode);

	menite_temp = gtk_menu_item_new_with_label(STR_AUTO_MODE);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_auto_mode),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,GDK_KEY_A,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_mode),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_MANUAL_MODE);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_manual_mode),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,GDK_KEY_M,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_mode),menite_temp);
	gtk_widget_show(menite_temp);

	gtk_widget_show(menite_mode);
	return menite_mode;
}

/*****************************************************************************/
/* главное меню : подменю работа                                             */
/*****************************************************************************/

static void activate_menu_load_job(GtkMenuItem * b,gpointer d)
{
	select_frame(LOAD_JOB_FRAME);
	g_message("%s.",STR_JOB_LOAD);
}

static void activate_menu_save_job(GtkMenuItem * b,gpointer d)
{
	select_frame(SAVE_JOB_FRAME);
	g_message("%s.",STR_JOB_SAVE);
}

static char STR_JOB[] = "Работа";
static char STR_LOAD_JOB[] = "Загрузить";
static char STR_CREATE_JOB[] = "Создать";
static char STR_EXIT[] = "ВЫХОД";

static void unrealize_menubar_main(GtkWidget * w,gpointer ud)
{
}

static void activate_menu_exit(GtkMenuItem * im,gpointer d)
{
	select_frame(INFO_FRAME);
	gtk_widget_destroy(win_main);
}

GtkWidget * create_menu_main(void)
{
	GtkWidget * menbar_main;
	GtkWidget * men_work;
	GtkWidget * menite_temp;

	menbar_main = gtk_menu_bar_new();
	gtk_widget_set_halign(menbar_main,GTK_ALIGN_START);
	g_signal_connect(menbar_main,"unrealize",G_CALLBACK(unrealize_menubar_main),NULL);

	menite_temp = gtk_menu_item_new_with_label(STR_JOB);
	gtk_menu_shell_append(GTK_MENU_SHELL(menbar_main),menite_temp);
	gtk_widget_show(menite_temp);

	men_work = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menite_temp),men_work);

	menite_temp = gtk_menu_item_new_with_label(STR_LOAD_JOB);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_load_job),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,GDK_KEY_L,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_work),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_CREATE_JOB);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_save_job),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,GDK_KEY_C,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_work),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(men_work),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_EXIT);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_exit),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,GDK_KEY_Q,GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_work),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = create_menu_mode();
	gtk_menu_shell_append(GTK_MENU_SHELL(menbar_main),menite_temp);

	menite_temp = create_menu_device();
	gtk_menu_shell_append(GTK_MENU_SHELL(menbar_main),menite_temp);

	menite_temp = create_menu_video();
	gtk_menu_shell_append(GTK_MENU_SHELL(menbar_main),menite_temp);

	gtk_widget_show(men_work);
	gtk_widget_show(menbar_main);

 	return menbar_main;
}

/*****************************************************************************/
/*****************************************************************************/
/* Панели управления и информации                                            */
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/* Панель информации о работе                                                */
/*****************************************************************************/

static char STR_INFO[] = "РАБОТА";
static char STR_NOT_DEVICE[] = "Загрузите информацию о работе";
static char STR_PRESSURE[] =         "Рабочие давление,  атм";
static char STR_PRESSURE_DEFAULT[] = "0,0";
static char STR_TIME_JOB[] =         "Время работы";
static char STR_TIME_JOB_DEFAULT[] = "00:00:00";
static char STR_UPRISE_ANGEL[] =     "Угол подъема,  градусов";
static char STR_LOWERING_ANGEL[] =   "Угол опускания,  градусов";
static char STR_ANGLE_DEFAULT[] =    "0,0";

static GtkWidget * lab_info_name_job = NULL;
static GtkWidget * lab_info_pressure = NULL;
static GtkWidget * lab_info_time = NULL;
static GtkWidget * lab_info_uprise = NULL;
static GtkWidget * lab_info_lowering = NULL;

static int set_current_value_info(void)
{
	if(current_job == NULL){
		gtk_label_set_text(GTK_LABEL(lab_info_name_job),STR_NOT_DEVICE);
		gtk_label_set_text(GTK_LABEL(lab_info_pressure),STR_PRESSURE_DEFAULT);
		gtk_label_set_text(GTK_LABEL(lab_info_time),STR_TIME_JOB_DEFAULT);
		gtk_label_set_text(GTK_LABEL(lab_info_uprise),STR_ANGLE_DEFAULT);
		gtk_label_set_text(GTK_LABEL(lab_info_lowering),STR_ANGLE_DEFAULT);
	}
	else{
		gtk_label_set_text(GTK_LABEL(lab_info_name_job),current_job->name->str);
		g_string_printf(temp_string,FORMAT_PRESSURE,PRINTF_PRESSURE(current_job->pressure));
		gtk_label_set_text(GTK_LABEL(lab_info_pressure),temp_string->str);
		g_string_printf(temp_string,"%02d:%02d:%02d"
		               ,get_hour_in_second(current_job->time)
		               ,get_minute_in_second(current_job->time)
		               ,get_second_in_second(current_job->time));
		gtk_label_set_text(GTK_LABEL(lab_info_time),temp_string->str);
		g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(current_job->uprise));
		gtk_label_set_text(GTK_LABEL(lab_info_uprise),temp_string->str);
		g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(current_job->lowering));
		gtk_label_set_text(GTK_LABEL(lab_info_lowering),temp_string->str);
	}
	return SUCCESS;
}

static GtkWidget * create_info(void)
{
	GtkGrid * gri_info;
	GtkWidget * lab_fra_info;
	GtkWidget * lab_pressure;
	GtkWidget * lab_time;
	GtkWidget * lab_uprise;
	GtkWidget * lab_lowering;

	fra_info = gtk_frame_new(NULL);
	gtk_frame_set_label_align(GTK_FRAME(fra_info),0.5,0.5);
	layout_widget(fra_info,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);

	lab_fra_info = gtk_label_new(STR_INFO);
	set_size_font(lab_fra_info,SIZE_FONT_MEDIUM);

	gri_info = GTK_GRID(gtk_grid_new());
	gtk_grid_set_row_spacing(gri_info,20);

	lab_info_name_job = gtk_label_new(STR_NOT_DEVICE);
	gtk_widget_set_hexpand(lab_info_name_job,TRUE);
	set_size_font(lab_info_name_job,SIZE_FONT_EXTRA_LARGE);

	gtk_widget_override_color(lab_info_name_job,GTK_STATE_FLAG_NORMAL,&color_lite_blue);

	lab_pressure = gtk_label_new(STR_PRESSURE);
	set_size_font(lab_pressure,SIZE_FONT_MEDIUM);

	lab_info_pressure = gtk_label_new(STR_PRESSURE_DEFAULT);
	set_size_font(lab_info_pressure,SIZE_FONT_MEDIUM);

	lab_time = gtk_label_new(STR_TIME_JOB);
	set_size_font(lab_time,SIZE_FONT_MEDIUM);

	lab_info_time = gtk_label_new(STR_TIME_JOB_DEFAULT);
	set_size_font(lab_info_time,SIZE_FONT_MEDIUM);

	lab_uprise = gtk_label_new(STR_UPRISE_ANGEL);
	set_size_font(lab_uprise,SIZE_FONT_MEDIUM);

	lab_info_uprise = gtk_label_new(STR_ANGLE_DEFAULT);
	set_size_font(lab_info_uprise,SIZE_FONT_MEDIUM);

	lab_lowering = gtk_label_new(STR_LOWERING_ANGEL);
	set_size_font(lab_lowering,SIZE_FONT_MEDIUM);

	lab_info_lowering = gtk_label_new(STR_ANGLE_DEFAULT);
	set_size_font(lab_info_lowering,SIZE_FONT_MEDIUM);

	gtk_frame_set_label_widget(GTK_FRAME(fra_info),lab_fra_info);
	gtk_container_add(GTK_CONTAINER(fra_info),GTK_WIDGET(gri_info));
	gtk_grid_attach(gri_info,lab_info_name_job,0,0,3,2);
	gtk_grid_attach(gri_info,lab_pressure,0,2,1,1);
	gtk_grid_attach(gri_info,lab_info_pressure,2,2,1,1);
	gtk_grid_attach(gri_info,lab_time,0,3,1,1);
	gtk_grid_attach(gri_info,lab_info_time,2,3,1,1);
	gtk_grid_attach(gri_info,lab_uprise,0,4,1,1);
	gtk_grid_attach(gri_info,lab_info_uprise,2,4,1,1);
	gtk_grid_attach(gri_info,lab_lowering,0,5,1,1);
	gtk_grid_attach(gri_info,lab_info_lowering,2,5,1,1);

	gtk_widget_show(lab_info_lowering);
	gtk_widget_show(lab_lowering);
	gtk_widget_show(lab_info_uprise);
	gtk_widget_show(lab_uprise);
	gtk_widget_show(lab_info_time);
	gtk_widget_show(lab_time);
	gtk_widget_show(lab_info_pressure);
	gtk_widget_show(lab_pressure);
	gtk_widget_show(lab_info_name_job);
	gtk_widget_show(GTK_WIDGET(gri_info));
	gtk_widget_show(lab_fra_info);
	gtk_widget_hide(fra_info);
	return fra_info;
}


/*****************************************************************************/
/* Панель работа в автоматическом режиме                                     */
/*****************************************************************************/

struct _label_auto_mode_s
{
	GtkWidget * pressure;
	GtkWidget * time;
	GtkWidget * uprise;
	GtkWidget * lowering;
};
typedef struct _label_auto_mode_s  label_auto_mode_s;

static label_auto_mode_s label_auto_mode;

static char STR_BUTTON_AUTO_START[] = "СТАРТ";
static char STR_BUTTON_AUTO_STOP[]  = "СТОП";
static char STR_BUTTON_AUTO_PAUSE[] = "ПАУЗА";
static char STR_SET_VALUE[] = "Установленое значение";
static char STR_CURRENT_VALUE[] = "Текущее значение";

static GtkWidget * but_auto_mode_stop;
static GtkWidget * but_auto_mode_pause;
static void clicked_button_auto_stop(GtkButton * b,gpointer d);
static void clicked_button_auto_pause(GtkButton * b,gpointer d);

static int auto_mode_pause = NOT_OK;
static unsigned long int amount_auto_mode = 0;
static unsigned long int timeout_auto_mode = DEFAULT_TIMEOUT_CHECK; /*таймоут в миллесукендах*/
static int console_pause_old = 0;
static int console_stop_old = 0;

static int check_registers_auto_mode(gpointer ud)
{
	long int rc;
	uint16_t angle;
	uint16_t pressure;
	/*uint16_t sensors;*/
	/*uint16_t input;*/
	int hour;
	int minut;
	int second;

	if(auto_mode_start != OK){
		return FALSE;
	}
	if(auto_mode_pause == OK){
		return FALSE;
	}
	rc = command_info();
	if(rc != SUCCESS){
		return FALSE;
	}

	angle = info_angle();
	pressure = info_pressure();
	/*sensors = info_sensors();*/
	/*input = info_input();*/

	if(console_pause_old !=  info_console_pause()){
		if(info_console_pause()){
			clicked_button_auto_pause(GTK_BUTTON(but_auto_mode_pause),NULL);
		}
		console_pause_old = info_console_pause();
	}

	if(console_stop_old != info_console_stop()){
		clicked_button_auto_stop(GTK_BUTTON(but_auto_mode_stop),NULL);
		console_stop_old = info_console_stop();
	}

	amount_auto_mode += timeout_auto_mode;
	rc = amount_auto_mode / MILLISECOND;

	if(rc >= current_job->time){
		auto_mode_start = NOT_OK;
		command_auto_stop();
		set_notactive_color(GTK_BUTTON(ud));
		return FALSE;
	}

	hour = (rc / (60*60));
	minut = (rc - (hour*60*60))/60;
	second = (rc - (hour*60*60) - (minut*60));

	g_string_printf(temp_string,FORMAT_PRESSURE,PRINTF_PRESSURE(pressure));
	gtk_label_set_text(GTK_LABEL(label_auto_mode.pressure),temp_string->str);

	g_string_printf(temp_string,"%02d:%02d:%02d",hour,minut,second);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.time),temp_string->str);

	g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(angle));
	gtk_label_set_text(GTK_LABEL(label_auto_mode.uprise),temp_string->str);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.lowering),temp_string->str);

	return TRUE;
}

static void clicked_button_auto_start(GtkButton * b,gpointer d)
{
	int rc;
	if(auto_mode_start == OK){
		return;
	}
	rc = command_auto_start();
	if(rc == SUCCESS){
		auto_mode_start = OK;
		amount_auto_mode = 0;
		g_timeout_add(timeout_auto_mode,check_registers_auto_mode,b);
		set_active_color(b);
	}
}

static void clicked_button_auto_stop(GtkButton * b,gpointer d)
{
	if(auto_mode_start != OK){
		return;
	}
	auto_mode_start = NOT_OK;
	auto_mode_pause = NOT_OK;
	command_auto_stop();
	set_notactive_color(GTK_BUTTON(d));
	set_notactive_color(GTK_BUTTON(but_auto_mode_pause));
}

static void clicked_button_auto_pause(GtkButton * b,gpointer d)
{
	if(auto_mode_pause == OK){
		command_auto_start();
		g_timeout_add(timeout_auto_mode,check_registers_auto_mode,&label_auto_mode);
		set_notactive_color(b);
		auto_mode_pause = NOT_OK;
		return;
	}
	if(auto_mode_start == OK){
		command_auto_pause();
		auto_mode_pause = OK;
		set_active_color(b);
	}
}

static GtkWidget * lab_auto_mode_name_job;
static GtkWidget * lab_auto_mode_pressure;
static GtkWidget * lab_auto_mode_time;
static GtkWidget * lab_auto_mode_uprise;
static GtkWidget * lab_auto_mode_lowering;

static int set_current_value_auto_mode(void)
{
	gtk_label_set_text(GTK_LABEL(label_auto_mode.pressure),STR_PRESSURE_DEFAULT);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.time),STR_TIME_JOB_DEFAULT);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.uprise),STR_ANGLE_DEFAULT);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.lowering),STR_ANGLE_DEFAULT);
	return SUCCESS;
}

static int set_preset_value_auto_mode(void)
{
	if(current_job == NULL){
		g_critical("Не выбрана работа!");
		return FAILURE;
	}
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_name_job),current_job->name->str);
	g_string_printf(temp_string,FORMAT_PRESSURE,PRINTF_PRESSURE(current_job->pressure));
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_pressure),temp_string->str);
	g_string_printf(temp_string,"%02d:%02d:%02d"
	               ,get_hour_in_second(current_job->time)
	               ,get_minute_in_second(current_job->time)
	               ,get_second_in_second(current_job->time));
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_time),temp_string->str);
	g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(current_job->uprise));
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_uprise),temp_string->str);
	g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(current_job->lowering));
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_lowering),temp_string->str);
	return SUCCESS;
}

static GtkWidget * create_label_mode_auto(void)
{
	GtkWidget * fra_auto;
	GtkWidget * gri_auto;
	GtkWidget * lab_set;
	GtkWidget * lab_current;
	GtkWidget * lab_pressure;
	GtkWidget * lab_time;
	GtkWidget * lab_uprise;
	GtkWidget * lab_lowering;

	fra_auto = gtk_frame_new(NULL);
	layout_widget(fra_auto,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(fra_auto),5);

	gri_auto = gtk_grid_new();
	gtk_widget_set_hexpand(gri_auto,TRUE);
	gtk_widget_set_vexpand(gri_auto,TRUE);
	gtk_grid_set_row_homogeneous(GTK_GRID(gri_auto),FALSE);
	gtk_grid_set_row_spacing(GTK_GRID(gri_auto),10);
	gtk_grid_set_column_homogeneous(GTK_GRID(gri_auto),FALSE);
	gtk_grid_set_column_spacing(GTK_GRID(gri_auto),10);

	lab_auto_mode_name_job = gtk_label_new(STR_NOT_DEVICE);
	gtk_widget_set_hexpand(lab_auto_mode_name_job,TRUE);
	gtk_widget_set_vexpand(lab_auto_mode_name_job,TRUE);
	set_size_font(lab_auto_mode_name_job,SIZE_FONT_MEDIUM);

	lab_set = gtk_label_new(STR_SET_VALUE);
	set_size_font(lab_set,SIZE_FONT_SMALL);
	gtk_widget_set_halign(lab_set,GTK_ALIGN_CENTER);
	lab_current = gtk_label_new(STR_CURRENT_VALUE);
	set_size_font(lab_current,SIZE_FONT_SMALL);
	gtk_widget_set_halign(lab_current,GTK_ALIGN_CENTER);
	lab_pressure = gtk_label_new(STR_PRESSURE);
	set_size_font(lab_pressure,SIZE_FONT_SMALL);
	gtk_widget_set_halign(lab_pressure,GTK_ALIGN_START);
	lab_time = gtk_label_new(STR_TIME_JOB);
	set_size_font(lab_time,SIZE_FONT_SMALL);
	gtk_widget_set_halign(lab_time,GTK_ALIGN_START);
	lab_uprise = gtk_label_new(STR_UPRISE_ANGEL);
	set_size_font(lab_uprise,SIZE_FONT_SMALL);
	gtk_widget_set_halign(lab_uprise,GTK_ALIGN_START);
	lab_lowering = gtk_label_new(STR_LOWERING_ANGEL);
	set_size_font(lab_lowering,SIZE_FONT_SMALL);
	gtk_widget_set_halign(lab_lowering,GTK_ALIGN_START);

	lab_auto_mode_pressure = gtk_label_new(STR_PRESSURE_DEFAULT);
	lab_auto_mode_time = gtk_label_new(STR_TIME_JOB_DEFAULT);
	lab_auto_mode_uprise = gtk_label_new(STR_ANGLE_DEFAULT);
	lab_auto_mode_lowering = gtk_label_new(STR_ANGLE_DEFAULT);
	label_auto_mode.pressure = gtk_label_new(STR_PRESSURE_DEFAULT);
	label_auto_mode.time = gtk_label_new(STR_TIME_JOB_DEFAULT);
	label_auto_mode.uprise = gtk_label_new(STR_ANGLE_DEFAULT);
	label_auto_mode.lowering = gtk_label_new(STR_ANGLE_DEFAULT);

	gtk_container_add(GTK_CONTAINER(fra_auto),gri_auto);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_auto_mode_name_job  ,0,0,3,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_set                 ,1,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_current             ,2,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_pressure            ,0,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_time                ,0,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_uprise              ,0,4,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_lowering            ,0,5,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_auto_mode_pressure  ,1,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_auto_mode_time      ,1,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_auto_mode_uprise    ,1,4,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),lab_auto_mode_lowering  ,1,5,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),label_auto_mode.pressure,2,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),label_auto_mode.time    ,2,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),label_auto_mode.uprise  ,2,4,1,1);
	gtk_grid_attach(GTK_GRID(gri_auto),label_auto_mode.lowering,2,5,1,1);

	gtk_widget_show(label_auto_mode.lowering);
	gtk_widget_show(label_auto_mode.uprise);
	gtk_widget_show(label_auto_mode.time);
	gtk_widget_show(label_auto_mode.pressure);
	gtk_widget_show(lab_auto_mode_lowering);
	gtk_widget_show(lab_auto_mode_uprise);
	gtk_widget_show(lab_auto_mode_time);
	gtk_widget_show(lab_auto_mode_pressure);
	gtk_widget_show(lab_lowering);
	gtk_widget_show(lab_uprise);
	gtk_widget_show(lab_time);
	gtk_widget_show(lab_pressure);
	gtk_widget_show(lab_current);
	gtk_widget_show(lab_set);
	gtk_widget_show(lab_auto_mode_name_job);
	gtk_widget_show(gri_auto);
	gtk_widget_show(fra_auto);

	return fra_auto;
}

static uint16_t horizontal_offset = DEFAULT_HORIZONTAL_OFFSET;

static void show_frame_auto_mode(GtkWidget * w,gpointer ud)
{
	int rc;
	if(current_job == NULL){
		g_critical("Не выбрана работа!");
		return;
	}
	rc = command_auto_mode();
	if(rc == SUCCESS){
		command_pressure(current_job->pressure);
		command_uprise_angle(current_job->uprise);
		command_lowering_angle(current_job->lowering);
		command_speed_vertical(DEFAULT_SPEED_VERTICAL_AUTO_MODE);
		command_horizontal(horizontal_offset);
		auto_mode_start = NOT_OK;
		auto_mode_pause = NOT_OK;
		console_pause_old = 0;
		console_stop_old = 0;
		set_preset_value_auto_mode();
		set_current_value_auto_mode();
	}
}

static void hide_frame_auto_mode(GtkWidget * w,gpointer ud)
{
	command_null_mode();
}

static GtkWidget * create_mode_auto(void)
{
	GtkWidget * lab_fra_mode_auto;
	GtkWidget * box_hor_main;
	GtkWidget * box_hor_but;
	GtkWidget * but_start;
	GtkWidget * fra_scale;
	GtkWidget * gri_auto;

	fra_mode_auto = gtk_frame_new(NULL);
	layout_widget(fra_mode_auto,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_frame_set_label_align(GTK_FRAME(fra_mode_auto),0.5,0.5);
	g_signal_connect(fra_mode_auto,"show",G_CALLBACK(show_frame_auto_mode),NULL);
	g_signal_connect(fra_mode_auto,"hide",G_CALLBACK(hide_frame_auto_mode),NULL);

	lab_fra_mode_auto = gtk_label_new(STR_MODE_AUTO);
	set_size_font(lab_fra_mode_auto,SIZE_FONT_MEDIUM);

	box_hor_main = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
	gtk_widget_set_hexpand(box_hor_main,TRUE);
	gtk_widget_set_vexpand(box_hor_main,TRUE);

	box_hor_but = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
	gtk_widget_set_vexpand(box_hor_but,TRUE);

	but_start = gtk_button_new_with_label(STR_BUTTON_AUTO_START);
	layout_widget(but_start,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_start)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_start));
	g_signal_connect(but_start,"clicked",G_CALLBACK(clicked_button_auto_start),NULL);

	but_auto_mode_stop = gtk_button_new_with_label(STR_BUTTON_AUTO_STOP);
	layout_widget(but_auto_mode_stop,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_auto_mode_stop)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_auto_mode_stop));
	g_signal_connect(but_auto_mode_stop,"clicked",G_CALLBACK(clicked_button_auto_stop),but_start);

	but_auto_mode_pause = gtk_button_new_with_label(STR_BUTTON_AUTO_PAUSE);
	layout_widget(but_auto_mode_pause,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_auto_mode_pause)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_auto_mode_pause));
	g_signal_connect(but_auto_mode_pause,"clicked",G_CALLBACK(clicked_button_auto_pause),but_start);

	fra_scale = create_scale_vertical_speed(DEFAULT_SPEED_VERTICAL_AUTO_MODE);

	gri_auto = create_label_mode_auto();

	gtk_frame_set_label_widget(GTK_FRAME(fra_mode_auto),lab_fra_mode_auto);
	gtk_container_add(GTK_CONTAINER(fra_mode_auto),box_hor_main);
	gtk_box_pack_start(GTK_BOX(box_hor_main),box_hor_but,TRUE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_hor_but),but_start,TRUE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_hor_but),but_auto_mode_stop,TRUE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_hor_but),but_auto_mode_pause,TRUE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_hor_but),fra_scale,TRUE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_hor_main),gri_auto,TRUE,TRUE,5);

	gtk_widget_show(fra_scale);
	gtk_widget_show(but_auto_mode_pause);
	gtk_widget_show(but_auto_mode_stop);
	gtk_widget_show(but_start);
	gtk_widget_show(box_hor_but);
	gtk_widget_show(box_hor_main);
	gtk_widget_show(lab_fra_mode_auto);
	gtk_widget_hide(fra_mode_auto);

	return fra_mode_auto;
}


/*****************************************************************************/
/* Панель работа в ручном режиме                                             */
/*****************************************************************************/

static char STR_BUTTON_MANUAL_UP[] =    "ВВЕРХ";
static char STR_BUTTON_MANUAL_DOWN[] =  "ВНИЗ";
static char STR_BUTTON_MANUAL_LEFT[] =  "ВЛЕВО";
static char STR_BUTTON_MANUAL_RIGHT[] = "ВПРАВО";
static char STR_BUTTON_MANUAL_LASER[] = "Лазер";
static char STR_BUTTON_MANUAL_OPEN[] =  "Включить\n Насос";
static char STR_BUTTON_MANUAL_CLOSE[] = "Выключить\n Насос";
static char STR_ANGLE[] = "Угол, градусов";
static char STR_VALVE[] = "Задвижка";

static void press_button_manual_up(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_active_color(b);
	command_manual_up_on();
}
static void release_button_manual_up(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_notactive_color(b);
	command_manual_up_off();
}
static void press_button_manual_down(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_active_color(b);
	command_manual_down_on();
}
static void release_button_manual_down(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_notactive_color(b);
	command_manual_down_off();
}
static void press_button_manual_left(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_active_color(b);
	command_manual_left_on();
}

static void release_button_manual_left(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_notactive_color(b);
	command_manual_left_off();
}
static void press_button_manual_right(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_active_color(b);
	command_manual_right_on();
}
static void release_button_manual_right(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_notactive_color(b);
	command_manual_right_off();
}

static int manual_pump_open = NOT_OK;

static void clicked_button_manual_pump(GtkButton * b,gpointer d)
{
	int console_pump;

	if(d != NULL){
		console_pump = *((int*)d);

		if(console_pump){
			if(manual_pump_open == OK){
				return ; /*нажатие на консоле и в программе совпадает*/
			}
		}
		else{
			if(manual_pump_open == NOT_OK){
				return ; /*нажатие на консоле и в программе совпадает*/
			}
		}
	}

	if(manual_pump_open == OK){
		manual_pump_open = NOT_OK;
		command_manual_drive_off();
		gtk_button_set_label(b,STR_BUTTON_MANUAL_OPEN);
		/*set_size_font(gtk_bin_get_child(GTK_BIN(b)),SIZE_FONT_SMALL);*/
		set_notactive_color(b);
	}
	else{
		manual_pump_open = OK;
		command_manual_drive_on();
		gtk_button_set_label(b,STR_BUTTON_MANUAL_CLOSE);
		/*set_size_font(gtk_bin_get_child(GTK_BIN(b)),SIZE_FONT_SMALL);*/
		set_active_color(b);
	}
}

static int manual_laser = NOT_OK;
static void clicked_button_manual_laser(GtkButton * b,gpointer d)
{
	if(manual_laser == OK){
		manual_laser = NOT_OK;
		command_manual_laser_off();
		/*set_size_font(gtk_bin_get_child(GTK_BIN(b)),SIZE_FONT_SMALL);*/
		set_notactive_color(b);
	}
	else{
		manual_laser = OK;
		command_manual_laser_on();
		/*set_size_font(gtk_bin_get_child(GTK_BIN(b)),SIZE_FONT_SMALL);*/
		set_active_color(b);
	}
}

static uint16_t valve_ui = MIN_VALVE_IN_TIC;

static gboolean change_value_scale_valve(GtkRange * r,GtkScrollType s,gdouble v,gpointer ud)
{
	double valve_d = v;

	valve_d = v / STEP_VALVE_IN_TIC;
	valve_d = (int)valve_d;
	valve_d *= STEP_VALVE_IN_TIC;

	if(valve_d < 0 ){
		valve_d = 0;
	}
	if(valve_d > MAX_VALVE_IN_TIC){
		valve_d = MAX_VALVE_IN_TIC;
	}

	gtk_range_set_value(r,valve_d);
	valve_ui = (uint16_t)valve_d;
	command_valve(valve_ui);
	return TRUE;
}

struct _label_manual_mode_s
{
	GtkWidget * pressure;
	GtkWidget * time;
	GtkWidget * angle;
};

typedef struct _label_manual_mode_s label_manual_mode_s;

static label_manual_mode_s label_manual_mode;

static GtkWidget * but_manual_mode_up;
static GtkWidget * but_manual_mode_down;
static GtkWidget * but_manual_mode_left;
static GtkWidget * but_manual_mode_right;
static GtkWidget * but_manual_mode_pump;

static unsigned long int amount_manual_mode = 0;
static unsigned long int timeout_manual_mode = DEFAULT_TIMEOUT_CHECK;

static int console_up_old = 0;
static int console_down_old = 0;
static int console_left_old = 0;
static int console_right_old = 0;
static int console_on_valve_old = 0;
static int console_off_valve_old = 0;

static int check_registers_manual_mode(gpointer ud)
{
	long int rc;
	uint16_t angle;
	uint16_t pressure;
	/*uint16_t sensors;*/
	/*uint16_t input;*/
	int hour;
	int minut;
	int second;

	if(manual_mode_start != OK){
		return FALSE;
	}

	rc = command_info();
	if(rc != SUCCESS){
		return FALSE;
	}

	angle = info_angle();
	pressure = info_pressure();
	/*sensors = info_sensors();*/
	/*input = info_input();*/

	if( console_up_old != info_console_up()){
		console_up_old = info_console_up();
		if(console_up_old){
			press_button_manual_up(GTK_BUTTON(but_manual_mode_up),NULL,NULL);
		}
		else{
			release_button_manual_up(GTK_BUTTON(but_manual_mode_up),NULL,NULL);
		}
	}
	if(console_down_old != info_console_down()){
		console_down_old = info_console_down();
		if(console_down_old){
			press_button_manual_down(GTK_BUTTON(but_manual_mode_down),NULL,NULL);
		}
		else{
			release_button_manual_down(GTK_BUTTON(but_manual_mode_down),NULL,NULL);
		}
	}
	if(console_left_old != info_console_left()){
		console_left_old = info_console_left();
		if(console_left_old){
			press_button_manual_left(GTK_BUTTON(but_manual_mode_left),NULL,NULL);
		}
		else{
			release_button_manual_left(GTK_BUTTON(but_manual_mode_left),NULL,NULL);
		}
	}
	if(console_right_old != info_console_right()){
		console_right_old = info_console_right();
		if(console_right_old){
			press_button_manual_right(GTK_BUTTON(but_manual_mode_right),NULL,NULL);
		}
		else{
			release_button_manual_right(GTK_BUTTON(but_manual_mode_right),NULL,NULL);
		}
	}

	if(console_on_valve_old != info_console_on_valve()){
		console_on_valve_old = info_console_on_valve();
		if(console_on_valve_old){
			int bit = 1;
			clicked_button_manual_pump(GTK_BUTTON(but_manual_mode_pump),&bit);
		}
	}
	if(console_off_valve_old != info_console_off_valve()){
		console_off_valve_old = info_console_off_valve();
		if(console_off_valve_old){
			int bit = 0;
			clicked_button_manual_pump(GTK_BUTTON(but_manual_mode_pump),&bit);
		}
	}

	amount_manual_mode += timeout_manual_mode;
	rc = amount_manual_mode / MILLISECOND;
	if(rc >= MAX_TIME_SECOND){
		amount_manual_mode = 0;
	}
	hour = (rc / (60*60));
	minut = (rc - (hour*60*60))/60;
	second = (rc - (hour*60*60) - (minut*60));

	g_string_printf(temp_string,FORMAT_PRESSURE,PRINTF_PRESSURE(pressure));
	gtk_label_set_text(GTK_LABEL(label_manual_mode.pressure),temp_string->str);

	g_string_printf(temp_string,"%02d:%02d:%02d",hour,minut,second);
	gtk_label_set_text(GTK_LABEL(label_manual_mode.time),temp_string->str);

	g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(angle));
	gtk_label_set_text(GTK_LABEL(label_manual_mode.angle),temp_string->str);

	return TRUE;
}

static uint16_t valve_ui_old = MIN_VALVE_IN_TIC;
static void show_frame_manual_mode(GtkWidget * w,gpointer ud)
{
	int rc;

	rc = command_manual_mode();

	if(rc == SUCCESS){
		command_manual_null();
		command_speed_vertical(DEFAULT_SPEED_VERTICAL_MANUAL_MODE);
		valve_ui_old = valve_ui;
		command_valve(valve_ui);
		amount_manual_mode = 0;
		manual_mode_start = OK;
		console_up_old = 0;
		console_down_old = 0;
		console_left_old = 0;
		console_right_old = 0;
		console_on_valve_old = 0;
		console_off_valve_old = 0;
		g_timeout_add(timeout_manual_mode,check_registers_manual_mode,NULL);
	}
}

static char STR_VALVE_TIC[] = "valve";

static int deini_manual_mode(void)
{
	if(valve_ui != valve_ui_old){
		g_key_file_set_integer(ini_file,STR_GLOBAL_KEY,STR_VALVE_TIC,valve_ui);
		set_flag_save_config();
		valve_ui_old = valve_ui;
	}
	return SUCCESS;
}

static void hide_frame_manual_mode(GtkWidget * w,gpointer ud)
{
 	manual_mode_start = NOT_OK;
	command_manual_null();
	command_null_mode();
	command_valve(MIN_VALVE_IN_TIC);
	deini_manual_mode();
}

static GtkWidget * create_scale_valve(void)
{
	GtkWidget * fra_valve;
	GtkWidget * box_valve;
	GtkWidget * lab_valve;
	GtkWidget * sca_valve;

	fra_valve = gtk_frame_new(NULL);
	layout_widget(fra_valve,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);

	box_valve = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	layout_widget(box_valve,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(box_valve),0);

	lab_valve = gtk_label_new(STR_VALVE);
	layout_widget(lab_valve,GTK_ALIGN_START,GTK_ALIGN_START,FALSE,FALSE);
	set_size_font(lab_valve,SIZE_FONT_MINI);

	sca_valve = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL
	                                    ,MIN_VALVE_IN_TIC,MAX_VALVE_IN_TIC,STEP_VALVE_IN_TIC);
	gtk_scale_set_digits(GTK_SCALE(sca_valve),0); /*колличество знаков после запятой*/
	gtk_scale_set_value_pos(GTK_SCALE(sca_valve),GTK_POS_RIGHT);
	layout_widget(sca_valve,GTK_ALIGN_FILL,GTK_ALIGN_FILL,FALSE,TRUE);
	gtk_range_set_inverted(GTK_RANGE(sca_valve),TRUE);
	gtk_range_set_value(GTK_RANGE(sca_valve),valve_ui);
	g_signal_connect(sca_valve,"change-value",G_CALLBACK(change_value_scale_valve),NULL);

	gtk_container_add(GTK_CONTAINER(fra_valve),box_valve);
	gtk_box_pack_start(GTK_BOX(box_valve),lab_valve,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(box_valve),sca_valve,TRUE,TRUE,0);

	gtk_widget_show(sca_valve);
	gtk_widget_show(lab_valve);
	gtk_widget_show(box_valve);
	gtk_widget_show(fra_valve);

	return fra_valve;
}

static GtkWidget * create_button_mode_manual(void)
{
	GtkWidget * fra_mode;
	GtkWidget * gri_mode;
	GtkWidget * but_laser;
	GtkWidget * fra_vertical_speed;
	GtkWidget * fra_valve;

	fra_mode = gtk_frame_new(NULL);
	layout_widget(fra_mode,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(fra_mode),5);

	gri_mode = gtk_grid_new();
	layout_widget(gri_mode,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(gri_mode),10);
	gtk_grid_set_row_homogeneous(GTK_GRID(gri_mode),FALSE);
	gtk_grid_set_column_spacing(GTK_GRID(gri_mode),10);
	gtk_grid_set_column_homogeneous(GTK_GRID(gri_mode),TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(gri_mode),5);

	but_manual_mode_up = gtk_button_new_with_label(STR_BUTTON_MANUAL_UP);
	layout_widget(but_manual_mode_up,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_notactive_color(GTK_BUTTON(but_manual_mode_up));
	g_signal_connect(but_manual_mode_up,"button-press-event",G_CALLBACK(press_button_manual_up),NULL);
	g_signal_connect(but_manual_mode_up,"button-release-event",G_CALLBACK(release_button_manual_up),NULL);

	but_manual_mode_down = gtk_button_new_with_label(STR_BUTTON_MANUAL_DOWN);
	layout_widget(but_manual_mode_down,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_notactive_color(GTK_BUTTON(but_manual_mode_down));
	g_signal_connect(but_manual_mode_down,"button-press-event",G_CALLBACK(press_button_manual_down),NULL);
	g_signal_connect(but_manual_mode_down,"button-release-event",G_CALLBACK(release_button_manual_down),NULL);

	but_manual_mode_left = gtk_button_new_with_label(STR_BUTTON_MANUAL_LEFT);
	layout_widget(but_manual_mode_left,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_notactive_color(GTK_BUTTON(but_manual_mode_left));
	g_signal_connect(but_manual_mode_left,"button-press-event",G_CALLBACK(press_button_manual_left),NULL);
	g_signal_connect(but_manual_mode_left,"button-release-event",G_CALLBACK(release_button_manual_left),NULL);

	but_manual_mode_right = gtk_button_new_with_label(STR_BUTTON_MANUAL_RIGHT);
	layout_widget(but_manual_mode_right,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_notactive_color(GTK_BUTTON(but_manual_mode_right));
	g_signal_connect(but_manual_mode_right,"button-press-event",G_CALLBACK(press_button_manual_right),NULL);
	g_signal_connect(but_manual_mode_right,"button-release-event",G_CALLBACK(release_button_manual_right),NULL);

	but_laser = gtk_button_new_with_label(STR_BUTTON_MANUAL_LASER);
	layout_widget(but_laser,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_notactive_color(GTK_BUTTON(but_laser));
	g_signal_connect(but_laser,"clicked",G_CALLBACK(clicked_button_manual_laser),NULL);

	fra_vertical_speed = create_scale_vertical_speed(DEFAULT_SPEED_VERTICAL_MANUAL_MODE);

	but_manual_mode_pump = gtk_button_new_with_label(STR_BUTTON_MANUAL_OPEN);
	layout_widget(but_manual_mode_pump,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_notactive_color(GTK_BUTTON(but_manual_mode_pump));
	g_signal_connect(but_manual_mode_pump,"clicked",G_CALLBACK(clicked_button_manual_pump),NULL);

	fra_valve = create_scale_valve();

	gtk_container_add(GTK_CONTAINER(fra_mode),gri_mode);
	gtk_grid_attach(GTK_GRID(gri_mode),but_manual_mode_up    ,1,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_manual_mode_down  ,1,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_manual_mode_left  ,0,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_manual_mode_right ,2,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_laser             ,1,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),fra_vertical_speed    ,3,0,1,3);
	gtk_grid_attach(GTK_GRID(gri_mode),but_manual_mode_pump  ,4,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),fra_valve             ,5,0,1,3);

	gtk_widget_show(but_manual_mode_pump);
	gtk_widget_show(but_laser);
	gtk_widget_show(but_manual_mode_right);
	gtk_widget_show(but_manual_mode_left);
	gtk_widget_show(but_manual_mode_down);
	gtk_widget_show(but_manual_mode_up);
	gtk_widget_show(gri_mode);
	gtk_widget_show(fra_mode);

	return fra_mode;
}

static GtkWidget * create_label_mode_manual(void)
{
	GtkWidget * fra_label;
	GtkWidget * gri_label;
	GtkWidget * lab_pressure;
	GtkWidget * lab_time;
	GtkWidget * lab_angle;

	fra_label = gtk_frame_new(NULL);
	layout_widget(fra_label,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(fra_label),5);

	gri_label = gtk_grid_new();
	layout_widget(gri_label,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(gri_label),10);
	gtk_grid_set_row_homogeneous(GTK_GRID(gri_label),TRUE);
	gtk_grid_set_column_spacing(GTK_GRID(gri_label),10);
	gtk_grid_set_column_homogeneous(GTK_GRID(gri_label),FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(gri_label),5);

	lab_pressure = gtk_label_new(STR_PRESSURE);
	layout_widget(lab_pressure,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);

	lab_time = gtk_label_new(STR_TIME_JOB);
	layout_widget(lab_time,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);

	lab_angle = gtk_label_new(STR_ANGLE);
	layout_widget(lab_angle,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);

	label_manual_mode.pressure = gtk_label_new(STR_PRESSURE_DEFAULT);
	layout_widget(label_manual_mode.pressure,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);

	label_manual_mode.time = gtk_label_new(STR_TIME_JOB_DEFAULT);
	layout_widget(label_manual_mode.time,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);

	label_manual_mode.angle = gtk_label_new(STR_ANGLE_DEFAULT);
	layout_widget(label_manual_mode.angle,GTK_ALIGN_FILL,GTK_ALIGN_CENTER,TRUE,TRUE);

	gtk_container_add(GTK_CONTAINER(fra_label),gri_label);
	gtk_grid_attach(GTK_GRID(gri_label),lab_pressure,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_label),lab_time    ,0,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_label),lab_angle   ,0,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_label),label_manual_mode.pressure,1,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_label),label_manual_mode.time    ,1,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_label),label_manual_mode.angle   ,1,2,1,1);

	gtk_widget_show(label_manual_mode.angle);
	gtk_widget_show(label_manual_mode.time);
	gtk_widget_show(label_manual_mode.pressure);
	gtk_widget_show(lab_angle);
	gtk_widget_show(lab_time);
	gtk_widget_show(lab_pressure);
	gtk_widget_show(gri_label);
	gtk_widget_show(fra_label);

	return fra_label;
}

static GtkWidget * create_mode_manual(void)
{
	GtkWidget * lab_fra_mode_manual;
	GtkWidget * box_horizontal;
	GtkWidget * gri_button;
	GtkWidget * gri_label;

	fra_mode_manual = gtk_frame_new(NULL);
	gtk_frame_set_label_align(GTK_FRAME(fra_mode_manual),0.5,0.5);
	layout_widget(fra_mode_manual,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	g_signal_connect(fra_mode_manual,"show",G_CALLBACK(show_frame_manual_mode),NULL);
	g_signal_connect(fra_mode_manual,"hide",G_CALLBACK(hide_frame_manual_mode),NULL);

	lab_fra_mode_manual = gtk_label_new(STR_MODE_MANUAL);
	set_size_font(lab_fra_mode_manual,SIZE_FONT_MEDIUM);

	box_horizontal = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
	layout_widget(box_horizontal,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);

	gri_button = create_button_mode_manual();
	gri_label = create_label_mode_manual();

	gtk_frame_set_label_widget(GTK_FRAME(fra_mode_manual),lab_fra_mode_manual);
	gtk_container_add(GTK_CONTAINER(fra_mode_manual),box_horizontal);
	gtk_box_pack_start(GTK_BOX(box_horizontal),gri_button,TRUE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_horizontal),gri_label,TRUE,TRUE,5);

	gtk_widget_show(box_horizontal);
	gtk_widget_show(lab_fra_mode_manual);
	gtk_widget_hide(fra_mode_manual);

	return fra_mode_manual;
}


/*****************************************************************************/
/* Панель загрузить работу                                                   */
/*****************************************************************************/

enum {
	COLUMN_NAME = 0,
	NUM_COLUMN
};

static char * select_row_activated(GtkTreeView * tre_job)
{
	int rc ;
	char * name = NULL;
	GtkTreeModel * model;
	GtkTreeIter iter;

	GtkTreeSelection * select =	gtk_tree_view_get_selection (tre_job);
	rc = gtk_tree_selection_get_selected(select,&model,&iter);
	if(rc == TRUE){
		gtk_tree_model_get(model,&iter,COLUMN_NAME,&name,-1);
	}
	return name;
}

static int select_current_job(GtkTreeView * tre_job)
{
	gpointer pt = NULL;
	char * name;

	name = select_row_activated(tre_job);
	if(name == NULL){
		g_critical("Cписок выбора не корректный (0)!");
		return FAILURE;
	}

	g_string_truncate(temp_job.name,0);
	g_string_append(temp_job.name,name);

	g_hash_table_lookup_extended(list_job,&temp_job,&pt,NULL);
	if(pt == NULL){
		g_critical("Таблица хешей не корректна (0)!");
		return FAILURE;
	}
	current_job = (job_s *)pt;
	set_current_value_info();

	return SUCCESS;
}

static void clicked_button_load_job(GtkButton * but,GtkTreeView * tre_job)
{
	select_current_job(tre_job);
	if(first_auto_mode == OK){
		first_auto_mode = NOT_OK;
		select_frame(AUTO_MODE_FRAME);
	}
	else{
		select_frame(INFO_FRAME);
	}
}

static void clicked_button_del_job(GtkButton * but,GtkTreeView * tre_job)
{
	char * name;
	int rc;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreeSelection * select;

	name = select_row_activated(tre_job);
	if(name == NULL){
		g_critical("Cписок выбора не корректный (1)!");
		return ;
	}

	g_string_truncate(temp_job.name,0);
	g_string_append(temp_job.name,name);

	rc = g_hash_table_remove(list_job,&temp_job);
	if(rc != TRUE){
		g_critical("Таблица хешей не корректна (1)!");
		return;
	}

	delete_job_db(name); /*из базы данных*/

	select =	gtk_tree_view_get_selection (tre_job);
	rc = gtk_tree_selection_get_selected(select,&model,&iter);
	if(rc == TRUE){
		gtk_list_store_remove(GTK_LIST_STORE(model),&iter);
	}

	current_job = NULL;

	set_current_value_info();
	select_frame(LOAD_JOB_FRAME);
	return ;
}

static int sort_model_list_job(GtkTreeModel *model
                       ,GtkTreeIter  *a
                       ,GtkTreeIter  *b
                       ,gpointer      userdata)
{
	gint rc = 0;
	gchar *name1, *name2;

  gtk_tree_model_get(model, a, COLUMN_NAME, &name1, -1);
  gtk_tree_model_get(model, b, COLUMN_NAME, &name2, -1);

	if (name1 == NULL || name2 == NULL){
		if (name1 == NULL && name2 == NULL)
			return rc; /* both equal => ret = 0 */
		rc = (name1 == NULL) ? -1 : 1;
	}
	else{
		rc = g_utf8_collate(name1,name2);
	}
	g_free(name1);
	g_free(name2);
	return rc;
}

static GtkTreeModel * create_model_list_job(void)
{
	GtkListStore * model;
	GtkTreeSortable *sortable;
	GtkTreeIter iter;
	job_s * pj_temp;

	model = gtk_list_store_new(NUM_COLUMN,G_TYPE_STRING);

	job_iter_init();
	for(;;){
		pj_temp = job_iter_next();
		if(pj_temp == NULL){
			break;
		}
		gtk_list_store_append(model,&iter);
		gtk_list_store_set(model,&iter,COLUMN_NAME,pj_temp->name->str,-1);
	}
	sortable = GTK_TREE_SORTABLE(model);
	gtk_tree_sortable_set_sort_func(sortable,0,sort_model_list_job,NULL,NULL);
	gtk_tree_sortable_set_sort_column_id(sortable,0,GTK_SORT_ASCENDING);
	return GTK_TREE_MODEL(sortable);
}

static void row_activated_tree_job(GtkTreeView *tree_view
                           ,GtkTreePath *path
                           ,GtkTreeViewColumn *column
                           ,gpointer           user_data)
{
	select_current_job(tree_view);
	if(first_auto_mode == OK){
		first_auto_mode = NOT_OK;
		select_frame(AUTO_MODE_FRAME);
	}
	else{
		select_frame(INFO_FRAME);
	}
}

static GtkWidget * lab_tree_pressure = NULL;
static GtkWidget * lab_tree_time = NULL;
static GtkWidget * lab_tree_uprise = NULL;
static GtkWidget * lab_tree_lowering = NULL;

static int set_current_value_tree(void)
{
	if(current_job == NULL){
		gtk_label_set_text(GTK_LABEL(lab_tree_pressure),STR_PRESSURE_DEFAULT);
		gtk_label_set_text(GTK_LABEL(lab_tree_time),STR_TIME_JOB_DEFAULT);
		gtk_label_set_text(GTK_LABEL(lab_tree_uprise),STR_ANGLE_DEFAULT);
		gtk_label_set_text(GTK_LABEL(lab_tree_lowering),STR_ANGLE_DEFAULT);
	}
	else{
		g_string_printf(temp_string,FORMAT_PRESSURE,PRINTF_PRESSURE(current_job->pressure));
		gtk_label_set_text(GTK_LABEL(lab_tree_pressure),temp_string->str);
		g_string_printf(temp_string,"%02d:%02d:%02d"
		               ,get_hour_in_second(current_job->time)
		               ,get_minute_in_second(current_job->time)
		               ,get_second_in_second(current_job->time));
		gtk_label_set_text(GTK_LABEL(lab_tree_time),temp_string->str);
		g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(current_job->uprise));
		gtk_label_set_text(GTK_LABEL(lab_tree_uprise),temp_string->str);
		g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(current_job->lowering));
		gtk_label_set_text(GTK_LABEL(lab_tree_lowering),temp_string->str);
	}
	return SUCCESS;
}

static void cursor_changed_tree_job(GtkTreeView *tree_view,gpointer ud)
{
	select_current_job(tree_view);
	if(current_job == NULL){
		return;
	}
	set_current_value_tree();
}

static char STR_NAME_COLUMN_JOB[] = "Наименование Работы";
static int add_column_tree_job(GtkTreeView * tv)
{
	GtkCellRenderer * renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer,"editable",FALSE,NULL);/*установка свойств*/

	column = gtk_tree_view_column_new_with_attributes(STR_NAME_COLUMN_JOB,renderer,"text",COLUMN_NAME,NULL);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 500);

	gtk_tree_view_column_set_sort_order(GTK_TREE_VIEW_COLUMN(column),GTK_SORT_ASCENDING);
	gtk_tree_view_column_set_sort_indicator(GTK_TREE_VIEW_COLUMN(column),TRUE);
	gtk_tree_view_append_column (tv, column);

	/*gtk_tree_view_set_tooltip_column(tv,COLUMN_NAME); подсказка */

	return 0;
}

static GtkWidget * create_info_load_tree(void)
{
	GtkWidget * fra_tree;
	GtkWidget * gri_info;
	GtkWidget * lab_pressure;
	GtkWidget * lab_time;
	GtkWidget * lab_uprise;
	GtkWidget * lab_lowering;

	fra_tree = gtk_frame_new(NULL);
	layout_widget(fra_tree,GTK_ALIGN_END,GTK_ALIGN_FILL,FALSE,TRUE);

	gri_info = gtk_grid_new();
	layout_widget(gri_info,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(gri_info),10);
	gtk_grid_set_row_homogeneous(GTK_GRID(gri_info),FALSE);
	gtk_grid_set_column_spacing(GTK_GRID(gri_info),10);
	gtk_grid_set_column_homogeneous(GTK_GRID(gri_info),TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(gri_info),5);

	lab_pressure = gtk_label_new(STR_PRESSURE);
	lab_tree_pressure = gtk_label_new(STR_PRESSURE_DEFAULT);

	lab_time = gtk_label_new(STR_TIME_JOB);
	lab_tree_time = gtk_label_new(STR_TIME_JOB_DEFAULT);

	lab_uprise = gtk_label_new(STR_UPRISE_ANGEL);
	lab_tree_uprise = gtk_label_new(STR_ANGLE_DEFAULT);

	lab_lowering = gtk_label_new(STR_LOWERING_ANGEL);
	lab_tree_lowering = gtk_label_new(STR_ANGLE_DEFAULT);

	gtk_container_add(GTK_CONTAINER(fra_tree),GTK_WIDGET(gri_info));
	gtk_grid_attach(GTK_GRID(gri_info),lab_pressure     ,0,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_info),lab_tree_pressure,1,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_info),lab_time         ,0,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_info),lab_tree_time    ,1,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_info),lab_uprise       ,0,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_info),lab_tree_uprise  ,1,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_info),lab_lowering     ,0,4,1,1);
	gtk_grid_attach(GTK_GRID(gri_info),lab_tree_lowering,1,4,1,1);

	gtk_widget_show(lab_tree_lowering);
	gtk_widget_show(lab_lowering);
	gtk_widget_show(lab_tree_uprise);
	gtk_widget_show(lab_uprise);
	gtk_widget_show(lab_tree_time);
	gtk_widget_show(lab_time);
	gtk_widget_show(lab_tree_pressure);
	gtk_widget_show(lab_pressure);
	gtk_widget_show(GTK_WIDGET(gri_info));
	gtk_widget_show(fra_tree);
	return fra_tree;
}
static char STR_DEL_JOB[] = "Удалить";

static GtkWidget * tree_view_job;

static GtkWidget * create_job_load(void)
{
	GtkWidget * lab_fra_job_load;
	GtkWidget * box_main;
	GtkWidget * box_tree_view;
	GtkWidget * scrwin_job;
	GtkTreeModel * tremod_job;
	GtkWidget * fra_info;
	GtkWidget * box_button;
	GtkWidget * but_load;
	GtkWidget * but_del;

	fra_job_load = gtk_frame_new(NULL);
	gtk_frame_set_label_align(GTK_FRAME(fra_job_load),0.5,0.5);
	layout_widget(fra_job_load,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);

	lab_fra_job_load = gtk_label_new(STR_JOB_LOAD);
	set_size_font(lab_fra_job_load,SIZE_FONT_MEDIUM);

	box_main = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
	layout_widget(box_main,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);

	box_tree_view = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
	layout_widget(box_tree_view,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_box_set_homogeneous(GTK_BOX(box_tree_view),FALSE);

	scrwin_job  = gtk_scrolled_window_new (NULL,NULL);
	layout_widget(scrwin_job,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrwin_job),GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrwin_job),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (scrwin_job,300,200);

	tremod_job = create_model_list_job();

	tree_view_job = gtk_tree_view_new_with_model(tremod_job);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view_job), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view_job))
	                           ,GTK_SELECTION_SINGLE);
	layout_widget(tree_view_job,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	g_signal_connect(tree_view_job,"row-activated",G_CALLBACK(row_activated_tree_job),NULL);
	g_signal_connect(tree_view_job,"cursor-changed",G_CALLBACK(cursor_changed_tree_job),NULL);

	add_column_tree_job(GTK_TREE_VIEW(tree_view_job));

	fra_info = create_info_load_tree();

	box_button = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
	layout_widget(box_button,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	/*gtk_box_set_homogeneous(GTK_BOX(box_button),TRUE);*/
	but_load = gtk_button_new_with_label(STR_LOAD_JOB);
	gtk_widget_set_halign(but_load,GTK_ALIGN_CENTER);
	gtk_widget_set_valign(but_load,GTK_ALIGN_CENTER);
	g_signal_connect(but_load,"clicked",G_CALLBACK(clicked_button_load_job),tree_view_job);
	but_del = gtk_button_new_with_label(STR_DEL_JOB);
	gtk_widget_set_halign(but_del,GTK_ALIGN_CENTER);
	gtk_widget_set_valign(but_del,GTK_ALIGN_CENTER);
	g_signal_connect(but_del,"clicked",G_CALLBACK(clicked_button_del_job),tree_view_job);

	gtk_frame_set_label_widget(GTK_FRAME(fra_job_load),lab_fra_job_load);
	gtk_container_add(GTK_CONTAINER(fra_job_load),box_main);

	gtk_box_pack_start(GTK_BOX(box_main),box_tree_view,TRUE,TRUE,5);

	gtk_box_pack_start(GTK_BOX(box_tree_view),scrwin_job,TRUE,TRUE,5);
	gtk_container_add(GTK_CONTAINER(scrwin_job),tree_view_job);
	gtk_box_pack_start(GTK_BOX(box_tree_view),fra_info,FALSE,TRUE,5);

	gtk_box_pack_start(GTK_BOX(box_main),box_button,FALSE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_button),but_load,FALSE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_button),but_del,FALSE,TRUE,5);

	gtk_widget_show(but_del);
	gtk_widget_show(but_load);
	gtk_widget_show(box_button);
	gtk_widget_show(tree_view_job);
	gtk_widget_show(scrwin_job);
	gtk_widget_show(box_tree_view);
	gtk_widget_show(box_main);
	gtk_widget_show(lab_fra_job_load);
	gtk_widget_hide(fra_job_load);
	return fra_job_load;
}


/*****************************************************************************/
/* Панель создать работу                                                     */
/*****************************************************************************/

static char STR_NAME_JOB[] = "Наименование работ";
static char STR_NAME_JOB_DEFAULT[] = "Изделие 000";
static char STR_FIX_ANGLE[] = "Зафиксировать угол";
static char STR_SAVE_JOB[] = "Сохранить работу";

static GtkEntryBuffer * entbuff_name_job;
static GtkEntryBuffer * entbuff_pressure;
static GtkEntryBuffer * entbuff_time;
static GtkEntryBuffer * entbuff_uprise;
static GtkEntryBuffer * entbuff_lowering;

static uint16_t check_entry_pressure(const char * pressure)
{
	double d;
	d = g_strtod(pressure,NULL);
	d /= rate_pressure;

	if( (d < MIN_PRESSURE_IN_TIC) || (d > MAX_PRESSURE_IN_TIC) ){
		return FAILURE;
	}
	return SUCCESS;
}

static long int value_uprise = ANGLE_NULL;
static long int value_lowering = ANGLE_NULL;

static int check_entry_angle(const char * uprise,const char * lowering)
{
	double vu = g_strtod(uprise,NULL);
	double vl = g_strtod(lowering,NULL);

	if((vu < MIN_ANGLE) || (vu > MAX_ANGLE) ){
		vu = MAX_ANGLE;
	}
	if((vl < MIN_ANGLE) || (vl > MAX_ANGLE) ){
		vl = MIN_ANGLE;
	}

	if(value_uprise == ANGLE_NULL){
		vu /= RATE_ANGLE;
		value_uprise = vu;
	}

	if(value_lowering == ANGLE_NULL){
		vl /= RATE_ANGLE;
		value_lowering = vl;
	}

	if( (value_uprise < MIN_ANGLE_IN_TIC) || (value_uprise > MAX_ANGLE_IN_TIC)){
		value_uprise = MAX_ANGLE_IN_TIC;
	}

	if((value_lowering < MIN_ANGLE_IN_TIC) || (value_lowering > MAX_ANGLE_IN_TIC)){
		value_lowering = MIN_ANGLE_IN_TIC;
	}

	if(value_uprise <= value_lowering){
		value_uprise = ANGLE_NULL;
		value_lowering = ANGLE_NULL;
		return FAILURE;
	}
	return SUCCESS;
}

static void clicked_button_fix_uprise(GtkButton * b,gpointer d)
{
	int rc;
	uint16_t angle;

	rc = command_info();
	if(rc != SUCCESS){
		return ;
	}
	angle = info_angle();

	value_uprise = angle;
	if( (value_uprise < MIN_ANGLE_IN_TIC) || (value_uprise > MAX_ANGLE_IN_TIC)){
		value_uprise = MAX_ANGLE_IN_TIC;
	}
	g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(angle));
	gtk_entry_buffer_set_text(entbuff_uprise,temp_string->str,-1);
}

static void clicked_button_fix_lowering(GtkButton * b,gpointer d)
{
	int rc;
	uint16_t angle;

	rc = command_info();
	if(rc != SUCCESS){
		return ;
	}
	angle = info_angle();

	value_lowering = angle;
	if((value_lowering < MIN_ANGLE_IN_TIC) || (value_lowering > MAX_ANGLE_IN_TIC)){
		value_lowering = MIN_ANGLE_IN_TIC;
	}
	g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(angle));
	gtk_entry_buffer_set_text(entbuff_lowering,temp_string->str,-1);
}

static void clicked_button_save_job(GtkButton * b,gpointer d)
{
	int rc;
	gpointer pt = NULL;
	GtkListStore * model;
	GtkTreeIter iter;
	const char * name = gtk_entry_buffer_get_text(entbuff_name_job);
	const char * pressure = gtk_entry_buffer_get_text(entbuff_pressure);
	const char * time = gtk_entry_buffer_get_text(entbuff_time);
	const char * uprise = gtk_entry_buffer_get_text(entbuff_uprise);
	const char * lowering = gtk_entry_buffer_get_text(entbuff_lowering);

	rc = check_entry_pressure(pressure);
	if(rc != SUCCESS){
		GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
		                              ,GTK_BUTTONS_CLOSE,"Введеное давление не корректно!");
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		return;
	}

	rc = str_to_time_second(time);
	if(rc == 0){
		GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
		                              ,GTK_BUTTONS_CLOSE,"Введеное время не корректно!");
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		return;
	}

	rc = check_entry_angle(uprise,lowering);
	if(rc != SUCCESS){
		GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE
		                                          ,"%s","Углы пересекаются");
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		return;
	}
	rc = insert_job_db(name,pressure,time,value_uprise,value_lowering);
	if(rc == MATCH_NAME){
		GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
		                              ,GTK_BUTTONS_CLOSE,"Введеное имя работы есть в базе данных!");
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		return;
	}
	if(rc == INVALID_NAME){
		GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
		                              ,GTK_BUTTONS_CLOSE,"Введеное имя работы содержит \" , \"!");
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		return;
	}

	if(rc == FAILURE){
		GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
		                              ,GTK_BUTTONS_CLOSE,"Нет возможности записать в базу данных!");
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		return;
	}

	model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view_job)));
	gtk_list_store_append(model,&iter);
	gtk_list_store_set(model,&iter,COLUMN_NAME,name,-1);

	g_string_truncate(temp_job.name,0);
	g_string_append(temp_job.name,name);

	pt = NULL;
	g_hash_table_lookup_extended(list_job,&temp_job,&pt,NULL);
	if(pt == NULL){
		g_critical("Таблица хешей не корректна (2)!");
		GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
		                              ,GTK_BUTTONS_CLOSE,"Нет возможности записать в базу данных!");
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		current_job = NULL;
	}
	else{
		current_job = (job_s *)pt;
	}

	set_current_value_info();
	select_frame(INFO_FRAME);
	return ;
}

static GtkWidget * create_entry_job_save(void)
{
	GtkWidget * gri_entry;
	GtkWidget * lab_name;
	GtkWidget * lab_pressure;
	GtkWidget * lab_time;
	GtkWidget * lab_uprise;
	GtkWidget * lab_lowering;
	GtkWidget * ent_name;
	GtkWidget * ent_pressure;
	GtkWidget * ent_time;
	GtkWidget * ent_uprise;
	GtkWidget * ent_lowering;
	GtkWidget * but_fix_uprise;
	GtkWidget * but_fix_lowering;
	GtkWidget * but_save;

	gri_entry = gtk_grid_new();
	layout_widget(gri_entry,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(gri_entry),10);
	gtk_grid_set_column_spacing(GTK_GRID(gri_entry),10);
	gtk_grid_set_column_homogeneous(GTK_GRID(gri_entry),FALSE);

	lab_name = gtk_label_new(STR_NAME_JOB);
	gtk_widget_set_halign(lab_name,GTK_ALIGN_START);
	lab_pressure = gtk_label_new(STR_PRESSURE);
	gtk_widget_set_halign(lab_pressure,GTK_ALIGN_START);
	lab_time = gtk_label_new(STR_TIME_JOB);
	gtk_widget_set_halign(lab_time,GTK_ALIGN_START);
	lab_uprise = gtk_label_new(STR_UPRISE_ANGEL);
	gtk_widget_set_halign(lab_uprise,GTK_ALIGN_START);
	lab_lowering = gtk_label_new(STR_LOWERING_ANGEL);
	gtk_widget_set_halign(lab_lowering,GTK_ALIGN_START);

	entbuff_name_job = gtk_entry_buffer_new(STR_NAME_JOB_DEFAULT,-1);
	ent_name = gtk_entry_new_with_buffer(entbuff_name_job);
	gtk_widget_set_hexpand(ent_name,TRUE);
	gtk_widget_set_vexpand(ent_name,FALSE);

	g_string_printf(temp_string,FORMAT_PRESSURE,PRINTF_PRESSURE(DEFAULT_PRESSURE_IN_TIC));
	entbuff_pressure = gtk_entry_buffer_new(temp_string->str,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(entbuff_pressure),3);
	ent_pressure = gtk_entry_new_with_buffer(entbuff_pressure);
	layout_widget(ent_pressure,GTK_ALIGN_START,GTK_ALIGN_START,FALSE,TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(ent_pressure),3);


	entbuff_time = gtk_entry_buffer_new(STR_TIME_JOB_DEFAULT,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(entbuff_time),8);
	ent_time = gtk_entry_new_with_buffer(entbuff_time);
	layout_widget(ent_time,GTK_ALIGN_START,GTK_ALIGN_START,FALSE,TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(ent_time),8);

	entbuff_uprise = gtk_entry_buffer_new(STR_ANGLE_DEFAULT,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(entbuff_uprise),MAX_LEN_STR_ANGLE);
	ent_uprise = gtk_entry_new_with_buffer(entbuff_uprise);
	layout_widget(ent_uprise,GTK_ALIGN_START,GTK_ALIGN_START,FALSE,TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(ent_uprise),MAX_LEN_STR_ANGLE);

	entbuff_lowering = gtk_entry_buffer_new(STR_ANGLE_DEFAULT,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(entbuff_lowering),MAX_LEN_STR_ANGLE);
	ent_lowering = gtk_entry_new_with_buffer(entbuff_lowering);
	layout_widget(ent_lowering,GTK_ALIGN_START,GTK_ALIGN_START,FALSE,TRUE);
	gtk_entry_set_width_chars(GTK_ENTRY(ent_lowering),MAX_LEN_STR_ANGLE);

	but_fix_uprise = gtk_button_new_with_label(STR_FIX_ANGLE);
	layout_widget(but_fix_uprise,GTK_ALIGN_START,GTK_ALIGN_START,FALSE,TRUE);
	g_signal_connect(but_fix_uprise,"clicked",G_CALLBACK(clicked_button_fix_uprise),NULL);

	but_fix_lowering = gtk_button_new_with_label(STR_FIX_ANGLE);
	layout_widget(but_fix_lowering,GTK_ALIGN_START,GTK_ALIGN_START,FALSE,TRUE);
	g_signal_connect(but_fix_lowering,"clicked",G_CALLBACK(clicked_button_fix_lowering),NULL);

	but_save = gtk_button_new_with_label(STR_SAVE_JOB);
	gtk_widget_set_hexpand(but_save,FALSE);
	gtk_widget_set_vexpand(but_save,TRUE);
	g_signal_connect(but_save,"clicked",G_CALLBACK(clicked_button_save_job),NULL);

	gtk_grid_attach(GTK_GRID(gri_entry),lab_name        ,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),lab_pressure    ,0,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),lab_time        ,0,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),lab_uprise      ,0,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),lab_lowering    ,0,4,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),ent_name        ,1,0,2,1);
	gtk_grid_attach(GTK_GRID(gri_entry),ent_pressure    ,1,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),ent_time        ,1,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),ent_uprise      ,1,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),ent_lowering    ,1,4,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),but_fix_uprise  ,2,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),but_fix_lowering,2,4,1,1);
	gtk_grid_attach(GTK_GRID(gri_entry),but_save        ,0,5,1,1);

	gtk_widget_show(but_save);
	gtk_widget_show(but_fix_lowering);
	gtk_widget_show(but_fix_uprise);
	gtk_widget_show(ent_lowering);
	gtk_widget_show(ent_uprise);
	gtk_widget_show(ent_time);
	gtk_widget_show(ent_pressure);
	gtk_widget_show(ent_name);
	gtk_widget_show(lab_lowering);
	gtk_widget_show(lab_uprise);
	gtk_widget_show(lab_time);
	gtk_widget_show(lab_pressure);
	gtk_widget_show(lab_name);
	gtk_widget_show(gri_entry);
 	return gri_entry;
}

static GtkWidget * lab_angle_save_job;

static GtkWidget * but_save_job_up;
static GtkWidget * but_save_job_down;

static GtkWidget * create_button_job_save(void)
{
	GtkWidget * fra_button;
	GtkWidget * gri_button;
	GtkWidget * lab_angle;
	GtkWidget * fra_scale;

	fra_button = gtk_frame_new(NULL);
	layout_widget(fra_button,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(fra_button),10);

	gri_button = gtk_grid_new();
	layout_widget(gri_button,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(gri_button),10);
	gtk_grid_set_row_spacing(GTK_GRID(gri_button),10);
	gtk_grid_set_column_spacing(GTK_GRID(gri_button),10);

	but_save_job_up = gtk_button_new_with_label(STR_BUTTON_MANUAL_UP);
	layout_widget(but_save_job_up,GTK_ALIGN_CENTER,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_save_job_up)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_save_job_up));
	g_signal_connect(but_save_job_up,"button-press-event",G_CALLBACK(press_button_manual_up),NULL);
	g_signal_connect(but_save_job_up,"button-release-event",G_CALLBACK(release_button_manual_up),NULL);

	but_save_job_down = gtk_button_new_with_label(STR_BUTTON_MANUAL_DOWN);
	layout_widget(but_save_job_down,GTK_ALIGN_CENTER,GTK_ALIGN_CENTER,TRUE,TRUE);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_save_job_down)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_save_job_down));
	g_signal_connect(but_save_job_down,"button-press-event",G_CALLBACK(press_button_manual_down),NULL);
	g_signal_connect(but_save_job_down,"button-release-event",G_CALLBACK(release_button_manual_down),NULL);

	lab_angle = gtk_label_new(STR_ANGLE);
	set_size_font(lab_angle,SIZE_FONT_SMALL);

	lab_angle_save_job = gtk_label_new(STR_ANGLE_DEFAULT);
	set_size_font(lab_angle_save_job,SIZE_FONT_SMALL);

	fra_scale = create_scale_vertical_speed(DEFAULT_SPEED_VERTICAL_SAVE_JOB);

	gtk_container_add(GTK_CONTAINER(fra_button),gri_button);
	gtk_grid_attach(GTK_GRID(gri_button),but_save_job_up   ,1,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_button),but_save_job_down ,1,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_button),lab_angle         ,0,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_button),lab_angle_save_job,1,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_button),fra_scale         ,2,0,1,3);

	gtk_widget_show(fra_scale);
	gtk_widget_show(lab_angle_save_job);
	gtk_widget_show(lab_angle);
	gtk_widget_show(but_save_job_down);
	gtk_widget_show(but_save_job_up);
	gtk_widget_show(gri_button);
	gtk_widget_show(fra_button);
	return fra_button;
}

static int config_mode = NOT_OK;

static int check_registers_config_mode(gpointer ud)
{
	int rc;
	uint16_t angle;

	if(config_mode != OK){
		return FALSE;
	}
	rc = command_info();
	if(rc != SUCCESS){
		return FALSE;
	}

	angle = info_angle();

	if( console_up_old != info_console_up()){
		console_up_old = info_console_up();
		if(console_up_old){
			press_button_manual_up(GTK_BUTTON(but_save_job_up),NULL,NULL);
		}
		else{
			release_button_manual_up(GTK_BUTTON(but_save_job_up),NULL,NULL);
		}
	}
	if(console_down_old != info_console_down()){
		console_down_old = info_console_down();
		if(console_down_old){
			press_button_manual_down(GTK_BUTTON(but_save_job_down),NULL,NULL);
		}
		else{
			release_button_manual_down(GTK_BUTTON(but_save_job_down),NULL,NULL);
		}
	}
	g_string_printf(temp_string,FORMAT_ANGLE,PRINTF_ANGLE(angle));
	gtk_label_set_text(GTK_LABEL(lab_angle_save_job),temp_string->str);
	return TRUE;
}

static int timeout_config_mode = DEFAULT_TIMEOUT_CHECK;

static void show_frame_save_job(GtkWidget * w,gpointer ud)
{
	int rc;

	rc = command_config_mode();
	if(rc == SUCCESS){
		command_speed_vertical(DEFAULT_SPEED_VERTICAL_SAVE_JOB);
		value_uprise = ANGLE_NULL;
		value_lowering = ANGLE_NULL;
		config_mode = OK;
		console_up_old = 0;
		console_down_old = 0;
		g_timeout_add(timeout_config_mode,check_registers_config_mode,NULL);
	}
}

static void hide_frame_save_job(GtkWidget * w,gpointer ud)
{
	config_mode = NOT_OK;
	command_null_mode();
}

static GtkWidget * create_job_save(void)
{
	GtkWidget * lab_fra_job_save;
	GtkWidget * box_horizontal;
	GtkWidget * temp;


	fra_job_save = gtk_frame_new(NULL);
	layout_widget(fra_job_save,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_frame_set_label_align(GTK_FRAME(fra_job_save),0.5,0.5);
	g_signal_connect(fra_job_save,"show",G_CALLBACK(show_frame_save_job),NULL);
	g_signal_connect(fra_job_save,"hide",G_CALLBACK(hide_frame_save_job),NULL);

	lab_fra_job_save = gtk_label_new(STR_JOB_SAVE);
	set_size_font(lab_fra_job_save,SIZE_FONT_MEDIUM);

	box_horizontal = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);

	gtk_frame_set_label_widget(GTK_FRAME(fra_job_save),lab_fra_job_save);
	gtk_container_add(GTK_CONTAINER(fra_job_save),box_horizontal);

	temp = create_entry_job_save();
	gtk_box_pack_start(GTK_BOX(box_horizontal),temp,TRUE,TRUE,5);

	temp = create_button_job_save();
	gtk_box_pack_start(GTK_BOX(box_horizontal),temp,TRUE,TRUE,5);

	gtk_widget_show(box_horizontal);
	gtk_widget_show(lab_fra_job_save);
	gtk_widget_hide(fra_job_save);
	return fra_job_save;
}

/*****************************************************************************/
/* основное окно                                                             */
/*****************************************************************************/

static char STR_HORIZONTAL_OFFSET[] =   "horizontal";
static char STR_TIMEOUT_AUTO_MODE[] =   "timeout_auto_mode";
static char STR_TIMEOUT_MANUAL_MODE[] = "timeout_manual_mode";
static char STR_TIMEOUT_CONFIG_MODE[] = "timeout_config_mode";
static char STR_RATE_PRESSURE[] = "rate_pressure";

static int load_config(void)
{
	int rc;
	double d;
	GError * err = NULL;

	rc = g_key_file_get_integer(ini_file,STR_GLOBAL_KEY,STR_HORIZONTAL_OFFSET,&err);
	if(err != NULL){
		g_critical("В секции %s нет ключа %s!",STR_GLOBAL_KEY,STR_HORIZONTAL_OFFSET);
		g_error_free(err);
	}
	else{
		if( ((rc < MIN_HORIZONTAL_OFFSET_IN_TIC) || (rc > MAX_HORIZONTAL_OFFSET_IN_TIC)) ){
			rc = DEFAULT_HORIZONTAL_OFFSET;
		}
		horizontal_offset = rc;
	}

	err = NULL;
	rc = g_key_file_get_integer(ini_file,STR_GLOBAL_KEY,STR_TIMEOUT_AUTO_MODE,&err);
	if(err != NULL){
		g_critical("В секции %s нет ключа %s!",STR_GLOBAL_KEY,STR_TIMEOUT_AUTO_MODE);
		g_error_free(err);
	}
	else{
		if( ((rc < MIN_TIMEOUT_CHECK) || (rc > MAX_TIMEOUT_CHECK)) ){
			rc = DEFAULT_TIMEOUT_CHECK;
		}
		timeout_auto_mode = rc;
	}

	err = NULL;
	rc = g_key_file_get_integer(ini_file,STR_GLOBAL_KEY,STR_TIMEOUT_MANUAL_MODE,&err);
	if(err != NULL){
		g_critical("В секции %s нет ключа %s!",STR_GLOBAL_KEY,STR_TIMEOUT_MANUAL_MODE);
		g_error_free(err);
	}
	else{
		if( ((rc < MIN_TIMEOUT_CHECK) || (rc > MAX_TIMEOUT_CHECK)) ){
			rc = DEFAULT_TIMEOUT_CHECK;
		}
		timeout_manual_mode = rc;
	}

	err = NULL;
	rc = g_key_file_get_integer(ini_file,STR_GLOBAL_KEY,STR_TIMEOUT_CONFIG_MODE,&err);
	if(err != NULL){
		g_critical("В секции %s нет ключа %s!",STR_GLOBAL_KEY,STR_TIMEOUT_CONFIG_MODE);
		g_error_free(err);
	}
	else{
		if( ((rc < MIN_TIMEOUT_CHECK) || (rc > MAX_TIMEOUT_CHECK)) ){
			rc = DEFAULT_TIMEOUT_CHECK;
		}
		timeout_config_mode = rc;
	}

	err = NULL;
	rc = g_key_file_get_integer(ini_file,STR_GLOBAL_KEY,STR_VALVE_TIC,&err);
	if(err != NULL){
		g_critical("В секции %s нет ключа %s!",STR_GLOBAL_KEY,STR_VALVE_TIC);
		g_error_free(err);
	}
	else{
		if( ((rc < MIN_VALVE_IN_TIC) || (rc > MAX_VALVE_IN_TIC)) ){
			rc = MIN_VALVE_IN_TIC;
		}
		valve_ui = rc;
	}

	err = NULL;
	d = g_key_file_get_double(ini_file,STR_GLOBAL_KEY,STR_RATE_PRESSURE,&err);
	if(err != NULL){
		g_critical("В секции %s нет ключа %s!",STR_GLOBAL_KEY,STR_RATE_PRESSURE);
		g_error_free(err);
	}
	else{
		if( ((d < MIN_RATE_PRESSURE) || (d > MAX_RATE_PRESSURE)) ){
			d = DEFAULT_RATE_PRESSURE;
		}
		rate_pressure = d;
	}

	return SUCCESS;
}

GtkWidget * create_control_panel(void)
{
	GtkWidget * fra_control;
	GtkWidget * gri_control;

	load_config();

	fra_control = gtk_frame_new(NULL);
	layout_widget(fra_control,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(fra_control),5);

	gri_control = gtk_grid_new();
	layout_widget(gri_control,GTK_ALIGN_FILL,GTK_ALIGN_FILL,TRUE,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(gri_control),10);
	gtk_widget_set_size_request(gri_control,-1,300);

	fra_info = create_info();
	fra_mode_auto = create_mode_auto();
	fra_mode_manual = create_mode_manual();
	fra_job_load = create_job_load();
	fra_job_save = create_job_save();

	gtk_container_add(GTK_CONTAINER(fra_control),gri_control);
	gtk_grid_attach(GTK_GRID(gri_control),fra_info       ,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_control),fra_mode_auto  ,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_control),fra_mode_manual,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_control),fra_job_load   ,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_control),fra_job_save   ,0,0,1,1);
	select_frame(INFO_FRAME);

	gtk_widget_show(gri_control);
	gtk_widget_show(fra_control);
	return fra_control;
}

int deinit_job(void)
{
	deini_manual_mode();
	return SUCCESS;
}
/*****************************************************************************/
