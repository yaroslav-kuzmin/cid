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
#include <glib/gprintf.h> /*TODO test only*/
#include <gtk/gtk.h>
#include <sqlite3.h>

#include <modbus.h>
#include "total.h"
#include "cid.h"
#include "video.h"
#include "control.h"


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
char QUIER_INSERT_ROW_JOB[] = "INSERT INTO job VALUES (";
#define DEFAULT_AMOUNT_COLUMN        5

#define NUMBER_NAME_COLUMN           0
#define NUMBER_PRESSURE_COLUMN       1
#define NUMBER_TIME_COLUMN           2
#define NUMBER_UPRISE_COLUMN         3
#define NUMBER_LOWERING_COLUMN       4

static const char NAME_COLUMN_0[] = "name";
#define SIZE_NAME_COLUMN_0           4
static const char NAME_COLUMN_1[] = "pressure";
#define SIZE_NAME_COLUMN_1           8
static const char NAME_COLUMN_2[] = "time";
#define SIZE_NAME_COLUMN_2           4
static const char NAME_COLUMN_3[] = "uprise";
#define SIZE_NAME_COLUMN_3           6
static const char NAME_COLUMN_4[] = "lowering";
#define SIZE_NAME_COLUMN_4           8

typedef struct _job_s job_s;
struct _job_s
{
	GString * name;
	int pressure;
	GDateTime * time; /**TODO преобразовать в секунды*/
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
	GDateTime *t = j->time;
	g_string_free(s,TRUE);
	g_date_time_unref(t);
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
		g_critical("SQL error QUERY_ALL_JOB : %s",sqlite3_errmsg(database));
		return FAILURE;
	}
	for(;;){
		rc = sqlite3_step(query);
		if(rc == SQLITE_DONE){
			break;	/* данных в запросе нет*/
		}
		if(rc == SQLITE_ERROR ){
			g_critical("SQL error QUERY_ALL_JOB step : %s",sqlite3_errmsg(database));
			break;
		}
		if(rc == SQLITE_ROW){
			const char * str;
			gint64 t;

			if(check_column != OK){
				int amount_column;
				amount_column = sqlite3_data_count(query);
				if(amount_column != DEFAULT_AMOUNT_COLUMN){
					g_critical("SQL error not correct table job : %d",amount_column);
					break;
				}
				str = sqlite3_column_name(query,0);
				str = g_strstr_len(NAME_COLUMN_0,SIZE_NAME_COLUMN_0,str);
				if(str == NULL){
					g_critical("SQL error not correct column 0 ");
					break;
				}
				str = sqlite3_column_name(query,1);
				str = g_strstr_len(NAME_COLUMN_1,SIZE_NAME_COLUMN_1,str);
				if(str == NULL){
					g_critical("SQL error not correct column 1");
					break;
				}
				str = sqlite3_column_name(query,2);
				str = g_strstr_len(NAME_COLUMN_2,SIZE_NAME_COLUMN_2,str);
				if(str == NULL){
					g_critical("SQL error not correct column 2");
					break;
				}
				str = sqlite3_column_name(query,3);
				str = g_strstr_len(NAME_COLUMN_3,SIZE_NAME_COLUMN_3,str);
				if(str == NULL){
					g_critical("SQL error not correct column 3");
					break;
				}
				str = sqlite3_column_name(query,4);
				str = g_strstr_len(NAME_COLUMN_4,SIZE_NAME_COLUMN_4,str);
				if(str == NULL){
					g_critical("SQL error not correct column 4");
					break;
				}
				check_column = OK;
			}
			job = g_slice_alloc0(sizeof(job_s));
			str = (char *)sqlite3_column_text(query,NUMBER_NAME_COLUMN);
			job->name = g_string_new(str);
			job->pressure = sqlite3_column_int64(query,NUMBER_PRESSURE_COLUMN);
			t = sqlite3_column_int64(query,NUMBER_TIME_COLUMN);
			job->time = g_date_time_new_from_unix_local(t);
			job->uprise = sqlite3_column_int64(query,NUMBER_UPRISE_COLUMN);
			job->lowering = sqlite3_column_int64(query,NUMBER_LOWERING_COLUMN);
			g_hash_table_add(list_job,job);
			continue;
		}
		g_critical("SQL error QUERY_ALL_JOB end");
		break;
	}
	sqlite3_finalize(query);
	return SUCCESS;
}

static GDateTime * str_time_to_datetime(const char * str)
{
	gint hour = 0;
	gint minut = 0;
	gint second = 0;

	/*str 00:00:00*/
	if((str[2] != ':') || (str[5] != ':')){
		return NULL;
	}

	hour = str[0] - '0';
	hour *= 10;
	hour += (str[1] - '0');
	if( (hour < 0) || (hour > 24) ){
		return NULL;
	}

	minut = str[3] - '0';
	minut *= 10;
	minut += (str[4] - '0');
	if((minut < 0)||(minut>59)){
		return NULL;
	}
	second = str[6] - '0';
	second *= 10;
	second += (str[7] - '0');
	if((second < 0) || (second > 59)){
		return NULL;
	}
	return g_date_time_new_local(2015,9,6,hour,minut,(gdouble)second);
}
#define MATCH_NAME      -1
#define INVALID_NAME    -2

job_s temp_job = {0};
GString * query_row_job = NULL;

/*проверка введеных значений лежит на вышестояшей функции*/
static int insert_job_db(const char * name,const char *pressure,const char * time
              ,long int uprise,long int lowering )
{
	job_s * pj;
	char * sql_error;
	int rc;

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
	pj->pressure = g_ascii_strtoll(pressure,NULL,10);
	pj->time = str_time_to_datetime(time);
	pj->uprise = uprise;
	pj->lowering = lowering;

	g_hash_table_add(list_job,pj);

	if(query_row_job == NULL){
		query_row_job = g_string_new(QUIER_INSERT_ROW_JOB);
	}
	g_string_printf(query_row_job,"%s",QUIER_INSERT_ROW_JOB);
	g_string_append_printf(query_row_job,"\'%s\',",pj->name->str);
	g_string_append_printf(query_row_job,"%d,",pj->pressure);
	g_string_append_printf(query_row_job,"%ld,",(long int)g_date_time_to_unix(pj->time));
	g_string_append_printf(query_row_job,"%d,%d)",pj->uprise,pj->lowering);

	rc = sqlite3_exec(database,query_row_job->str,NULL,NULL,&sql_error);
	if(rc != SQLITE_OK){
		/*TODO окно пользователю что несмог записать*/
		g_critical("SQL error QUERY_INSERT_JOB : %s",sql_error);
		sqlite3_free(sql_error);
		return FAILURE;
	}

	return SUCCESS;
}
static const char QUERY_DELETE_ROW_JOB[] = "DELETE FROM job WHERE name=";

