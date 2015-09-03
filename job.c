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
#include <sqlite3.h>

#include <modbus.h>
#include "total.h"
#include "cid.h"
#include "video.h"
#include "control.h"

/*****************************************************************************/
/*  работа с базой данных                                                    */
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
	GDateTime * time;
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

static int fill_list_job(void)
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
				str = g_strrstr_len(NAME_COLUMN_0,SIZE_NAME_COLUMN_0,str);
				if(str == NULL){
					g_critical("SQL error not correct column 0 ");
					break;
				}
				str = sqlite3_column_name(query,1);
				str = g_strrstr_len(NAME_COLUMN_1,SIZE_NAME_COLUMN_1,str);
				if(str == NULL){
					g_critical("SQL error not correct column 1");
					break;
				}
				str = sqlite3_column_name(query,2);
				str = g_strrstr_len(NAME_COLUMN_2,SIZE_NAME_COLUMN_2,str);
				if(str == NULL){
					g_critical("SQL error not correct column 2");
					break;
				}
				str = sqlite3_column_name(query,3);
				str = g_strrstr_len(NAME_COLUMN_3,SIZE_NAME_COLUMN_3,str);
				if(str == NULL){
					g_critical("SQL error not correct column 3");
					break;
				}
				str = sqlite3_column_name(query,4);
				str = g_strrstr_len(NAME_COLUMN_4,SIZE_NAME_COLUMN_4,str);
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
	return g_date_time_new_local(2015,1,1,hour,minut,(gdouble)second);
}
#define MATCH_NAME      -1

job_s temp_job = {0};
GString * query_row_job = NULL;

