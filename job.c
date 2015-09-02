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

#include "total.h"
#include "cid.h"


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

			g_debug("%s",sqlite3_column_database_name(query,0));
			g_debug("%s",sqlite3_column_table_name(query,0));

			if(check_column != OK){
				int amount_column;
				amount_column = sqlite3_data_count(query);
				if(amount_column != DEFAULT_AMOUNT_COLUMN){
					g_critical("SQL error not correct table job : %d",amount_column);
					break;
				}
				str = sqlite3_column_database_name(query,0);
				str = g_strrstr_len(NAME_COLUMN_0,SIZE_NAME_COLUMN_0,str);
				if(str == NULL){
					g_critical("SQL error not correct column 0");
					break;
				}
				str = sqlite3_column_database_name(query,1);
				str = g_strrstr_len(NAME_COLUMN_1,SIZE_NAME_COLUMN_1,str);
				if(str == NULL){
					g_critical("SQL error not correct column 1");
					break;
				}
				str = sqlite3_column_database_name(query,2);
				str = g_strrstr_len(NAME_COLUMN_2,SIZE_NAME_COLUMN_2,str);
				if(str == NULL){
					g_critical("SQL error not correct column 2");
					break;
				}
				str = sqlite3_column_database_name(query,3);
				str = g_strrstr_len(NAME_COLUMN_3,SIZE_NAME_COLUMN_3,str);
				if(str == NULL){
					g_critical("SQL error not correct column 3");
					break;
				}
				str = sqlite3_column_database_name(query,4);
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

GString * query_row_job = NULL;
/*проверка введеных значений лежит на вышестояшей функции*/
static int insert_job(const char * name,const char *pressure,const char * time
              ,const char * str_uprise,const char * str_lowering )
{
	job_s j;
	job_s * pj;
	char * sql_error;
	int rc;

	j.name = name;
	rc = g_hash_table_contains(list_job,&j);
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
		query_row_job = g_string_new(STR_INSERT_JOB);
	}
	g_string_printf(query_insert_job,"%s",STR_INSERT_JOB);
	g_string_append_printf(query_insert_job,"\'%s\',",pj->name->str);
	g_string_append_printf(query_insert_job,"%d,",pj->pressure);
	g_string_append_printf(query_insert_job,"%ld,",(long int)g_date_time_to_unix(pj->time));
	g_string_append_printf(query_insert_job,"%ld,%ld)",pj->uprise,pj->lowering);

	rc = sqlite3_exec(database,query_insert_job->str,NULL,NULL,&sql_error);
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
	job_s j;
	j.name = name;
	int rc;
	char * sql_error;

	g_hash_table_remove(list_job,&j);
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

/*****************************************************************************/
GtkWidget * load_job_window = NULL;
GtkWidget * create_job_window = NULL;

GtkEntryBuffer * eb_name_job = NULL;
GtkEntryBuffer * eb_pressure = NULL;
GtkEntryBuffer * eb_time = NULL;
GtkEntryBuffer * eb_uprise = NULL;
GtkEntryBuffer * eb_lowering = NULL;

/*****************************************************************************/
void load_job(GtkButton * b,gpointer d)
{
	g_message("load_job");
	gtk_widget_destroy(load_job_window);
}

void load_job_window_destroy(GtkWidget * w,gpointer d)
{
	g_message("load_job_window destroy");
}

int check_name_job(const char * name)
{
	return SUCCESS;
}
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


int _save_job(const char * name,const char *pressure,const char * time
            ,const char * str_uprise,const char * str_lowering )
{
	int rc;
	char * error_message = NULL;
	long int v_uprise = 0;
	long int v_lowering = 0;

}

void create_job(GtkButton *b,gpointer d)
{
	int rc;
	GtkWidget * error;

	rc = check_name_job(gtk_entry_buffer_get_text(eb_name_job));
	if(rc != SUCCESS){
		return ;
	}
	g_message(" :> %s",gtk_entry_buffer_get_text(eb_name_job));

	rc = check_pressure(gtk_entry_buffer_get_text(eb_pressure));
	if(rc != SUCCESS){
		return ;
	}
	g_message(" :> %s",gtk_entry_buffer_get_text(eb_pressure));

	rc = check_time(gtk_entry_buffer_get_text(eb_time));
	if(rc != SUCCESS){
		return;
	}
	g_message(" :> %s",gtk_entry_buffer_get_text(eb_time));

	g_message(" :> %s",gtk_entry_buffer_get_text(eb_uprise));
	g_message(" :> %s",gtk_entry_buffer_get_text(eb_lowering));

	rc = save_job(gtk_entry_buffer_get_text(eb_name_job)
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
char STR_LOAD_JOB_WINDOW[] = "Загрузить работу";
char STR_CREATE_JOB_WINDOW[] = "Создать работу";

void create_window_load_job(GtkMenuItem * b,gpointer d)
{
	GtkWidget * hbox;
	GtkWidget * b_exit;

	load_job_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_screen(GTK_WINDOW (load_job_window),gtk_widget_get_screen(main_window));
	gtk_window_set_title(GTK_WINDOW(load_job_window),STR_LOAD_JOB_WINDOW);
	gtk_container_set_border_width(GTK_CONTAINER(load_job_window),5);
	g_signal_connect (load_job_window,"destroy",G_CALLBACK(load_job_window_destroy),NULL);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
	gtk_container_add(GTK_CONTAINER(load_job_window),hbox);

	b_exit = gtk_button_new_with_label("Загрузить");
	g_signal_connect(b_exit,"clicked",G_CALLBACK(load_job),NULL);

	gtk_box_pack_start(GTK_BOX(hbox),b_exit,TRUE,TRUE,5);
	gtk_widget_show(b_exit);
	gtk_widget_show(hbox);
	gtk_widget_show(load_job_window);
}

void create_window_create_job(GtkMenuItem * b,gpointer d)
{
	GtkWidget * cwgrid;
	GtkWidget * b_save_job;

	GtkWidget * l_name_job;
	GtkWidget * l_pressure;
	GtkWidget * l_time;
	GtkWidget * l_uprise;
	GtkWidget * l_lowering;
	GtkWidget * e_name_job;
	GtkWidget * e_pressure;
	GtkWidget * e_time;
	GtkWidget * e_uprise;
	GtkWidget * e_lowering;

	create_job_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_screen(GTK_WINDOW (create_job_window),gtk_widget_get_screen(main_window));
	gtk_window_set_title(GTK_WINDOW(create_job_window),STR_CREATE_JOB_WINDOW);
	gtk_container_set_border_width(GTK_CONTAINER(create_job_window),5);
	g_signal_connect (create_job_window,"destroy",G_CALLBACK(create_job_window_destroy),NULL);

	cwgrid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(create_job_window),cwgrid);
	gtk_container_set_border_width(GTK_CONTAINER(cwgrid),5);

	b_save_job = gtk_button_new_with_label("сохранить");
	gtk_grid_attach(GTK_GRID(cwgrid),b_save_job,2,2,1,1);
	g_signal_connect(b_save_job,"clicked",G_CALLBACK(create_job),NULL);

	l_name_job = gtk_label_new("Наименование работы");
	eb_name_job = gtk_entry_buffer_new("------",-1);
	e_name_job = gtk_entry_new_with_buffer(eb_name_job);
	gtk_entry_set_width_chars(GTK_ENTRY(e_name_job),30);
	gtk_widget_set_halign(e_name_job,GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(cwgrid),l_name_job,0,0,2,1);
	gtk_grid_attach(GTK_GRID(cwgrid),e_name_job,0,1,2,1);

	l_pressure = gtk_label_new("Давление");
	eb_pressure = gtk_entry_buffer_new(STR_DEFAULT_PRESSURE,-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(eb_pressure),1);
	e_pressure = gtk_entry_new_with_buffer(eb_pressure);
	gtk_entry_set_width_chars(GTK_ENTRY(e_pressure),3);
	gtk_widget_set_halign(e_pressure,GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(cwgrid),l_pressure,2,0,1,1);
	gtk_grid_attach(GTK_GRID(cwgrid),e_pressure,2,1,1,1);

	l_time = gtk_label_new("Время работы");
	eb_time = gtk_entry_buffer_new("00:00:00",-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(eb_time),8);
	e_time = gtk_entry_new_with_buffer(eb_time);
	gtk_entry_set_width_chars(GTK_ENTRY(e_time),10);
	gtk_widget_set_halign(e_time,GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(cwgrid),l_time,3,0,1,1);
	gtk_grid_attach(GTK_GRID(cwgrid),e_time,3,1,1,1);

	l_uprise = gtk_label_new("Угол подъема");
	eb_uprise = gtk_entry_buffer_new("000",-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(eb_uprise),3);
	e_uprise = gtk_entry_new_with_buffer(eb_uprise);
	gtk_entry_set_width_chars(GTK_ENTRY(e_uprise),5);
	gtk_widget_set_halign(e_uprise,GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(cwgrid),l_uprise,4,0,1,1);
	gtk_grid_attach(GTK_GRID(cwgrid),e_uprise,4,1,1,1);

	l_lowering = gtk_label_new("Угол опускания");
	eb_lowering = gtk_entry_buffer_new("000",-1);
	gtk_entry_buffer_set_max_length(GTK_ENTRY_BUFFER(eb_lowering),3);
	e_lowering = gtk_entry_new_with_buffer(eb_lowering);
	gtk_entry_set_width_chars(GTK_ENTRY(e_lowering),5);
	gtk_widget_set_halign(e_lowering,GTK_ALIGN_CENTER);
	gtk_grid_attach(GTK_GRID(cwgrid),l_lowering,5,0,1,1);
	gtk_grid_attach(GTK_GRID(cwgrid),e_lowering,5,1,1,1);

	gtk_widget_show(l_lowering);
	gtk_widget_show(l_uprise);
	gtk_widget_show(l_time);
	gtk_widget_show(l_pressure);
	gtk_widget_show(l_name_job);
	gtk_widget_show(e_lowering);
	gtk_widget_show(e_uprise);
	gtk_widget_show(e_time);
	gtk_widget_show(e_pressure);
	gtk_widget_show(e_name_job);
	gtk_widget_show(b_save_job);
	gtk_widget_show(cwgrid);
	gtk_widget_show(create_job_window);
	g_message("Cоздать работу");
}

char STR_JOB[] = "Работа";
char STR_LOAD_JOB[] = "Загрузить";
char STR_CREATE_JOB[] = "Создать";
char STR_EXIT[] = "ВЫХОД";

GtkWidget * job_panel = NULL;

void job_panel_destroy(GtkWidget * w,gpointer ud)
{
	deinit_db();
}

GtkWidget * create_job_panel(void)
{
	GtkWidget * menu_work;
	GtkWidget * mitem;

	job_panel = gtk_menu_bar_new();
	gtk_widget_set_halign(job_panel,GTK_ALIGN_START);
	g_signal_connect(job_panel,"unrealize",G_CALLBACK(job_panel_destroy),NULL);

	mitem = gtk_menu_item_new_with_label(STR_JOB);
	gtk_menu_shell_append(GTK_MENU_SHELL(job_panel),mitem);
	gtk_widget_show(mitem);

	menu_work = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem),menu_work);

	mitem = gtk_menu_item_new_with_label(STR_LOAD_JOB);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_work),mitem);
	g_signal_connect(mitem,"activate",G_CALLBACK(create_window_load_job),NULL);
	gtk_widget_add_accelerator(mitem,"activate",accel_group
	                          ,'L',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_widget_show(mitem);

	mitem = gtk_menu_item_new_with_label(STR_CREATE_JOB);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_work),mitem);
	g_signal_connect(mitem,"activate",G_CALLBACK(create_window_create_job),NULL);
	gtk_widget_add_accelerator(mitem,"activate",accel_group
	                          ,'C',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_widget_show(mitem);

	mitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_work),mitem);
	gtk_widget_show(mitem);

	mitem = gtk_menu_item_new_with_label(STR_EXIT);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_work),mitem);
	g_signal_connect(mitem,"activate",G_CALLBACK(main_exit),NULL);
	gtk_widget_add_accelerator(mitem,"activate",accel_group
	                          ,'Q',GDK_CONTROL_MASK,GTK_ACCEL_VISIBLE);
	gtk_widget_show(mitem);

	mitem = create_menu_mode();
	gtk_menu_shell_append(GTK_MENU_SHELL(job_panel),mitem);

	gtk_widget_show(menu_work);
	gtk_widget_show(job_panel);

 	return job_panel;
}
/*****************************************************************************/