int delete_job_db(const char * name)
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
		g_critical("SQL error QUERY_DELETE_ROW_JOB : %s",sql_error);
		sqlite3_free(sql_error);
		return FAILURE;
	}
	return SUCCESS;
}
GHashTableIter iter_job;
int job_iter_init(void)
{
	g_hash_table_iter_init (&iter_job,list_job);
	return SUCCESS;
}

job_s * job_iter_next(void)
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
		char STR_ERROR[] = "Несмог открыть базу данных %s : %s";
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
		g_critical("SQL error QUERY_JOB_TABLE : %s\n",error_message);
		sqlite3_free(error_message);
		return FAILURE;
	}

	if(table_job == NOT_OK){
		rc = sqlite3_exec(database,QUERY_CREATE_JOB_TABLE,NULL,NULL, &error_message);
		if( rc != SQLITE_OK){
			g_critical("SQL error QUERY_CREATE_JOB_TABLE : %s",error_message);
			sqlite3_free(error_message);
			return FAILURE;
		}
	}
	g_message("Открыл базу данных");

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
		g_message("Закрыл базу данных");
		database = NULL;
	}

	return SUCCESS;
}


/*****************************************************************************/
/*****************************************************************************/
/*  общие элементы управления                                                */
/*****************************************************************************/
/*****************************************************************************/

job_s * current_job = NULL;
GtkWidget * fra_info = NULL;
GtkWidget * fra_mode_auto = NULL;
GtkWidget * fra_mode_manual = NULL;
GtkWidget * fra_job_load = NULL;
GtkWidget * fra_job_save = NULL;

static char STR_MODE_AUTO[] = "Автоматическое управление";
static char STR_MODE_MANUAL[] = "Ручное Управление";
static char STR_JOB_LOAD[] = "Загрузить работу";
static char STR_JOB_SAVE[] = "Новая работа";


/*****************************************************************************/
/* Основное меню программы                                                   */
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

int not_connect_device(void)
{
	GtkWidget * error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
	                                          ,GTK_BUTTONS_CLOSE,"Нет подключения к устройству!");
	gtk_dialog_run(GTK_DIALOG(error));
	gtk_widget_destroy(error);
	return SUCCESS;
}

int select_frame(int frame)
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
			rc = check_connect_device();
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
			rc = check_connect_device();
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
			rc = check_connect_device();
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

/****************************************************************************/
/*  подменю управление                                                      */
/****************************************************************************/

void activate_menu_auto_mode(GtkMenuItem * im,gpointer d)
{
	select_frame(AUTO_MODE_FRAME);
	g_message("%s",STR_MODE_AUTO);
}

void activate_menu_manual_mode(GtkMenuItem * im,gpointer d)
{
 	select_frame(MANUAL_MODE_FRAME);
	g_message("%s",STR_MODE_MANUAL);
}

char STR_MODE[] = "Режим";
char STR_AUTO_MODE[] = "автоматический";
char STR_MANUAL_MODE[] = "ручной";

GtkWidget * create_menu_mode(void)
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
	                          ,'A',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_mode),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_MANUAL_MODE);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_manual_mode),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'M',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_mode),menite_temp);
	gtk_widget_show(menite_temp);

	gtk_widget_show(menite_mode);
	return menite_mode;
}

/*****************************************************************************/
/* главное меню : подменю работа                                             */
/*****************************************************************************/

void activate_menu_load_job(GtkMenuItem * b,gpointer d)
{
	select_frame(LOAD_JOB_FRAME);
	g_message("%s",STR_JOB_LOAD);
}

void activate_menu_save_job(GtkMenuItem * b,gpointer d)
{
	select_frame(SAVE_JOB_FRAME);
	g_message("%s",STR_JOB_SAVE);
}

static char STR_JOB[] = "Работа";
static char STR_LOAD_JOB[] = "Загрузить";
static char STR_CREATE_JOB[] = "Создать";
static char STR_EXIT[] = "ВЫХОД";

void unrealize_menubar_main(GtkWidget * w,gpointer ud)
{
}

void activate_menu_exit(GtkMenuItem * im,gpointer d)
{
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
	                          ,'L',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_work),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_CREATE_JOB);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_save_job),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'C',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_work),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(men_work),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = gtk_menu_item_new_with_label(STR_EXIT);
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_exit),NULL);
	gtk_widget_add_accelerator(menite_temp,"activate",accgro_main
	                          ,'Q',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(men_work),menite_temp);
	gtk_widget_show(menite_temp);

	menite_temp = create_menu_mode();
	gtk_menu_shell_append(GTK_MENU_SHELL(menbar_main),menite_temp);

	menite_temp = create_menu_video();
	gtk_menu_shell_append(GTK_MENU_SHELL(menbar_main),menite_temp);

	menite_temp = create_menu_device();
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

#define ANGLE_BASE        10
#define ANGLE_FORMAT      "%03d"

int set_active_color(GtkButton * w)
{
	GtkWidget * c = gtk_bin_get_child(GTK_BIN(w));
	gtk_widget_override_background_color(c,GTK_STATE_FLAG_NORMAL,&color_lite_red);
	gtk_widget_override_color(c,GTK_STATE_FLAG_NORMAL,&color_white);
	gtk_widget_override_background_color(GTK_WIDGET(w),GTK_STATE_FLAG_NORMAL,&color_green);
	return SUCCESS;
}

int set_notactive_color(GtkButton * w)
{
	GtkWidget * c = gtk_bin_get_child (GTK_BIN(w));
	gtk_widget_override_background_color(c,GTK_STATE_FLAG_NORMAL,&color_lite_green);
	gtk_widget_override_color(c,GTK_STATE_FLAG_NORMAL,&color_black);
	gtk_widget_override_background_color(GTK_WIDGET(w),GTK_STATE_FLAG_NORMAL,&color_red);
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
/* Панель информации о работе                                                */
/*****************************************************************************/

static char STR_INFO[] = "РАБОТА";
static char STR_NOT_DEVICE[] = "Загрузите информацию о работе";
static char STR_PRESSURE[] =         "Рабочие давление,  атм";
static char STR_PRESSURE_DEFAULT[] = "0";
static char STR_TIME_JOB[] =         "Время работы";
static char STR_TIME_JOB_DEFAULT[] = "00:00:00";
static char STR_UPRISE_ANGEL[] =     "Угол подъема,  градусов";
static char STR_LOWERING_ANGEL[] =   "Угол опускания,  градусов";
static char STR_ANGLE_DEFAULT[] =    "000";

static GtkWidget * lab_info_name_job = NULL;
static GtkWidget * lab_info_pressure = NULL;
static GtkWidget * lab_info_time = NULL;
static GtkWidget * lab_info_uprise = NULL;
static GtkWidget * lab_info_lowering = NULL;


int set_current_value_info(void)
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
		g_string_printf(temp_string,"%d",current_job->pressure);
		gtk_label_set_text(GTK_LABEL(lab_info_pressure),temp_string->str);
		g_string_printf(temp_string,"%02d:%02d:%02d"
		               ,g_date_time_get_hour(current_job->time)
		               ,g_date_time_get_minute(current_job->time)
		               ,g_date_time_get_second(current_job->time));
		gtk_label_set_text(GTK_LABEL(lab_info_time),temp_string->str);
		g_string_printf(temp_string,ANGLE_FORMAT,current_job->uprise);
		gtk_label_set_text(GTK_LABEL(lab_info_uprise),temp_string->str);
		g_string_printf(temp_string,ANGLE_FORMAT,current_job->lowering);
		gtk_label_set_text(GTK_LABEL(lab_info_lowering),temp_string->str);
	}
	return SUCCESS;
}