/*проверка введеных значений лежит на вышестояшей функции*/
static int insert_job(const char * name,const char *pressure,const char * time
              ,const char * str_uprise,const char * str_lowering )
{
	job_s * pj;
	char * sql_error;
	int rc;

	if(temp_job.name == NULL){
	 	temp_job.name = g_string_new(name);
	}
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
	pj->uprise = g_ascii_strtoll(str_uprise,NULL,10);
	pj->lowering = g_ascii_strtoll(str_lowering,NULL,10);

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

int delete_job(const char * name)
{
	int rc;
	char * sql_error;

	if(temp_job.name == NULL){
		temp_job.name = g_string_new(name);
	}
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
static int check_table_job(void * ud, int argc, char **argv, char ** col_name)
{
	int i;
	char * name;
	char * value;
	char * check;
	for(i = 0;i < argc;i++){
		name = col_name[i];
		value = argv[i];
		check = NULL;
		check = g_strrstr_len(name,SIZE_STR_COL_NAME_JOB_TABLE,STR_COL_NAME_JOB_TABLE);
		if(check != NULL){
			check = g_strrstr_len(value,SIZE_STR_JOB_TABLE_NAME,STR_JOB_TABLE_NAME);
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

	rc = sqlite3_exec(database,QUERY_JOB_TABLE,check_table_job,NULL,&error_message);
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

	rc = fill_list_job();
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
/*  общие элементы управления                                                */
/*****************************************************************************/
job_s * current_job = NULL;
GtkWidget * fra_info = NULL;
GtkWidget * fra_mode_auto = NULL;
GtkWidget * fra_mode_manual = NULL;
GtkWidget * fra_job_load = NULL;
GtkWidget * fra_job_save = NULL;

static char STR_INFO[] = "ИНФОРМАЦИЯ";
static char STR_MODE_AUTO[] = "Автоматическое управление";
static char STR_MODE_MANUAL[] = "Ручное Управление";
static char STR_JOB_LOAD[] = "Загрузить работу";
static char STR_JOB_SAVE[] = "Новая работа";

/*****************************************************************************/
/*  подменю управление                                                       */
/*****************************************************************************/

void activate_menu_auto_mode(GtkMenuItem * im,gpointer d)
{
	gtk_widget_hide(fra_info);
	gtk_widget_hide(fra_mode_manual);
	gtk_widget_hide(fra_job_load);
	gtk_widget_hide(fra_job_save);
	gtk_widget_show(fra_mode_auto);
	g_message("%s",STR_MODE_AUTO);
}

void activate_menu_manual_mode(GtkMenuItem * im,gpointer d)
{
	gtk_widget_hide(fra_info);
	gtk_widget_hide(fra_mode_auto);
	gtk_widget_hide(fra_job_load);
	gtk_widget_hide(fra_job_save);
	gtk_widget_show(fra_mode_manual);

	set_manual_mode();

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
	gtk_widget_hide(fra_info);
	gtk_widget_hide(fra_mode_auto);
	gtk_widget_hide(fra_mode_manual);
	gtk_widget_hide(fra_job_save);
	gtk_widget_show(fra_job_load);
	g_message("%s",STR_JOB_LOAD);
}

void activate_menu_create_job(GtkMenuItem * b,gpointer d)
{
	gtk_widget_hide(fra_info);
	gtk_widget_hide(fra_mode_auto);
	gtk_widget_hide(fra_mode_manual);
	gtk_widget_hide(fra_job_load);
	gtk_widget_show(fra_job_save);
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
	g_signal_connect(menite_temp,"activate",G_CALLBACK(activate_menu_create_job),NULL);
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

	gtk_widget_show(men_work);
	gtk_widget_show(menbar_main);

 	return menbar_main;
}

/*****************************************************************************/
/* Панель управления и информации                                            */
/*****************************************************************************/
static char STR_NOT_DEVICE[] = "Загрузите информацию об изделии";
/*TODO сделать описание всех значений работы*/
GtkWidget * create_info(void)
{
	GtkWidget * lab_info;
	PangoContext * pancon_info;
	PangoFontDescription * panfondes_info;
	GdkRGBA color_info;

	/*TODO тестирование*/
	job_iter_init();
	current_job = job_iter_next();
	fra_info = gtk_frame_new(STR_INFO);
	gtk_frame_set_label_align(GTK_FRAME(fra_info),0.5,0.5);

	if(current_job != NULL){
		lab_info = gtk_label_new(current_job->name->str);
	}
	else{
		lab_info = gtk_label_new(STR_NOT_DEVICE);
	}
	/**/

	pancon_info = gtk_widget_get_pango_context(lab_info);
	panfondes_info = pango_context_get_font_description(pancon_info);
	pango_font_description_set_size(panfondes_info,30000);
	gtk_widget_override_font(lab_info,panfondes_info);
	color_info.red = 0;
	color_info.green = 0;
	color_info.blue = 1;
	color_info.alpha = 1;
	gtk_widget_override_color(lab_info,GTK_STATE_FLAG_NORMAL,&color_info);

	gtk_container_add(GTK_CONTAINER(fra_info),lab_info);

	gtk_widget_show(lab_info);
	gtk_widget_show(fra_info);
	return fra_info;
}

GtkWidget * create_mode_auto(void)
{
	fra_mode_auto = gtk_frame_new(STR_MODE_AUTO);
	gtk_frame_set_label_align(GTK_FRAME(fra_mode_auto),0.5,0.5);
	return fra_mode_auto;
}
/*************************************/
void clicked_button_manual_up(GtkButton * b,gpointer d)
{
	/*g_debug("Вверх ручная работа");*/
}
void press_button_manual_up(GtkWidget * b,GdkEvent * e,gpointer ud)
{
	g_debug("Вверх");
	set_manual_up();
}
void release_button_manual_up(GtkWidget * b,GdkEvent * e,gpointer ud)
{
	g_debug("Стоп Вверх");
	set_manual_null();
}

void clicked_button_manual_down(GtkButton * b,gpointer d)
{
	/*g_debug("Вниз ручная работа");*/
}
void press_button_manual_down(GtkWidget * b,GdkEvent * e,gpointer ud)
{
	g_debug("Вниз");
	set_manual_down();
}
void release_button_manual_down(GtkWidget * b,GdkEvent * e,gpointer ud)
{
	g_debug("Стоп Вниз");
	set_manual_null();
}

void clicked_button_manual_left(GtkButton * b,gpointer d)
{
	g_message("Влево ручная работа");
}

void clicked_button_manual_right(GtkButton * b,gpointer d)
{
	g_message("Вправо ручная работа");
}

void clicked_button_manual_close(GtkButton * b,gpointer d)
{
	g_message("Закрыть задвижку ручная работа");
}

void clicked_button_manual_open(GtkButton * b,gpointer d)
{
	g_message("Открыть задвижку ручная работа");
}
char STR_BUTTON_MANUAL_UP[] =    "ВВЕРХ";
char STR_BUTTON_MANUAL_DOWN[] =  "ВНИЗ";
char STR_BUTTON_MANUAL_LEFT[] =  "ВЛЕВО";
char STR_BUTTON_MANUAL_RIGHT[] = "ВПРАВО";
char STR_BUTTON_MANUAL_CLOSE[] = "Закрыть";
char STR_BUTTON_MANUAL_OPEN[] =  "Открыть";

GtkWidget * create_mode_manual(void)
{
	GtkWidget * gri_mode;
	GtkWidget * but_up;
	GtkWidget * but_down;
	GtkWidget * but_left;
	GtkWidget * but_right;
	GtkWidget * but_close;
	GtkWidget * but_open;

	fra_mode_manual = gtk_frame_new(STR_MODE_MANUAL);
	gtk_frame_set_label_align(GTK_FRAME(fra_mode_manual),0.5,0.5);

	gri_mode = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(gri_mode),5);

	but_up = gtk_button_new_with_label(STR_BUTTON_MANUAL_UP);
	g_signal_connect(but_up,"clicked",G_CALLBACK(clicked_button_manual_up),NULL);
	g_signal_connect(but_up,"button-press-event",G_CALLBACK(press_button_manual_up),NULL);
	g_signal_connect(but_up,"button-release-event",G_CALLBACK(release_button_manual_up),NULL);

	but_down = gtk_button_new_with_label(STR_BUTTON_MANUAL_DOWN);
	g_signal_connect(but_down,"clicked",G_CALLBACK(clicked_button_manual_down),NULL);
	g_signal_connect(but_down,"button-press-event",G_CALLBACK(press_button_manual_down),NULL);
	g_signal_connect(but_down,"button-release-event",G_CALLBACK(release_button_manual_down),NULL);

	but_left = gtk_button_new_with_label(STR_BUTTON_MANUAL_LEFT);
	g_signal_connect(but_left,"clicked",G_CALLBACK(clicked_button_manual_left),NULL);

	but_right = gtk_button_new_with_label(STR_BUTTON_MANUAL_RIGHT);
	g_signal_connect(but_right,"clicked",G_CALLBACK(clicked_button_manual_right),NULL);

	but_close = gtk_button_new_with_label(STR_BUTTON_MANUAL_CLOSE);
	g_signal_connect(but_close,"clicked",G_CALLBACK(clicked_button_manual_close),NULL);

	but_open = gtk_button_new_with_label(STR_BUTTON_MANUAL_OPEN);
	g_signal_connect(but_open,"clicked",G_CALLBACK(clicked_button_manual_open),NULL);

	gtk_container_add(GTK_CONTAINER(fra_mode_manual),gri_mode);

	gtk_grid_attach(GTK_GRID(gri_mode),but_up,1,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_down,1,3,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_left,0,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_right,2,2,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_close,3,1,1,1);
	gtk_grid_attach(GTK_GRID(gri_mode),but_open,3,2,1,1);

	gtk_widget_show(but_close);
	gtk_widget_show(but_open);
	gtk_widget_show(but_right);
	gtk_widget_show(but_left);
	gtk_widget_show(but_down);
	gtk_widget_show(but_up);
	gtk_widget_show(gri_mode);
	gtk_widget_hide(fra_mode_manual);

	return fra_mode_manual;
}
/*************************************/
GtkWidget * create_job_load(void)
{
	fra_job_load = gtk_frame_new(STR_JOB_LOAD);
	gtk_frame_set_label_align(GTK_FRAME(fra_job_load),0.5,0.5);
	return fra_job_load;
}

GtkWidget * create_job_save(void)
{
	fra_job_save = gtk_frame_new(STR_JOB_SAVE);
	gtk_frame_set_label_align(GTK_FRAME(fra_job_save),0.5,0.5);
	return fra_job_save;
}

GtkWidget * create_control_panel(void)
{
	GtkWidget * gri_control;

	gri_control = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(gri_control),5);
	/*TODO установить нужный размер*/
	gtk_widget_set_size_request(gri_control,-1,300);

	fra_info = create_info();
	fra_mode_auto = create_mode_auto();
	fra_mode_manual = create_mode_manual();
	fra_job_load = create_job_load();
	fra_job_save = create_job_save();
	gtk_grid_attach(GTK_GRID(gri_control),fra_info       ,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_control),fra_mode_auto  ,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_control),fra_mode_manual,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_control),fra_job_load   ,0,0,1,1);
	gtk_grid_attach(GTK_GRID(gri_control),fra_job_save   ,0,0,1,1);

	gtk_widget_show(gri_control);
	return gri_control;
}
/*****************************************************************************/
#if 0
GtkWidget * l_control;
GtkWidget * b_auto_start;
GtkWidget * b_auto_stop;
GtkWidget * b_auto_pause;