GtkWidget * create_info(void)
{
	GtkGrid * gri_info;
	GtkWidget * lab_fra_info;
	GtkWidget * lab_pressure;
	GtkWidget * lab_time;
	GtkWidget * lab_uprise;
	GtkWidget * lab_lowering;

	fra_info = gtk_frame_new(NULL);
	gtk_frame_set_label_align(GTK_FRAME(fra_info),0.5,0.5);
	gtk_widget_set_vexpand(fra_info,TRUE);
	gtk_widget_set_hexpand(fra_info,TRUE);

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

#define MAX_TIME_SECOND      360000

struct _label_auto_mode_s
{
	GtkWidget * pressure;
	GtkWidget * time;
	GtkWidget * uprise;
	GtkWidget * lowering;
};
typedef struct _label_auto_mode_s  label_auto_mode_s;

label_auto_mode_s label_auto_mode;

static char STR_BUTTON_AUTO_START[] = "СТАРТ";
static char STR_BUTTON_AUTO_STOP[]  = "СТОП";
static char STR_BUTTON_AUTO_PAUSE[] = "ПАУЗА";
static char STR_SET_VALUE[] = "Установленое значение";
static char STR_CURRENT_VALUE[] = "Текущее значение";
static char STR_SPEED_VERTICAL[] = "Скорость вертикальная, град/сек";

int amount_auto_mode = 0;
int auto_mode_pause = NOT_OK;

int check_registers_auto_mode(gpointer ud)
{
	int rc;
	uint16_t angle;
	uint16_t pressure;
	uint16_t sensors;
	uint16_t input;
	uint16_t console;
	int hour;
	int minut;
	int second;

	if(auto_mode_start != OK){
		return FALSE;
	}
	if(auto_mode_pause == OK){
		return FALSE;
	}
	rc = command_angle(&angle);
	if(rc != SUCCESS){
		return FALSE;
	}
	rc = command_pressure(&pressure);
	if(rc != SUCCESS){
		return FALSE;
	}
	rc = command_sensors(&sensors);
	if(rc != SUCCESS){
		return FALSE;
	}
	rc = command_input(&input);
	if(rc != SUCCESS){
		return FALSE;
	}
	rc = command_console(&console);
	if(rc != SUCCESS){
		return FALSE;
	}

	amount_auto_mode ++;
	if(amount_auto_mode == MAX_TIME_SECOND){
		amount_auto_mode = 0;
	}

	hour = amount_auto_mode / (60*60);
	minut = amount_auto_mode / (60 - (hour * 60));
	second = amount_auto_mode - (minut * 60);

	g_string_printf(temp_string,"%d",pressure);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.pressure),temp_string->str);

	g_string_printf(temp_string,"%02d:%02d:%02d",hour,minut,second);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.time),temp_string->str);

	g_string_printf(temp_string,ANGLE_FORMAT,angle);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.uprise),temp_string->str);
	gtk_label_set_text(GTK_LABEL(label_auto_mode.lowering),temp_string->str);

	g_printf("%02d:%02d:%02d\n",hour,minut,second);
	g_printf("angle    :> %#x\n",angle);
	g_printf("pressure :> %#x\n",pressure);
	g_printf("sensors  :> %#x\n",sensors);
	g_printf("input    :> %#x\n",input);
	g_printf("console  :> %#x\n",console);

	{
		int second = g_date_time_get_second(current_job->time);
		second += (g_date_time_get_minute(current_job->time)*60);
		second += (g_date_time_get_hour(current_job->time)*(60*60));

		if(amount_auto_mode == second){
			auto_mode_start = NOT_OK;
			command_auto_stop();
			set_notactive_color(GTK_BUTTON(ud));
			return FALSE;
		}
	}

	return TRUE;
}

int timeout_auto_mode = 1000; /* раз в секунду*/

void clicked_button_auto_start(GtkButton * b,gpointer d)
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

GtkWidget * but_auto_mode_pause;

void clicked_button_auto_stop(GtkButton * b,gpointer d)
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

void clicked_button_auto_pause(GtkButton * b,gpointer d)
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

uint16_t speed_vertical_auto_mode = 4000;

double SPEED_VERTICAL_MIN = 0;
double SPEED_VERTICAL_MAX = 6;
double SPEED_VERTICAL_STEP = 0.75;
double SPEED_VERTICAL_RATE = 666.66667;

GtkWidget * lab_auto_mode_name_job;
GtkWidget * lab_auto_mode_pressure;
GtkWidget * lab_auto_mode_time;
GtkWidget * lab_auto_mode_uprise;
GtkWidget * lab_auto_mode_lowering;

int set_current_value_auto_mode(void)
{
	gtk_label_set_text(GTK_LABEL(label_auto_mode.pressure),"0");
	gtk_label_set_text(GTK_LABEL(label_auto_mode.time),"00:00:00");
	gtk_label_set_text(GTK_LABEL(label_auto_mode.uprise),"00");
	gtk_label_set_text(GTK_LABEL(label_auto_mode.lowering),"00");
	return SUCCESS;
}

int set_preset_value_auto_mode(void)
{
	if(current_job == NULL){
		g_critical("не выбрана работа!");
		return FAILURE;
	}
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_name_job),current_job->name->str);
	g_string_printf(temp_string,"%d",current_job->pressure);
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_pressure),temp_string->str);
	g_string_printf(temp_string,"%02d:%02d:%02d"
	               ,g_date_time_get_hour(current_job->time)
	               ,g_date_time_get_minute(current_job->time)
	               ,g_date_time_get_second(current_job->time));
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_time),temp_string->str);
	g_string_printf(temp_string,ANGLE_FORMAT,current_job->uprise);
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_uprise),temp_string->str);
	g_string_printf(temp_string,ANGLE_FORMAT,current_job->lowering);
	gtk_label_set_text(GTK_LABEL(lab_auto_mode_lowering),temp_string->str);
	return SUCCESS;
}

GtkWidget * create_label_mode_auto(void)
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
	gtk_widget_set_hexpand(fra_auto,TRUE);
	gtk_widget_set_vexpand(fra_auto,TRUE);
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
	set_size_font(lab_auto_mode_name_job,SIZE_FONT_SMALL);


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

void show_frame_auto_mode(GtkWidget * w,gpointer ud)
{
	/*TODO проверка на связь*/
	command_auto_mode();
	command_uprise_angle(current_job->uprise);
	command_lowering_angle(current_job->lowering);
	command_speed_vertical(speed_vertical_auto_mode);
	auto_mode_start = NOT_OK;
	auto_mode_pause = NOT_OK;
	set_preset_value_auto_mode();
	set_current_value_auto_mode();
}

void hide_frame_auto_mode(GtkWidget * w,gpointer ud)
{
	command_null_mode();
}

GtkWidget * create_mode_auto(void)
{
	GtkWidget * lab_fra_mode_auto;
	GtkWidget * box_hor_main;
	GtkWidget * box_hor_but;
	GtkWidget * but_start;
	GtkWidget * but_stop;
	GtkWidget * gri_auto;

	fra_mode_auto = gtk_frame_new(NULL);
	gtk_widget_set_hexpand(fra_mode_auto,TRUE);
	gtk_widget_set_vexpand(fra_mode_auto,TRUE);
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
	gtk_widget_set_halign(but_start,GTK_ALIGN_FILL);
	gtk_widget_set_valign(but_start,GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(but_start,TRUE);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_start)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_start));
	g_signal_connect(but_start,"clicked",G_CALLBACK(clicked_button_auto_start),NULL);

	but_stop = gtk_button_new_with_label(STR_BUTTON_AUTO_STOP);
	gtk_widget_set_halign(but_stop,GTK_ALIGN_FILL);
	gtk_widget_set_valign(but_stop,GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(but_stop,TRUE);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_stop)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_stop));
	g_signal_connect(but_stop,"clicked",G_CALLBACK(clicked_button_auto_stop),but_start);

	but_auto_mode_pause = gtk_button_new_with_label(STR_BUTTON_AUTO_PAUSE);
	gtk_widget_set_halign(but_auto_mode_pause,GTK_ALIGN_FILL);
	gtk_widget_set_valign(but_auto_mode_pause,GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(but_auto_mode_pause,TRUE);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_auto_mode_pause)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_auto_mode_pause));
	g_signal_connect(but_auto_mode_pause,"clicked",G_CALLBACK(clicked_button_auto_pause),but_start);

	gri_auto = create_label_mode_auto();

	gtk_frame_set_label_widget(GTK_FRAME(fra_mode_auto),lab_fra_mode_auto);
	gtk_container_add(GTK_CONTAINER(fra_mode_auto),box_hor_main);
	gtk_box_pack_start(GTK_BOX(box_hor_main),box_hor_but,TRUE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_hor_but),but_start,TRUE,TRUE,10);
	gtk_box_pack_start(GTK_BOX(box_hor_but),but_stop,TRUE,TRUE,10);
	gtk_box_pack_start(GTK_BOX(box_hor_but),but_auto_mode_pause,TRUE,TRUE,10);
	gtk_box_pack_start(GTK_BOX(box_hor_main),gri_auto,TRUE,TRUE,5);

	gtk_widget_show(but_auto_mode_pause);
	gtk_widget_show(but_stop);
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
static char STR_BUTTON_MANUAL_OPEN[] =  "Включить Насос";
static char STR_BUTTON_MANUAL_CLOSE[] = "Выключить Насос";
static char STR_ANGLE[] = "Угол, градусов";

void press_button_manual_up(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_active_color(b);
	command_manual_up();
}
void release_button_manual_up(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_notactive_color(b);
	command_manual_null();
}
void press_button_manual_down(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_active_color(b);
	command_manual_down();
}
void release_button_manual_down(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_notactive_color(b);
	command_manual_null();
}
void press_button_manual_left(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_active_color(b);
	command_manual_left();
}

void release_button_manual_left(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_notactive_color(b);
	command_manual_null();
}
void press_button_manual_right(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_active_color(b);
	command_manual_right();
}
void release_button_manual_right(GtkButton * b,GdkEvent * e,gpointer ud)
{
	set_notactive_color(b);
	command_manual_null();
}

int manual_pump_open = NOT_OK;

void clicked_button_manual_pump(GtkButton * b,gpointer d)
{
	if(manual_pump_open == OK){
		manual_pump_open = NOT_OK;
		command_manual_off_drive();
		gtk_button_set_label(b,STR_BUTTON_MANUAL_OPEN);
		/*set_size_font(gtk_bin_get_child(GTK_BIN(b)),SIZE_FONT_SMALL);*/
		set_notactive_color(b);
	}
	else{
		manual_pump_open = OK;
		command_manual_on_drive();
		gtk_button_set_label(b,STR_BUTTON_MANUAL_CLOSE);
		/*set_size_font(gtk_bin_get_child(GTK_BIN(b)),SIZE_FONT_SMALL);*/
		set_active_color(b);
	}
}

gboolean change_value_scale_speed(GtkRange * r,GtkScrollType s,gdouble v,gpointer ud)
{
	double speed_d;
	uint16_t speed_ui;

	speed_d = v / SPEED_VERTICAL_STEP;
	speed_d = (int)speed_d;
	speed_d *= SPEED_VERTICAL_STEP;

	gtk_range_set_value(r,speed_d);

	speed_ui = (uint16_t)(speed_d * SPEED_VERTICAL_RATE);
	command_speed_vertical(speed_ui);
	return TRUE;
}

struct _label_manual_mode_s
{
	GtkWidget * pressure;
	GtkWidget * time;
	GtkWidget * angle;
};

typedef struct _label_manual_mode_s label_manual_mode_s;

label_manual_mode_s label_manual_mode;

int amount_manual_mode = 0;