GtkWidget * load_job_window = NULL;
GtkWidget * create_job_window = NULL;

GtkEntryBuffer * eb_name_job = NULL;
GtkEntryBuffer * eb_pressure = NULL;
GtkEntryBuffer * eb_time = NULL;
GtkEntryBuffer * eb_uprise = NULL;
GtkEntryBuffer * eb_lowering = NULL;

/*****************************************************************************/
char STR_DEFAULT_PRESSURE[] = "2";

int check_pressure(const char * pressure)
{
	char * c;
	c = g_strrstr_len(pressure,1,STR_DEFAULT_PRESSURE);
	if(c == NULL){
		return FAILURE;
	}
	return SUCCESS;
}

int check_time(const char * time)
{
	return SUCCESS;
}

int check_angle(const char * str_uprise,long int * v_uprise,const char * str_lowering,long int * v_lowering)
{
	gint64 t_uprise  = g_ascii_strtoll(str_uprise,NULL,10);
	gint64 t_lowering = g_ascii_strtoll(str_lowering,NULL,10);

	if(t_uprise <= t_lowering){
		/*TODO*/
		return FAILURE;
	}
	*v_uprise = t_uprise;
	*v_lowering = t_lowering;

	return SUCCESS;
}

void create_job(GtkButton *b,gpointer d)
{
	int rc;
	GtkWidget * error;

	rc = check_name_job(gtk_entry_buffer_get_text(eb_name_job));
	if(rc != SUCCESS){
		return ;
	}
	g_debug(" :> %s",gtk_entry_buffer_get_text(eb_name_job));

	rc = check_pressure(gtk_entry_buffer_get_text(eb_pressure));
	if(rc != SUCCESS){
		return ;
	}
	g_debug(" :> %s",gtk_entry_buffer_get_text(eb_pressure));

	rc = check_time(gtk_entry_buffer_get_text(eb_time));
	if(rc != SUCCESS){
		return;
	}
	g_debug(" :> %s",gtk_entry_buffer_get_text(eb_time));

	g_debug(" :> %s",gtk_entry_buffer_get_text(eb_uprise));
	g_debug(" :> %s",gtk_entry_buffer_get_text(eb_lowering));

	rc = insert_job(gtk_entry_buffer_get_text(eb_name_job)
	             ,gtk_entry_buffer_get_text(eb_pressure)
	             ,gtk_entry_buffer_get_text(eb_time)
	             ,gtk_entry_buffer_get_text(eb_uprise)
	             ,gtk_entry_buffer_get_text(eb_lowering));
	if(rc != SUCCESS){
		error = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR
		                              ,GTK_BUTTONS_CLOSE,"Введеные данные не корректны");
		gtk_dialog_run(GTK_DIALOG(error));
		gtk_widget_destroy(error);
		return;
	}

	gtk_widget_destroy(create_job_window);
	g_message("Сохранить работу");
}