int check_registers_manual_mode(gpointer ud)
{
	int rc;
	uint16_t angle;
	uint16_t pressure;
	uint16_t sensors;
	uint16_t input;
	uint16_t console;
	int hour;
	int minut;
	int second;

	if(manual_mode_start != OK){
		return FALSE;
	}
	rc = command_angle(&angle);
	if(rc != SUCCESS){
		return FALSE;
	}
	rc = command_pressure(&pressure);
	if(rc != SUCCESS){
		return FALSE;
	}
	rc = command_sensors(&sensors);
	if(rc != SUCCESS){
		return FALSE;
	}
	rc = command_input(&input);
	if(rc != SUCCESS){
		return FALSE;
	}
	rc = command_console(&console);
	if(rc != SUCCESS){
		return FALSE;
	}

	amount_manual_mode ++;
	if(amount_manual_mode == MAX_TIME_SECOND){
		amount_manual_mode = 0;
	}
	hour = amount_manual_mode / (60*60);
	minut = amount_manual_mode / (60 - (hour * 60));
	second = amount_manual_mode - (minut * 60);

	g_string_printf(temp_string,"%d",pressure);
	gtk_label_set_text(GTK_LABEL(label_manual_mode.pressure),temp_string->str);

	g_string_printf(temp_string,"%02d:%02d:%02d",hour,minut,second);
	gtk_label_set_text(GTK_LABEL(label_manual_mode.time),temp_string->str);

	g_string_printf(temp_string,ANGLE_FORMAT,angle);
	gtk_label_set_text(GTK_LABEL(label_manual_mode.angle),temp_string->str);

	g_printf("\n");
	g_printf("%02d:%02d:%02d\n",hour,minut,second);
	g_printf("angle    :> %#x\n",angle);
	g_printf("pressure :> %#x\n",pressure);
	g_printf("sensors  :> %#x\n",sensors);
	g_printf("input    :> %#x\n",input);
	g_printf("console  :> %#x\n",console);
	return TRUE;
}

int timeout_manual_mode = 1000;

void show_frame_manual_mode(GtkWidget * w,gpointer ud)
{
	int rc;
	rc = command_manual_mode();
	if(rc == SUCCESS){
		command_speed_vertical(0);
		amount_manual_mode = 0;
		manual_mode_start = OK;
		g_timeout_add(timeout_manual_mode,check_registers_manual_mode,NULL);
	}
}

void hide_frame_manual_mode(GtkWidget * w,gpointer ud)
{
	manual_mode_start = NOT_OK;
	command_null_mode();
}

GtkWidget * create_label_mode_manual(void)
{
	GtkWidget * fra_label;
	GtkWidget * gri_label;
	GtkWidget * lab_pressure;
	GtkWidget * lab_time;
	GtkWidget * lab_angle;

	fra_label = gtk_frame_new(NULL);
	gtk_widget_set_hexpand(fra_label,TRUE);
	gtk_widget_set_vexpand(fra_label,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(fra_label),5);

	gri_label = gtk_grid_new();
	gtk_widget_set_hexpand(gri_label,TRUE);
	gtk_widget_set_vexpand(gri_label,TRUE);

	lab_pressure = gtk_label_new(STR_PRESSURE);
	gtk_widget_set_halign(lab_pressure,GTK_ALIGN_START);
	gtk_widget_set_vexpand(lab_pressure,TRUE);

	lab_time = gtk_label_new(STR_TIME_JOB);
	gtk_widget_set_halign(lab_time,GTK_ALIGN_START);
	gtk_widget_set_vexpand(lab_time,TRUE);

	lab_angle = gtk_label_new(STR_ANGLE);
	gtk_widget_set_halign(lab_angle,GTK_ALIGN_START);
	gtk_widget_set_vexpand(lab_angle,TRUE);

	label_manual_mode.pressure = gtk_label_new(STR_PRESSURE_DEFAULT);
	gtk_widget_set_halign(label_manual_mode.pressure,GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(label_manual_mode.pressure,TRUE);

	label_manual_mode.time = gtk_label_new(STR_TIME_JOB_DEFAULT);
	gtk_widget_set_halign(label_manual_mode.time,GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(label_manual_mode.time,TRUE);

	label_manual_mode.angle = gtk_label_new(STR_ANGLE_DEFAULT);
	gtk_widget_set_halign(label_manual_mode.angle,GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(label_manual_mode.angle,TRUE);

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

GtkWidget * create_mode_manual(void)
{
	GtkWidget * lab_fra_mode_manual;
	GtkWidget * box_horizontal;
	GtkWidget * gri_mode;
	GtkWidget * but_up;
	GtkWidget * but_down;
	GtkWidget * but_left;
	GtkWidget * but_right;
	GtkWidget * but_pump;
	GtkWidget * box_speed;
	GtkWidget * fra_speed;
	GtkWidget * lab_speed;
	GtkWidget * sca_speed;
	GtkWidget * gri_label;

	fra_mode_manual = gtk_frame_new(NULL);
	gtk_frame_set_label_align(GTK_FRAME(fra_mode_manual),0.5,0.5);
	gtk_widget_set_hexpand(fra_mode_manual,TRUE);
	gtk_widget_set_vexpand(fra_mode_manual,TRUE);
	g_signal_connect(fra_mode_manual,"show",G_CALLBACK(show_frame_manual_mode),NULL);
	g_signal_connect(fra_mode_manual,"hide",G_CALLBACK(hide_frame_manual_mode),NULL);

	lab_fra_mode_manual = gtk_label_new(STR_MODE_MANUAL);
	set_size_font(lab_fra_mode_manual,SIZE_FONT_MEDIUM);

	box_horizontal = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);

	gri_mode = gtk_grid_new();
	gtk_widget_set_hexpand(gri_mode,TRUE);
	gtk_widget_set_vexpand(gri_mode,TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(gri_mode),5);
	gtk_grid_set_row_homogeneous(GTK_GRID(gri_mode),TRUE);
	gtk_grid_set_column_spacing(GTK_GRID(gri_mode),5);
	gtk_grid_set_column_homogeneous(GTK_GRID(gri_mode),TRUE);

	but_up = gtk_button_new_with_label(STR_BUTTON_MANUAL_UP);
	/*set_size_font(gtk_bin_get_child(GTK_BIN(but_up)),SIZE_FONT_SMALL);*/
	set_notactive_color(GTK_BUTTON(but_up));
	g_signal_connect(but_up,"button-press-event",G_CALLBACK(press_button_manual_up),NULL);
	g_signal_connect(but_up,"button-release-event",G_CALLBACK(release_button_manual_up),NULL);

	but_down = gtk_button_new_with_label(STR_BUTTON_MANUAL_DOWN);
	/*set_size_font(gtk_bin_get_child(GTK_BIN(but_down)),SIZE_FONT_SMALL);*/
	set_notactive_color(GTK_BUTTON(but_down));
	g_signal_connect(but_down,"button-press-event",G_CALLBACK(press_button_manual_down),NULL);
	g_signal_connect(but_down,"button-release-event",G_CALLBACK(release_button_manual_down),NULL);

	but_left = gtk_button_new_with_label(STR_BUTTON_MANUAL_LEFT);
	/*set_size_font(gtk_bin_get_child(GTK_BIN(but_left)),SIZE_FONT_SMALL);*/
	set_notactive_color(GTK_BUTTON(but_left));
	g_signal_connect(but_left,"button-press-event",G_CALLBACK(press_button_manual_left),NULL);
	g_signal_connect(but_left,"button-release-event",G_CALLBACK(release_button_manual_left),NULL);

	but_right = gtk_button_new_with_label(STR_BUTTON_MANUAL_RIGHT);
	/*set_size_font(gtk_bin_get_child(GTK_BIN(but_right)),SIZE_FONT_SMALL);*/
	set_notactive_color(GTK_BUTTON(but_right));
	g_signal_connect(but_right,"button-press-event",G_CALLBACK(press_button_manual_right),NULL);
	g_signal_connect(but_right,"button-release-event",G_CALLBACK(release_button_manual_right),NULL);

	but_pump = gtk_button_new_with_label(STR_BUTTON_MANUAL_OPEN);
	/*set_size_font(gtk_bin_get_child(GTK_BIN(but_pump)),SIZE_FONT_SMALL);*/
	set_notactive_color(GTK_BUTTON(but_pump));
	gtk_widget_set_size_request(but_pump,150,-1);
	g_signal_connect(but_pump,"clicked",G_CALLBACK(clicked_button_manual_pump),NULL);

	fra_speed = gtk_frame_new(NULL);

	box_speed = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
	gtk_container_set_border_width(GTK_CONTAINER(box_speed),5);

	lab_speed = gtk_label_new(STR_SPEED_VERTICAL);
 	sca_speed = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL
	                                    ,SPEED_VERTICAL_MIN,SPEED_VERTICAL_MAX,SPEED_VERTICAL_STEP);
	gtk_scale_set_digits(GTK_SCALE(sca_speed),2); /*колличество знаков после запятой*/
	gtk_scale_set_value_pos(GTK_SCALE(sca_speed),GTK_POS_TOP);
	g_signal_connect(sca_speed,"change-value",G_CALLBACK(change_value_scale_speed),NULL);

	gri_label = create_label_mode_manual();

	gtk_frame_set_label_widget(GTK_FRAME(fra_mode_manual),lab_fra_mode_manual);
	gtk_container_add(GTK_CONTAINER(fra_mode_manual),box_horizontal);

	gtk_box_pack_start(GTK_BOX(box_horizontal),gri_mode,TRUE,TRUE,5);
	gtk_grid_attach(GTK_GRID(gri_mode),but_up   ,1,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_down ,1,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_left ,0,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_right,2,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_pump ,3,1,1,1);

	gtk_grid_attach(GTK_GRID(gri_mode),fra_speed,2,2,2,1);
	gtk_container_add(GTK_CONTAINER(fra_speed),box_speed);
	gtk_box_pack_start(GTK_BOX(box_speed),lab_speed,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(box_speed),sca_speed,TRUE,TRUE,0);

	gtk_box_pack_start(GTK_BOX(box_horizontal),gri_label,TRUE,TRUE,5);

	gtk_widget_show(sca_speed);
	gtk_widget_show(lab_speed);
	gtk_widget_show(box_speed);
	gtk_widget_show(fra_speed);
	gtk_widget_show(but_pump);
	gtk_widget_show(but_right);
	gtk_widget_show(but_left);
	gtk_widget_show(but_down);
	gtk_widget_show(but_up);
	gtk_widget_show(gri_mode);
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
/*
	COLUMN_PRESSURE,
	COLUMN_TIME,
	COLUMMN_UPRISE,
	COLUMMN_LOWERING,
*/
	NUM_COLUMN
};

char * select_row_activated(GtkTreeView * tre_job)
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

int select_current_job(GtkTreeView * tre_job)
{
	gpointer pt = NULL;
	char * name;

	name = select_row_activated(tre_job);
	if(name == NULL){
		g_critical("Cписок выбора не корректный (0)");
		return FAILURE;
	}

	g_string_truncate(temp_job.name,0);
	g_string_append(temp_job.name,name);

	g_hash_table_lookup_extended(list_job,&temp_job,&pt,NULL);
	if(pt == NULL){
		g_critical("Таблица хешей не корректна (0)");
		return FAILURE;
	}
	current_job = (job_s *)pt;
	set_current_value_info();

	if(first_auto_mode == OK){
		first_auto_mode = NOT_OK;
		select_frame(AUTO_MODE_FRAME);
	}
	else{
		select_frame(INFO_FRAME);
	}
	return SUCCESS;
}
void clicked_button_load_job(GtkButton * but,GtkTreeView * tre_job)
{
	select_current_job(tre_job);
}

void clicked_button_del_job(GtkButton * but,GtkTreeView * tre_job)
{
	char * name;
	int rc;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreeSelection * select;

	name = select_row_activated(tre_job);
	if(name == NULL){
		g_critical("Cписок выбора не корректный (1)");
		return ;
	}

	g_string_truncate(temp_job.name,0);
	g_string_append(temp_job.name,name);

	rc = g_hash_table_remove(list_job,&temp_job);
	if(rc != TRUE){
		g_critical("Таблица хешей не корректна (1)");
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
	select_frame(INFO_FRAME);
	return ;
}

int sort_model_list_job(GtkTreeModel *model
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

void row_activated_tree_job(GtkTreeView *tree_view
                           ,GtkTreePath *path
                           ,GtkTreeViewColumn *column
                           ,gpointer           user_data)
{
	select_current_job(tree_view);
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

static char STR_DEL_JOB[] = "Удалить";

GtkWidget * tree_view_job;

GtkWidget * create_job_load(void)
{
	GtkWidget * lab_fra_job_load;
 	GtkWidget * box_vertical;
	GtkWidget * scrwin_job;
	GtkTreeModel * tremod_job;
	GtkWidget * box_horizontal;
	GtkWidget * but_load;
	GtkWidget * but_del;

	fra_job_load = gtk_frame_new(NULL);
	gtk_frame_set_label_align(GTK_FRAME(fra_job_load),0.5,0.5);
	gtk_widget_set_hexpand(fra_job_load,TRUE);
	gtk_widget_set_vexpand(fra_job_load,TRUE);

	lab_fra_job_load = gtk_label_new(STR_JOB_LOAD);
	set_size_font(lab_fra_job_load,SIZE_FONT_MEDIUM);

	box_vertical = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);

	scrwin_job  = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrwin_job),GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrwin_job),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

	tremod_job = create_model_list_job();

	tree_view_job = gtk_tree_view_new_with_model(tremod_job);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view_job), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view_job))
	                           ,GTK_SELECTION_SINGLE);
	/*gtk_widget_set_halign(tre_job,GTK_ALIGN_FILL);*/
	/*gtk_widget_set_valign(tre_job,GTK_ALIGN_FILL);*/
	gtk_widget_set_hexpand(tree_view_job,TRUE);
	gtk_widget_set_vexpand (tree_view_job,TRUE);
	g_signal_connect(tree_view_job,"row-activated",G_CALLBACK(row_activated_tree_job),NULL);

	add_column_tree_job(GTK_TREE_VIEW(tree_view_job));

	box_horizontal = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
	/*gtk_box_set_homogeneous(GTK_BOX(box_horizontal),TRUE);*/
	but_load = gtk_button_new_with_label(STR_LOAD_JOB);
	gtk_widget_set_halign(but_load,GTK_ALIGN_CENTER);
	g_signal_connect(but_load,"clicked",G_CALLBACK(clicked_button_load_job),tree_view_job);
	but_del = gtk_button_new_with_label(STR_DEL_JOB);
	gtk_widget_set_halign(but_load,GTK_ALIGN_CENTER);
	g_signal_connect(but_del,"clicked",G_CALLBACK(clicked_button_del_job),tree_view_job);

	gtk_frame_set_label_widget(GTK_FRAME(fra_job_load),lab_fra_job_load);
	gtk_container_add(GTK_CONTAINER(fra_job_load),box_vertical);
	gtk_box_pack_start(GTK_BOX(box_vertical),scrwin_job,FALSE,TRUE,5);
	gtk_container_add(GTK_CONTAINER(scrwin_job),tree_view_job);
	gtk_box_pack_start(GTK_BOX(box_vertical),box_horizontal,FALSE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_horizontal),but_load,FALSE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(box_horizontal),but_del,FALSE,TRUE,5);

	gtk_widget_show(but_del);
	gtk_widget_show(but_load);
	gtk_widget_show(box_horizontal);
	gtk_widget_show(tree_view_job);
	gtk_widget_show(scrwin_job);
	gtk_widget_show(box_vertical);
	gtk_widget_show(lab_fra_job_load);
	gtk_widget_hide(fra_job_load);
	return fra_job_load;
}