void create_job_window_destroy(GtkWidget * w,gpointer d)
{
	g_message("create_job_window destroy");
}
/*****************************************************************************/


GtkWidget * info_panel = NULL;
GString * s_name_job_info_panel = NULL;
GtkWidget * w_name_job_info_panel = NULL;


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


char STR_CONTROL_PANEL[] = "УПРАВЛЕНИЕ";
char STR_BUTTON_AUTO_START[] = "СТАРТ";
char STR_BUTTON_AUTO_STOP[]  = "СТОП";
char STR_BUTTON_AUTO_PAUSE[] = "ПАУЗА";
#define CONTROL_BUTTON_SPACING        3



	gtk_grid_attach(GTK_GRID(cgrid),l_control,1,0,4,1);

	gtk_grid_attach(GTK_GRID(cgrid),b_auto_start,0,1,2,1);
	gtk_grid_attach(GTK_GRID(cgrid),b_auto_stop,2,1,2,1);
	gtk_grid_attach(GTK_GRID(cgrid),b_auto_pause,4,1,2,1);


	gtk_widget_show(b_auto_pause);
	gtk_widget_show(b_auto_stop);
	gtk_widget_show(b_auto_start);
	gtk_widget_show(l_control);

	gtk_widget_show(cgrid);
	gtk_widget_show(cframe);

	return cframe;
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
	GtkWidget * iframe;
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

	gtk_container_add(GTK_CONTAINER(iframe),igrid);
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
	gtk_widget_show(iframe);

	return iframe;
}
/**************************************/

GtkWidget * control_panel = NULL;

#define CONTROL_SPACING        1

GtkWidget * create_control_panel(void)
{
	GtkWidget * cbutton;
	GtkWidget * spanel;

	control_panel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,CONTROL_SPACING);

	cbutton = create_control_button();
	spanel = create_set_panel();

	gtk_box_pack_start(GTK_BOX(control_panel),cbutton,TRUE,TRUE,CONTROL_SPACING);
	gtk_box_pack_start(GTK_BOX(control_panel),spanel,TRUE,TRUE,CONTROL_SPACING);
	gtk_widget_show(control_panel);
	return control_panel;
}

#endif
/*****************************************************************************/