/*****************************************************************************/
/* Панель создать работу                                                     */
/*****************************************************************************/

static char STR_NAME_JOB[] = "Наименование работ";
static char STR_NAME_JOB_DEFAULT[] = "Изделие 000";
static char STR_DEFAULT_PRESSURE[] = "2";
static char STR_FIX_ANGLE[] = "Зафиксировать угол";
static char STR_SAVE_JOB[] = "Сохранить работу";

static GtkEntryBuffer * entbuff_name_job;
static GtkEntryBuffer * entbuff_pressure;
static GtkEntryBuffer * entbuff_time;
static GtkEntryBuffer * entbuff_uprise;
static GtkEntryBuffer * entbuff_lowering;

int check_entry_pressure(const char * pressure)
{
	char * c;
	c = g_strstr_len(pressure,1,STR_DEFAULT_PRESSURE);
	if(c == NULL){
		return FAILURE;
	}
	return SUCCESS;
}
/*TODO обединеть с проверкой в базе данных*/
int check_entry_time(const char * str)
{
	gint hour = 0;
	gint minut = 0;
	gint second = 0;
	gint amount = 0;

	/*str 00:00:00*/
	if((str[2] != ':') || (str[5] != ':')){
		return FAILURE;
	}

	hour = str[0] - '0';
	hour *= 10;
	hour += (str[1] - '0');
	if( (hour < 0) || (hour > 24) ){
		return FAILURE;
	}

	minut = str[3] - '0';
	minut *= 10;
	minut += (str[4] - '0');
	if((minut < 0)||(minut>59)){
		return FAILURE;
	}
	second = str[6] - '0';
	second *= 10;
	second += (str[7] - '0');
	if((second < 0) || (second > 59)){
		return FAILURE;
	}
	amount += ((hour*(60*60)) + (minut*60) + second);
	if(amount == 0){
		return FAILURE;
	}
	return SUCCESS;
}

#define ANGLE_NULL        -1

static long int value_uprise = ANGLE_NULL;
static long int value_lowering = ANGLE_NULL;

int check_entry_angle(const char * uprise,const char * lowering)
{
	value_uprise = g_ascii_strtoll(uprise,NULL,ANGLE_BASE);
	value_lowering = g_ascii_strtoll(lowering,NULL,ANGLE_BASE);

	if(value_uprise <= value_lowering){
		return FAILURE;
	}
	return SUCCESS;
}

void clicked_button_fix_uprise(GtkButton * b,gpointer d)
{
	int rc;
	uint16_t angle;

	rc = command_angle(&angle);
	if(rc != SUCCESS){
		return ;
	}
	g_string_printf(temp_string,ANGLE_FORMAT,angle);
	gtk_entry_buffer_set_text(entbuff_uprise,temp_string->str,-1);
}

void clicked_button_fix_lowering(GtkButton * b,gpointer d)
{
	int rc;
	uint16_t angle;

	rc = command_angle(&angle);
	if(rc != SUCCESS){
		return ;
	}
	g_string_printf(temp_string,ANGLE_FORMAT,angle);
	gtk_entry_buffer_set_text(entbuff_lowering,temp_string->str,-1);
}

void clicked_button_save_job(GtkButton * b,gpointer d)
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

	rc = check_entry_time(time);
	if(rc != SUCCESS){
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

	g_hash_table_lookup_extended(list_job,&temp_job,&pt,NULL);
	if(pt == NULL){
		g_critical("Таблица хешей не корректна (0)");
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

GtkWidget * create_entry_job_save(void)
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
	gtk_widget_set_hexpand(gri_entry,TRUE);
	gtk_widget_set_vexpand(gri_entry,TRUE);
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

	entbuff_pressure = gtk_entry_buffer_new(STR_DEFAULT_PRESSURE,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(entbuff_pressure),1);
	ent_pressure = gtk_entry_new_with_buffer(entbuff_pressure);
	gtk_entry_set_width_chars(GTK_ENTRY(ent_pressure),1);
	gtk_widget_set_hexpand(ent_pressure,FALSE);
	gtk_widget_set_vexpand(ent_pressure,TRUE);
	gtk_widget_set_halign(ent_pressure,GTK_ALIGN_START);
	gtk_widget_set_valign(ent_pressure,GTK_ALIGN_START);


	entbuff_time = gtk_entry_buffer_new(STR_TIME_JOB_DEFAULT,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(entbuff_time),8);
	ent_time = gtk_entry_new_with_buffer(entbuff_time);
	gtk_entry_set_width_chars(GTK_ENTRY(ent_time),8);
	gtk_widget_set_hexpand(ent_time,FALSE);
	gtk_widget_set_vexpand(ent_time,TRUE);
	gtk_widget_set_halign(ent_time,GTK_ALIGN_START);
	gtk_widget_set_valign(ent_time,GTK_ALIGN_START);

	entbuff_uprise = gtk_entry_buffer_new(STR_ANGLE_DEFAULT,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(entbuff_uprise),3);
	ent_uprise = gtk_entry_new_with_buffer(entbuff_uprise);
	gtk_entry_set_width_chars(GTK_ENTRY(ent_uprise),3);
	gtk_widget_set_hexpand(ent_uprise,FALSE);
	gtk_widget_set_vexpand(ent_uprise,TRUE);
	gtk_widget_set_halign(ent_uprise,GTK_ALIGN_START);
	gtk_widget_set_valign(ent_uprise,GTK_ALIGN_START);

	entbuff_lowering = gtk_entry_buffer_new(STR_ANGLE_DEFAULT,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(entbuff_lowering),3);
	ent_lowering = gtk_entry_new_with_buffer(entbuff_lowering);
	gtk_entry_set_width_chars(GTK_ENTRY(ent_lowering),3);
	gtk_widget_set_hexpand(ent_lowering,FALSE);
	gtk_widget_set_vexpand(ent_lowering,TRUE);
	gtk_widget_set_halign(ent_lowering,GTK_ALIGN_START);
	gtk_widget_set_valign(ent_lowering,GTK_ALIGN_START);

	but_fix_uprise = gtk_button_new_with_label(STR_FIX_ANGLE);
	gtk_widget_set_hexpand(but_fix_uprise,FALSE);
	gtk_widget_set_vexpand(but_fix_uprise,TRUE);
	gtk_widget_set_halign(but_fix_uprise,GTK_ALIGN_START);
	gtk_widget_set_valign(but_fix_uprise,GTK_ALIGN_START);
	g_signal_connect(but_fix_uprise,"clicked",G_CALLBACK(clicked_button_fix_uprise),NULL);

	but_fix_lowering = gtk_button_new_with_label(STR_FIX_ANGLE);
	gtk_widget_set_hexpand(but_fix_lowering,FALSE);
	gtk_widget_set_vexpand(but_fix_lowering,TRUE);
	gtk_widget_set_halign(but_fix_lowering,GTK_ALIGN_START);
	gtk_widget_set_valign(but_fix_lowering,GTK_ALIGN_START);
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

GtkWidget * lab_angle_save_job;

uint16_t speed_vertical_config_mode = 2000;

GtkWidget * create_button_job_save(void)
{
	GtkWidget * fra_button;
	GtkWidget * gri_button;
	GtkWidget * but_up;
	GtkWidget * but_down;
	GtkWidget * lab_angle;

	fra_button = gtk_frame_new(NULL);
	gtk_widget_set_hexpand(fra_button,TRUE);
	gtk_widget_set_vexpand(fra_button,TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(fra_button),10);

	gri_button = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(gri_button),10);
	gtk_widget_set_hexpand(gri_button,TRUE);
	gtk_widget_set_vexpand(gri_button,TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(gri_button),10);
	gtk_grid_set_column_spacing(GTK_GRID(gri_button),10);

	but_up = gtk_button_new_with_label(STR_BUTTON_MANUAL_UP);
	gtk_widget_set_hexpand(but_up,TRUE);
	gtk_widget_set_vexpand(but_up,TRUE);
	gtk_widget_set_halign(but_up,GTK_ALIGN_CENTER);
	gtk_widget_set_valign(but_up,GTK_ALIGN_CENTER);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_up)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_up));
	g_signal_connect(but_up,"button-press-event",G_CALLBACK(press_button_manual_up),NULL);
	g_signal_connect(but_up,"button-release-event",G_CALLBACK(release_button_manual_up),NULL);

	but_down = gtk_button_new_with_label(STR_BUTTON_MANUAL_DOWN);
	gtk_widget_set_hexpand(but_down,TRUE);
	gtk_widget_set_vexpand(but_down,TRUE);
	gtk_widget_set_halign(but_down,GTK_ALIGN_CENTER);
	gtk_widget_set_valign(but_down,GTK_ALIGN_CENTER);
	set_size_font(gtk_bin_get_child(GTK_BIN(but_down)),SIZE_FONT_MEDIUM);
	set_notactive_color(GTK_BUTTON(but_down));
	g_signal_connect(but_down,"button-press-event",G_CALLBACK(press_button_manual_down),NULL);
	g_signal_connect(but_down,"button-release-event",G_CALLBACK(release_button_manual_down),NULL);

	lab_angle = gtk_label_new(STR_ANGLE);
	set_size_font(lab_angle,SIZE_FONT_SMALL);

	lab_angle_save_job = gtk_label_new(STR_ANGLE_DEFAULT);
	set_size_font(lab_angle_save_job,SIZE_FONT_SMALL);

	gtk_container_add(GTK_CONTAINER(fra_button),gri_button);
	gtk_grid_attach(GTK_GRID(gri_button),but_up            ,1,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_button),but_down          ,1,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_button),lab_angle         ,0,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_button),lab_angle_save_job,1,2,1,1);

	gtk_widget_show(lab_angle_save_job);
	gtk_widget_show(lab_angle);
	gtk_widget_show(but_down);
	gtk_widget_show(but_up);
	gtk_widget_show(gri_button);
	gtk_widget_show(fra_button);
	return fra_button;
}

int config_mode = NOT_OK;

int check_registers_config_mode(gpointer ud)
{
	int rc;
	uint16_t angle;

	if(config_mode != OK){
		return FALSE;
	}

	rc = command_angle(&angle);
	if(rc != SUCCESS){
		return FALSE;
	}

	g_string_printf(temp_string,ANGLE_FORMAT,angle);
	gtk_label_set_text(GTK_LABEL(lab_angle_save_job),temp_string->str);
	return TRUE;
}

int timeout_config_mode = 1000;

void show_frame_save_job(GtkWidget * w,gpointer ud)
{
	value_uprise = ANGLE_NULL;
	value_lowering = ANGLE_NULL;
	config_mode = OK;
	command_config_mode();
	command_speed_vertical(speed_vertical_config_mode);
	g_timeout_add(timeout_config_mode,check_registers_config_mode,NULL);
}

void hide_frame_save_job(GtkWidget * w,gpointer ud)
{
	config_mode = NOT_OK;
	command_null_mode();
}

GtkWidget * create_job_save(void)
{
	GtkWidget * lab_fra_job_save;
	GtkWidget * box_horizontal;
	GtkWidget * temp;


	fra_job_save = gtk_frame_new(NULL);
	gtk_widget_set_hexpand(fra_job_save,TRUE);
	gtk_widget_set_vexpand(fra_job_save,TRUE);
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

GtkWidget * create_control_panel(void)
{
	GtkWidget * fra_control;
	GtkWidget * gri_control;

	fra_control = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(fra_control),5);

	gri_control = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(gri_control),10);
	gtk_widget_set_halign(gri_control,GTK_ALIGN_FILL);
	gtk_widget_set_valign(gri_control,GTK_ALIGN_FILL);
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
/*****************************************************************************/
